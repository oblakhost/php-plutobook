// SPDX-License-Identifier: Apache-2.0
// Remote assets. The binding drives libcurl itself, never PHP's curl extension and never
// PlutoBook's built-in fetcher, so every request follows the document's policy:
// http/https only, allowed hosts on every hop, no internal addresses, bounded size and time.
#include "binding.h"
#include <curl/curl.h>
#ifdef PHP_WIN32
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <time.h>
#endif

#if PLUTOBOOK_VERSION != PLUTOBOOK_VERSION_ENCODE(0, 20, 0)
#error "PlutoBook 0.20.0 is required"
#endif
#if LIBCURL_VERSION_NUM < 0x074400
#error "libcurl 7.68.0 or newer is required"
#endif

#define PHP_PLUTOBOOK_USER_AGENT "php-plutobook/" PHP_PLUTOBOOK_VERSION

typedef struct {
	char *bytes;
	size_t size;
	size_t capacity;
	size_t limit;
	bool too_large;
	bool blocked;
	bool interrupted;
	// Whether the document budgets, rather than the per-asset settings, set limit and deadline.
	bool total_limited;
	bool document_limited;
	char blocked_address[INET6_ADDRSTRLEN];
} php_plutobook_download;

// The libcurl loaded at run time can be older than the headers. EL8's libcurl 7.61.1 has
// a backported URL API, so a module built for newer systems loads there. Remote assets
// then stay off: the module still renders, and failing startup would stop PHP itself.
static bool php_plutobook_curl_supported;
static char php_plutobook_curl_unsupported[64];
// Built from curl_version_info() rather than curl_version(): the latter also asks OpenLDAP
// for its version, and libldap then sets up global state, including a host name lookup,
// that it never frees. Every phpinfo() would pay for that lookup.
static char php_plutobook_curl_description[192];
// CURLOPT_NOSIGNAL is always set, since libcurl's SIGALRM timeouts are unsafe in PHP. Only an
// asynchronous resolver (threaded or c-ares) then lets the timeouts stop a DNS lookup.
static bool php_plutobook_async_dns;

bool php_plutobook_network_startup(void)
{
	const curl_version_info_data *curl = curl_version_info(CURLVERSION_NOW);
	php_plutobook_curl_supported = curl->version_num >= 0x074400;
	php_plutobook_async_dns = (curl->features & CURL_VERSION_ASYNCHDNS) != 0;
	if (!php_plutobook_curl_supported) {
		snprintf(php_plutobook_curl_unsupported, sizeof(php_plutobook_curl_unsupported),
		         "libcurl %s is older than 7.68.0", curl->version);
		return true;
	}
	snprintf(php_plutobook_curl_description, sizeof(php_plutobook_curl_description),
	         "libcurl/%s%s%s%s%s", curl->version, curl->ssl_version ? " " : "",
	         curl->ssl_version ? curl->ssl_version : "", curl->libz_version ? " zlib/" : "",
	         curl->libz_version ? curl->libz_version : "");
	return curl_global_init(CURL_GLOBAL_DEFAULT) == CURLE_OK;
}

void php_plutobook_network_shutdown(void)
{
	if (php_plutobook_curl_supported) {
		curl_global_cleanup();
	}
}

const char *php_plutobook_network_version(void)
{
	return php_plutobook_curl_supported ? php_plutobook_curl_description
	                                    : php_plutobook_curl_unsupported;
}

const char *php_plutobook_network_dns(void)
{
	if (!php_plutobook_curl_supported) {
		return "not used (libcurl is older than 7.68.0)";
	}
	return php_plutobook_async_dns
	           ? "bounded by the download timeouts"
	           : "not bounded by the download timeouts (libcurl resolves host names synchronously)";
}

void php_plutobook_network_release(php_plutobook_document *doc)
{
	if (doc->http) {
		curl_easy_cleanup(doc->http);
		doc->http = NULL;
	}
}

bool php_plutobook_is_remote_url(const zend_string *url)
{
	return !strncasecmp(ZSTR_VAL(url), "http://", 7) || !strncasecmp(ZSTR_VAL(url), "https://", 8);
}

// Split an absolute URL into scheme and host. The caller frees both with curl_free().
static bool php_plutobook_url_parts(const char *url, char **scheme, char **host)
{
	CURLU *parts = curl_url();
	*scheme = *host = NULL;
	bool ok = parts && curl_url_set(parts, CURLUPART_URL, url, 0) == CURLUE_OK &&
	          curl_url_get(parts, CURLUPART_SCHEME, scheme, 0) == CURLUE_OK &&
	          curl_url_get(parts, CURLUPART_HOST, host, 0) == CURLUE_OK && **host;
	curl_url_cleanup(parts);
	if (!ok) {
		curl_free(*scheme);
		curl_free(*host);
		*scheme = *host = NULL;
	}
	return ok;
}

bool php_plutobook_check_remote_base(zend_string *base, uint32_t arg)
{
	char *scheme = NULL, *host = NULL;
	bool ok = strlen(ZSTR_VAL(base)) == ZSTR_LEN(base) &&
	          php_plutobook_url_parts(ZSTR_VAL(base), &scheme, &host);
	curl_free(scheme);
	curl_free(host);
	if (!ok) {
		zend_argument_value_error(arg, "must be a valid http:// or https:// URL with a host");
	}
	return ok;
}

// Accept "*", "example.com", "*.example.com" and bracketed IPv6 literals such as "[::1]".
static bool php_plutobook_valid_host_pattern(const zend_string *pattern)
{
	const char *host = ZSTR_VAL(pattern);
	size_t length = ZSTR_LEN(pattern);
	if (length == 1 && *host == '*') {
		return true;
	}
	if (length > 2 && host[0] == '[') {
		return host[length - 1] == ']' &&
		       strspn(host + 1, "0123456789abcdefABCDEF:.") == length - 2;
	}
	if (length > 2 && host[0] == '*' && host[1] == '.') {
		host += 2;
		length -= 2;
	}
	if (!length || length > 253 || host[0] == '.' || host[length - 1] == '.' ||
	    strstr(host, "..")) {
		return false;
	}
	return strspn(host, "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.-_") ==
	       length;
}

HashTable *php_plutobook_parse_allowed_hosts(HashTable *hosts, uint32_t arg, bool *failed)
{
	*failed = false;
	if (!zend_hash_num_elements(hosts)) {
		return NULL;
	}
	HashTable *allowed = zend_new_array(zend_hash_num_elements(hosts));
	zval *entry;
	ZEND_HASH_FOREACH_VAL(hosts, entry)
	{
		if (Z_TYPE_P(entry) != IS_STRING) {
			zend_argument_type_error(arg, "must contain only strings, %s given",
			                         zend_zval_type_name(entry));
			*failed = true;
			break;
		}
		if (!php_plutobook_valid_host_pattern(Z_STR_P(entry))) {
			zend_argument_value_error(arg, "must contain host names such as example.com, "
			                               "*.example.com, [::1] or *");
			*failed = true;
			break;
		}
		zval lower;
		ZVAL_STR(&lower, zend_string_tolower(Z_STR_P(entry)));
		zend_hash_next_index_insert(allowed, &lower);
	}
	ZEND_HASH_FOREACH_END();
	if (*failed) {
		zend_array_destroy(allowed);
		return NULL;
	}
	return allowed;
}

static bool php_plutobook_host_allowed(HashTable *allowed, const char *host)
{
	size_t length = strlen(host);
	zval *entry;
	ZEND_HASH_FOREACH_VAL(allowed, entry)
	{
		const char *pattern = Z_STRVAL_P(entry);
		size_t size = Z_STRLEN_P(entry);
		if (size == 1 && *pattern == '*') {
			return true;
		}
		// "*.example.com" matches subdomains at any depth, not example.com itself.
		if (size > 2 && pattern[0] == '*' && length > size - 1 &&
		    !strcasecmp(host + length - (size - 1), pattern + 1)) {
			return true;
		}
		if (!strcasecmp(host, pattern)) {
			return true;
		}
	}
	ZEND_HASH_FOREACH_END();
	return false;
}

// Loopback, private, shared, link-local (cloud metadata), multicast and reserved IPv4 ranges.
static bool php_plutobook_internal_ipv4(uint32_t address)
{
	const uint32_t a = ntohl(address);
	return (a >> 24) == 0 || (a >> 24) == 10 || (a >> 24) == 127 ||
	       (a & 0xffc00000U) == 0x64400000U || (a & 0xffff0000U) == 0xa9fe0000U ||
	       (a & 0xfff00000U) == 0xac100000U || (a & 0xffffff00U) == 0xc0000000U ||
	       (a & 0xffff0000U) == 0xc0a80000U || (a & 0xfffe0000U) == 0xc6120000U || (a >> 28) >= 0xe;
}

static bool php_plutobook_internal_ipv6(const struct in6_addr *address)
{
	static const uint8_t mapped[12] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xff, 0xff};
	static const uint8_t nat64[12] = {0, 0x64, 0xff, 0x9b};
	static const uint8_t zero[12] = {0};
	const uint8_t *b = address->s6_addr;
	// ::/96 covers ::, ::1 and the deprecated IPv4-compatible form. None of it is public.
	if (!memcmp(b, zero, 12)) {
		return true;
	}
	// IPv4-mapped and NAT64 addresses reach the embedded IPv4 address.
	if (!memcmp(b, mapped, 12) || !memcmp(b, nat64, 12)) {
		uint32_t embedded;
		memcpy(&embedded, b + 12, sizeof(embedded));
		return php_plutobook_internal_ipv4(embedded);
	}
	// fc00::/7 unique local, fe80::/10 link-local, fec0::/10 site-local, ff00::/8 multicast.
	return (b[0] & 0xfe) == 0xfc || (b[0] == 0xfe && (b[1] & 0x80) == 0x80) || b[0] == 0xff;
}

// Every connection, including redirects and connections to re-resolved names, opens here, so
// the check applies to the address actually used rather than to the name in the URL.
static curl_socket_t php_plutobook_open_socket(void *closure, curlsocktype purpose,
                                               struct curl_sockaddr *address)
{
	php_plutobook_download *download = closure;
	bool internal = false;
	if (address->family == AF_INET) {
		const struct sockaddr_in *ipv4 = (const struct sockaddr_in *)&address->addr;
		internal = php_plutobook_internal_ipv4(ipv4->sin_addr.s_addr);
		inet_ntop(AF_INET, &ipv4->sin_addr, download->blocked_address,
		          sizeof(download->blocked_address));
	} else if (address->family == AF_INET6) {
		const struct sockaddr_in6 *ipv6 = (const struct sockaddr_in6 *)&address->addr;
		internal = php_plutobook_internal_ipv6(&ipv6->sin6_addr);
		inet_ntop(AF_INET6, &ipv6->sin6_addr, download->blocked_address,
		          sizeof(download->blocked_address));
	} else {
		internal = true;
	}
	if (purpose != CURLSOCKTYPE_IPCXN || (internal && !PLUTOBOOK_G(allow_private_addresses))) {
		download->blocked = true;
		return CURL_SOCKET_BAD;
	}
#ifdef PHP_WIN32
	return WSASocketW(address->family, address->socktype, address->protocol, NULL, 0,
	                  WSA_FLAG_OVERLAPPED | WSA_FLAG_NO_HANDLE_INHERIT);
#else
	return socket(address->family, address->socktype | SOCK_CLOEXEC, address->protocol);
#endif
}

// Decoded body bytes count toward the limit, so compressed responses cannot exceed it either.
static size_t php_plutobook_receive(char *data, size_t size, size_t count, void *closure)
{
	php_plutobook_download *download = closure;
	const size_t length = size * count;
	if (length > download->limit - download->size) {
		download->too_large = true;
		return 0;
	}
	if (length > download->capacity - download->size) {
		size_t capacity = download->capacity ? download->capacity : 16384;
		while (capacity - download->size < length) {
			capacity = capacity > download->limit / 2 ? download->limit : capacity * 2;
		}
		char *bytes = realloc(download->bytes, capacity);
		if (!bytes) {
			return 0;
		}
		download->bytes = bytes;
		download->capacity = capacity;
	}
	memcpy(download->bytes + download->size, data, length);
	download->size += length;
	return length;
}

static int php_plutobook_progress(void *closure, curl_off_t download_total, curl_off_t downloaded,
                                  curl_off_t upload_total, curl_off_t uploaded)
{
	php_plutobook_download *download = closure;
	download->interrupted = php_plutobook_timed_out();
	return download->interrupted;
}

static int64_t php_plutobook_now_ms(void)
{
#ifdef PHP_WIN32
	return (int64_t)GetTickCount64();
#else
	struct timespec now;
	clock_gettime(CLOCK_MONOTONIC, &now);
	return (int64_t)now.tv_sec * 1000 + now.tv_nsec / 1000000;
#endif
}

// Check the scheme, the host allowlist and https downgrades for a request or redirect target.
static php_plutobook_asset_status php_plutobook_check_target(php_plutobook_document *doc,
                                                             const char *target, bool redirected,
                                                             bool *secure, char *detail,
                                                             size_t detail_size)
{
	char *scheme = NULL, *host = NULL;
	php_plutobook_asset_status status = PHP_PLUTOBOOK_ASSET_INVALID_URL;
	if (php_plutobook_url_parts(target, &scheme, &host)) {
		bool https = !strcasecmp(scheme, "https");
		if (!https && strcasecmp(scheme, "http")) {
			status = redirected ? PHP_PLUTOBOOK_ASSET_REDIRECT_ERROR
			                    : PHP_PLUTOBOOK_ASSET_UNSUPPORTED_SCHEME;
			snprintf(detail, detail_size, "%s%s: URL", redirected ? "redirect to a " : "", scheme);
		} else if (redirected && *secure && !https) {
			status = PHP_PLUTOBOOK_ASSET_REDIRECT_ERROR;
			snprintf(detail, detail_size, "redirect from https to http");
		} else if (!php_plutobook_host_allowed(doc->allowed_hosts, host)) {
			status = PHP_PLUTOBOOK_ASSET_HOST_NOT_ALLOWED;
			snprintf(detail, detail_size, "%s%s is not in allowedHosts",
			         redirected ? "redirect target " : "", host);
		} else {
			status = PHP_PLUTOBOOK_ASSET_OK;
			*secure = https;
		}
	}
	curl_free(scheme);
	curl_free(host);
	return status;
}

static void php_plutobook_configure(CURL *curl, const char *target,
                                    php_plutobook_download *download, long timeout_ms, char *error)
{
	const char *ca_file = PLUTOBOOK_G(http_ca_file);
	// libcurl's compile-time option type checks expand curl_easy_setopt() into sizeof tests.
	// NOLINTBEGIN(bugprone-sizeof-expression)
	curl_easy_reset(curl);
	curl_easy_setopt(curl, CURLOPT_URL, target);
#if LIBCURL_VERSION_NUM >= 0x075500
	curl_easy_setopt(curl, CURLOPT_PROTOCOLS_STR, "http,https");
#else
	curl_easy_setopt(curl, CURLOPT_PROTOCOLS, (long)(CURLPROTO_HTTP | CURLPROTO_HTTPS));
#endif
	// Redirects are followed by the caller, which re-checks every target.
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 0L);
	// Ignore http_proxy and similar environment variables.
	curl_easy_setopt(curl, CURLOPT_PROXY, "");
	curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
	curl_easy_setopt(curl, CURLOPT_USERAGENT, PHP_PLUTOBOOK_USER_AGENT);
	curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "");
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);
	curl_easy_setopt(curl, CURLOPT_SSLVERSION, (long)CURL_SSLVERSION_TLSv1_2);
	if (ca_file && *ca_file) {
		curl_easy_setopt(curl, CURLOPT_CAINFO, ca_file);
#if defined(PHP_WIN32) && defined(CURLSSLOPT_NATIVE_CA)
	} else {
		curl_easy_setopt(curl, CURLOPT_SSL_OPTIONS, (long)CURLSSLOPT_NATIVE_CA);
#endif
	}
	curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, timeout_ms);
	curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT_MS, timeout_ms);
	curl_easy_setopt(curl, CURLOPT_MAXFILESIZE_LARGE, (curl_off_t)download->limit);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, php_plutobook_receive);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, download);
	curl_easy_setopt(curl, CURLOPT_OPENSOCKETFUNCTION, php_plutobook_open_socket);
	curl_easy_setopt(curl, CURLOPT_OPENSOCKETDATA, download);
	curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
	curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, php_plutobook_progress);
	curl_easy_setopt(curl, CURLOPT_XFERINFODATA, download);
	curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, error);
	// NOLINTEND(bugprone-sizeof-expression)
}

static php_plutobook_asset_status
php_plutobook_transfer_error(CURLcode result, const php_plutobook_download *download,
                             const char *error, char *detail, size_t detail_size)
{
	const char *message = *error ? error : curl_easy_strerror(result);
	if (download->interrupted) {
		snprintf(detail, detail_size, "PHP time limit reached");
		return PHP_PLUTOBOOK_ASSET_TIMEOUT;
	}
	if (download->blocked) {
		snprintf(detail, detail_size, "connection to internal address %s refused",
		         download->blocked_address);
		return PHP_PLUTOBOOK_ASSET_BLOCKED_ADDRESS;
	}
	snprintf(detail, detail_size, "%s", message);
	switch (result) {
	case CURLE_FILESIZE_EXCEEDED:
	case CURLE_WRITE_ERROR:
		if (!download->too_large && result != CURLE_FILESIZE_EXCEEDED) {
			return PHP_PLUTOBOOK_ASSET_READ_ERROR;
		}
		snprintf(detail, detail_size, "%s",
		         download->total_limited ? "plutobook.max_total_asset_size reached"
		                                 : "larger than plutobook.max_asset_size");
		return PHP_PLUTOBOOK_ASSET_TOO_LARGE;
	case CURLE_OPERATION_TIMEDOUT:
		if (download->document_limited) {
			snprintf(detail, detail_size, "plutobook.http_document_timeout reached");
		}
		return PHP_PLUTOBOOK_ASSET_TIMEOUT;
	case CURLE_PEER_FAILED_VERIFICATION:
	case CURLE_SSL_CONNECT_ERROR:
	case CURLE_SSL_CERTPROBLEM:
	case CURLE_SSL_CIPHER:
	case CURLE_SSL_CACERT_BADFILE:
	case CURLE_SSL_ISSUER_ERROR:
	case CURLE_SSL_INVALIDCERTSTATUS:
		return PHP_PLUTOBOOK_ASSET_TLS_ERROR;
	default:
		return PHP_PLUTOBOOK_ASSET_NETWORK_ERROR;
	}
}

// Split "type/subtype; charset=x" like PlutoBook's own fetcher does.
static void php_plutobook_content_type(const char *value, char *mime, size_t mime_size,
                                       char *charset, size_t charset_size)
{
	*mime = *charset = '\0';
	if (!value) {
		return;
	}
	value += strspn(value, " \t");
	size_t length = strcspn(value, ";");
	while (length && (value[length - 1] == ' ' || value[length - 1] == '\t')) {
		length--;
	}
	snprintf(mime, mime_size, "%.*s", (int)length, value);
	for (const char *param = strchr(value, ';'); param; param = strchr(param + 1, ';')) {
		const char *name = param + 1 + strspn(param + 1, " \t");
		if (!strncasecmp(name, "charset=", 8)) {
			const char *start = name + 8 + (name[8] == '"');
			snprintf(charset, charset_size, "%.*s", (int)strcspn(start, "\"; \t"), start);
			break;
		}
	}
}

static plutobook_resource_data_t *php_plutobook_resource(CURL *curl, const char *target,
                                                         php_plutobook_download *download)
{
	const char *type = NULL;
	char mime[128], charset[64];
	curl_easy_getinfo(curl, CURLINFO_CONTENT_TYPE, &type);
	php_plutobook_content_type(type, mime, sizeof(mime), charset, sizeof(charset));
	if (!*mime) {
		const char *fallback = php_plutobook_mime_from_path(target, strcspn(target, "?#"));
		snprintf(mime, sizeof(mime), "%s", fallback);
	}
	plutobook_resource_data_t *resource = plutobook_resource_data_create_without_copy(
	    download->bytes, (unsigned int)download->size, mime, charset, free, download->bytes);
	if (resource) {
		download->bytes = NULL;
	}
	return resource;
}

plutobook_resource_data_t *php_plutobook_fetch_remote(php_plutobook_document *doc, const char *url,
                                                      php_plutobook_asset_status *status,
                                                      char *detail, size_t detail_size)
{
	// Per-asset limits, tightened by what the document's budgets have left.
	const zend_long max_size = PLUTOBOOK_G(max_asset_size);
	const size_t asset_limit = max_size ? (size_t)max_size : INT_MAX;
	const size_t bytes_left = php_plutobook_asset_bytes_left(doc);
	const int64_t request_ms = PLUTOBOOK_G(http_timeout) * 1000;
	const zend_long document_timeout = PLUTOBOOK_G(http_document_timeout);
	const int64_t document_ms =
	    document_timeout ? document_timeout * 1000 - doc->network_ms : INT64_MAX;
	if (!php_plutobook_curl_supported) {
		snprintf(detail, detail_size, "%s", php_plutobook_curl_unsupported);
		*status = PHP_PLUTOBOOK_ASSET_NETWORK_ERROR;
		return NULL;
	}
	if (!bytes_left) {
		snprintf(detail, detail_size, "plutobook.max_total_asset_size reached");
		*status = PHP_PLUTOBOOK_ASSET_TOO_LARGE;
		return NULL;
	}
	if (document_ms <= 0) {
		snprintf(detail, detail_size, "plutobook.http_document_timeout reached");
		*status = PHP_PLUTOBOOK_ASSET_TIMEOUT;
		return NULL;
	}
	if (!doc->http) {
		doc->http = curl_easy_init();
	}
	char *target = strdup(url);
	if (!doc->http || !target) {
		free(target);
		snprintf(detail, detail_size, "could not start a libcurl transfer");
		*status = PHP_PLUTOBOOK_ASSET_NETWORK_ERROR;
		return NULL;
	}
	php_plutobook_download download = {
	    .limit = bytes_left < asset_limit ? bytes_left : asset_limit,
	    .total_limited = bytes_left < asset_limit,
	    .document_limited = document_ms < request_ms,
	};
	const int64_t started = php_plutobook_now_ms();
	const int64_t deadline = started + (download.document_limited ? document_ms : request_ms);
	plutobook_resource_data_t *resource = NULL;
	bool secure = false;
	for (zend_long hop = 0;; hop++) {
		*status = php_plutobook_check_target(doc, target, hop > 0, &secure, detail, detail_size);
		const int64_t remaining = deadline - php_plutobook_now_ms();
		if (*status == PHP_PLUTOBOOK_ASSET_OK && remaining <= 0) {
			snprintf(detail, detail_size, "%s",
			         download.document_limited ? "plutobook.http_document_timeout reached"
			                                   : "plutobook.http_timeout reached");
			*status = PHP_PLUTOBOOK_ASSET_TIMEOUT;
		}
		if (*status != PHP_PLUTOBOOK_ASSET_OK) {
			break;
		}
		char error[CURL_ERROR_SIZE] = "";
		download.size = 0;
		download.too_large = download.blocked = download.interrupted = false;
		php_plutobook_configure(doc->http, target, &download, (long)remaining, error);
		CURLcode result = curl_easy_perform(doc->http);
		if (result != CURLE_OK) {
			*status = php_plutobook_transfer_error(result, &download, error, detail, detail_size);
			curl_off_t lookup_us = 0;
			if (*status == PHP_PLUTOBOOK_ASSET_TIMEOUT && !php_plutobook_async_dns &&
			    curl_easy_getinfo(doc->http, CURLINFO_NAMELOOKUP_TIME_T, &lookup_us) == CURLE_OK &&
			    lookup_us / 1000 >= remaining) {
				snprintf(detail, detail_size,
				         "the DNS lookup took %" CURL_FORMAT_CURL_OFF_T
				         " ms, and this libcurl cannot stop lookups early",
				         lookup_us / 1000);
			}
			break;
		}
		long code = 0;
		char *location = NULL;
		curl_easy_getinfo(doc->http, CURLINFO_RESPONSE_CODE, &code);
		curl_easy_getinfo(doc->http, CURLINFO_REDIRECT_URL, &location);
		if (code >= 300 && code < 400 && location) {
			if (hop >= PLUTOBOOK_G(http_max_redirects)) {
				snprintf(detail, detail_size, "more than " ZEND_LONG_FMT " redirects",
				         PLUTOBOOK_G(http_max_redirects));
				*status = PHP_PLUTOBOOK_ASSET_REDIRECT_ERROR;
				break;
			}
			char *next = strdup(location);
			free(target);
			target = next;
			if (!target) {
				*status = PHP_PLUTOBOOK_ASSET_READ_ERROR;
				break;
			}
			continue;
		}
		if (code < 200 || code >= 300) {
			snprintf(detail, detail_size, "HTTP %ld", code);
			*status = PHP_PLUTOBOOK_ASSET_HTTP_ERROR;
		} else if (!download.size) {
			*status = PHP_PLUTOBOOK_ASSET_EMPTY;
		} else if (!(resource = php_plutobook_resource(doc->http, target, &download))) {
			*status = PHP_PLUTOBOOK_ASSET_READ_ERROR;
		}
		break;
	}
	doc->network_ms += php_plutobook_now_ms() - started;
	if (resource) {
		doc->asset_bytes += download.size;
	}
	free(download.bytes);
	free(target);
	return resource;
}
