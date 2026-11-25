// SPDX-License-Identifier: Apache-2.0
#include "binding.h"
#include "Zend/zend_smart_str.h"
#include "ext/standard/url.h"
#include "main/fopen_wrappers.h"
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

// Reason codes are public API through Document::getResourceErrors(); keep them stable.
static const struct {
	const char *code;
	const char *description;
} php_plutobook_asset_reasons[] = {
    [PHP_PLUTOBOOK_ASSET_OK] = {"", ""},
    [PHP_PLUTOBOOK_ASSET_NETWORK_DISABLED] = {"network-disabled",
                                              "network access needs allowedHosts"},
    [PHP_PLUTOBOOK_ASSET_UNSUPPORTED_SCHEME] = {"unsupported-scheme", "unsupported URL scheme"},
    [PHP_PLUTOBOOK_ASSET_NO_BASE_URL] = {"no-base-url",
                                         "local files need a baseUrl or documentRoot directory"},
    [PHP_PLUTOBOOK_ASSET_INVALID_URL] = {"invalid-url", "malformed URL"},
    [PHP_PLUTOBOOK_ASSET_MISSING] = {"missing", "file not found"},
    [PHP_PLUTOBOOK_ASSET_OPEN_BASEDIR] = {"open-basedir", "blocked by open_basedir"},
    [PHP_PLUTOBOOK_ASSET_OUTSIDE_BASE] = {"outside-base", "outside the asset directory"},
    [PHP_PLUTOBOOK_ASSET_VERIFICATION_FAILED] = {"verification-failed",
                                                 "path changed during the checks"},
    [PHP_PLUTOBOOK_ASSET_NOT_REGULAR_FILE] = {"not-regular-file", "not a regular file"},
    [PHP_PLUTOBOOK_ASSET_EMPTY] = {"empty", "empty file"},
    [PHP_PLUTOBOOK_ASSET_TOO_LARGE] = {"too-large", "larger than plutobook.max_asset_size"},
    [PHP_PLUTOBOOK_ASSET_PERMISSION_DENIED] = {"permission-denied", "not readable by PHP"},
    [PHP_PLUTOBOOK_ASSET_READ_ERROR] = {"read-error", "read failed"},
    [PHP_PLUTOBOOK_ASSET_HOST_NOT_ALLOWED] = {"host-not-allowed", "host not in allowedHosts"},
    [PHP_PLUTOBOOK_ASSET_BLOCKED_ADDRESS] = {"blocked-address", "internal network address"},
    [PHP_PLUTOBOOK_ASSET_HTTP_ERROR] = {"http-error", "HTTP error status"},
    [PHP_PLUTOBOOK_ASSET_REDIRECT_ERROR] = {"redirect-error", "redirect refused"},
    [PHP_PLUTOBOOK_ASSET_TLS_ERROR] = {"tls-error", "TLS or certificate error"},
    [PHP_PLUTOBOOK_ASSET_TIMEOUT] = {"timeout", "request timed out"},
    [PHP_PLUTOBOOK_ASSET_NETWORK_ERROR] = {"network-error", "network error"},
};

// The extension table PlutoBook's own loaders use. Only SVG needs a type to be decoded;
// bitmaps are recognised by content and text resources ignore it.
const char *php_plutobook_mime_from_path(const char *path, size_t length)
{
	static const struct {
		const char *extension;
		const char *mime;
	} types[] = {{"xhtml", "application/xhtml+xml"},
	             {"html", "text/html"},
	             {"htm", "text/html"},
	             {"txt", "text/plain"},
	             {"css", "text/css"},
	             {"xml", "text/xml"},
	             {"jpeg", "image/jpeg"},
	             {"jpg", "image/jpeg"},
	             {"png", "image/png"},
	             {"webp", "image/webp"},
	             {"svg", "image/svg+xml"},
	             {"gif", "image/gif"},
	             {"bmp", "image/bmp"}};
	const char *dot = NULL;
	for (size_t i = length; i-- > 0 && path[i] != '/';) {
		if (path[i] == '.') {
			dot = path + i + 1;
			break;
		}
	}
	for (size_t i = 0; dot && i < sizeof(types) / sizeof(types[0]); i++) {
		size_t size = strlen(types[i].extension);
		if ((size_t)(path + length - dot) == size && !strncasecmp(dot, types[i].extension, size)) {
			return types[i].mime;
		}
	}
	return "";
}

static php_plutobook_asset_status php_plutobook_decode_file_url(const char *url,
                                                                char decoded[MAXPATHLEN])
{
	if (strncmp(url, "file:///", 8) != 0) {
		return PHP_PLUTOBOOK_ASSET_INVALID_URL;
	}
	const char *path = url + sizeof("file://") - 1;
	size_t length = strcspn(path, "?#");
	if (length >= MAXPATHLEN) {
		return PHP_PLUTOBOOK_ASSET_INVALID_URL;
	}
	memcpy(decoded, path, length);
	decoded[length] = '\0';
	size_t decoded_length = php_raw_url_decode(decoded, length);
	return strlen(decoded) == decoded_length ? PHP_PLUTOBOOK_ASSET_OK
	                                         : PHP_PLUTOBOOK_ASSET_INVALID_URL;
}

// Only local file URLs reach the filesystem. Inline data URLs are decoded by PlutoBook.
// With a document root, the URL path is joined below it before resolution; any ".."
// that escapes the root is caught afterwards by the confinement check on the real path.
static php_plutobook_asset_status php_plutobook_resolve_file(const char *url, const char *root,
                                                             char resolved[MAXPATHLEN])
{
	char decoded[MAXPATHLEN], joined[MAXPATHLEN];
	php_plutobook_asset_status status = php_plutobook_decode_file_url(url, decoded);
	if (status != PHP_PLUTOBOOK_ASSET_OK) {
		return status;
	}
	const char *path = decoded;
	if (root) {
		int length =
		    snprintf(joined, sizeof(joined), "%s%s", strcmp(root, "/") ? root : "", decoded);
		if (length < 0 || (size_t)length >= sizeof(joined)) {
			return PHP_PLUTOBOOK_ASSET_INVALID_URL;
		}
		path = joined;
	}
	if (VCWD_REALPATH(path, resolved) == NULL) {
		return PHP_PLUTOBOOK_ASSET_MISSING;
	}
	return php_check_open_basedir_ex(resolved, 0) == 0 ? PHP_PLUTOBOOK_ASSET_OK
	                                                   : PHP_PLUTOBOOK_ASSET_OPEN_BASEDIR;
}

static php_plutobook_asset_status php_plutobook_open_error(int error)
{
	switch (error) {
	case ENOENT:
		return PHP_PLUTOBOOK_ASSET_MISSING;
	case EACCES:
	case EPERM:
		return PHP_PLUTOBOOK_ASSET_PERMISSION_DENIED;
	case ELOOP:
	case ENOTDIR:
	case ENXIO:
		return PHP_PLUTOBOOK_ASSET_VERIFICATION_FAILED;
	default:
		return PHP_PLUTOBOOK_ASSET_READ_ERROR;
	}
}

// Open the directory that holds the last component of a canonical relative path, one
// component at a time with O_NOFOLLOW. realpath() already resolved every symlink, so a
// symlink met here was swapped in after the realpath() and open_basedir checks and the
// walk fails. Unlike reading /proc/self/fd links, this works without /proc (FPM chroot).
// On success *leaf points into path and the caller owns the returned descriptor.
static int php_plutobook_open_parent(int root, const char *path, const char **leaf)
{
	int current = fcntl(root, F_DUPFD_CLOEXEC, 0);
	const char *slash;
	while (current >= 0 && (slash = strchr(path, '/')) != NULL) {
		char name[MAXPATHLEN];
		size_t length = (size_t)(slash - path);
		int next = -1;
		if (length && length < sizeof(name)) {
			memcpy(name, path, length);
			name[length] = '\0';
			next = openat(current, name, O_PATH | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC);
		} else {
			errno = ENOENT;
		}
		int error = errno;
		close(current);
		errno = error;
		current = next;
		path = slash + 1;
	}
	*leaf = path;
	return current;
}

// Pin a canonical absolute directory path without following symlinks.
static int php_plutobook_open_directory(const char *resolved)
{
	int root = open("/", O_PATH | O_DIRECTORY | O_CLOEXEC);
	if (root < 0 || !strcmp(resolved, "/")) {
		return root;
	}
	const char *leaf = NULL;
	int parent = php_plutobook_open_parent(root, resolved + 1, &leaf);
	close(root);
	if (parent < 0) {
		return -1;
	}
	int directory = openat(parent, leaf, O_PATH | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC);
	close(parent);
	return directory;
}

zend_string *php_plutobook_asset_root(zend_string *base, int *directory, uint32_t arg)
{
	const char *url = ZSTR_VAL(base);
	char resolved[MAXPATHLEN];
	if (ZSTR_LEN(base) && strlen(url) == ZSTR_LEN(base) && !strpbrk(url, "?#") &&
	    url[ZSTR_LEN(base) - 1] == '/' &&
	    php_plutobook_resolve_file(url, NULL, resolved) == PHP_PLUTOBOOK_ASSET_OK) {
		*directory = php_plutobook_open_directory(resolved);
		if (*directory >= 0) {
			return zend_string_init(resolved, strlen(resolved), 0);
		}
	}
	zend_argument_value_error(
	    arg, "must be a file:/// URL ending in / for an accessible local directory");
	return NULL;
}

// The base URL PlutoBook sees in documentRoot mode: the base directory's path below the
// root, percent-encoded per segment, as a file URL whose "/" is the document root.
zend_string *php_plutobook_virtual_base(zend_string *root, zend_string *base)
{
	if (!base) {
		return zend_string_init("file:///", sizeof("file:///") - 1, 0);
	}
	int directory = -1;
	zend_string *resolved = php_plutobook_asset_root(base, &directory, 2);
	if (!resolved) {
		return NULL;
	}
	close(directory);
	const size_t root_len = ZSTR_LEN(root) == 1 ? 0 : ZSTR_LEN(root);
	const char *relative = ZSTR_VAL(resolved) + root_len;
	if (strncmp(ZSTR_VAL(resolved), ZSTR_VAL(root), root_len) || (*relative && *relative != '/')) {
		zend_string_release(resolved);
		zend_argument_value_error(2, "must be a directory inside documentRoot");
		return NULL;
	}
	smart_str url = {0};
	smart_str_appends(&url, "file:///");
	while (*relative) {
		size_t length = strcspn(++relative, "/");
		if (length) {
			zend_string *segment = php_raw_url_encode(relative, length);
			smart_str_append(&url, segment);
			smart_str_appendc(&url, '/');
			zend_string_release(segment);
		}
		relative += length;
	}
	zend_string_release(resolved);
	return smart_str_extract(&url);
}

static php_plutobook_asset_status php_plutobook_scheme(const char *url)
{
	if (!strncasecmp(url, "file:", 5)) {
		return PHP_PLUTOBOOK_ASSET_OK;
	}
	if (!strncasecmp(url, "http:", 5) || !strncasecmp(url, "https:", 6)) {
		return PHP_PLUTOBOOK_ASSET_NETWORK_DISABLED;
	}
	return PHP_PLUTOBOOK_ASSET_UNSUPPORTED_SCHEME;
}

static php_plutobook_asset_status php_plutobook_check_file(const struct stat *st)
{
	if (!S_ISREG(st->st_mode)) {
		return PHP_PLUTOBOOK_ASSET_NOT_REGULAR_FILE;
	}
	if (st->st_size == 0) {
		return PHP_PLUTOBOOK_ASSET_EMPTY;
	}
	// Asset bytes are native allocations outside memory_limit, so enforce the cap before reading.
	const zend_long limit = PLUTOBOOK_G(max_asset_size);
	if (st->st_size > INT_MAX || (limit && st->st_size > limit)) {
		return PHP_PLUTOBOOK_ASSET_TOO_LARGE;
	}
	return PHP_PLUTOBOOK_ASSET_OK;
}

// Inspect the entry through O_PATH first, which never opens a device, FIFO or socket for I/O.
// Then reopen it by name for reading. O_NONBLOCK keeps a FIFO swapped in meanwhile from
// blocking, and the device/inode comparison rejects any replacement.
static php_plutobook_asset_status php_plutobook_open_leaf(int parent, const char *leaf,
                                                          size_t *size, int *reader)
{
	int pinned = openat(parent, leaf, O_PATH | O_NOFOLLOW | O_CLOEXEC);
	if (pinned < 0) {
		return php_plutobook_open_error(errno);
	}
	struct stat expected, actual;
	php_plutobook_asset_status status = fstat(pinned, &expected) == 0
	                                        ? php_plutobook_check_file(&expected)
	                                        : PHP_PLUTOBOOK_ASSET_READ_ERROR;
	close(pinned);
	if (status != PHP_PLUTOBOOK_ASSET_OK) {
		return status;
	}
	*reader = openat(parent, leaf, O_RDONLY | O_NONBLOCK | O_NOFOLLOW | O_NOCTTY | O_CLOEXEC);
	if (*reader < 0) {
		return php_plutobook_open_error(errno);
	}
	if (fstat(*reader, &actual) != 0 || actual.st_dev != expected.st_dev ||
	    actual.st_ino != expected.st_ino) {
		status = PHP_PLUTOBOOK_ASSET_VERIFICATION_FAILED;
	} else {
		status = php_plutobook_check_file(&actual);
	}
	if (status != PHP_PLUTOBOOK_ASSET_OK) {
		close(*reader);
		*reader = -1;
		return status;
	}
	*size = (size_t)actual.st_size;
	return PHP_PLUTOBOOK_ASSET_OK;
}

static plutobook_resource_data_t *php_plutobook_read_asset(php_plutobook_document *doc,
                                                           const char *url,
                                                           php_plutobook_asset_status *status,
                                                           char *detail, size_t detail_size)
{
	char path[MAXPATHLEN];
	*status = php_plutobook_scheme(url);
	if (*status == PHP_PLUTOBOOK_ASSET_NETWORK_DISABLED && doc->allowed_hosts) {
		return php_plutobook_fetch_remote(doc, url, status, detail, detail_size);
	}
	if (*status != PHP_PLUTOBOOK_ASSET_OK) {
		return NULL;
	}
	if (!doc->asset_root) {
		*status = PHP_PLUTOBOOK_ASSET_NO_BASE_URL;
		return NULL;
	}
	*status =
	    php_plutobook_resolve_file(url, doc->virtual_root ? ZSTR_VAL(doc->asset_root) : NULL, path);
	if (*status != PHP_PLUTOBOOK_ASSET_OK) {
		return NULL;
	}
	const size_t root_len = ZSTR_LEN(doc->asset_root);
	if (strncmp(path, ZSTR_VAL(doc->asset_root), root_len) ||
	    (root_len != 1 && path[root_len] != '/')) {
		*status = PHP_PLUTOBOOK_ASSET_OUTSIDE_BASE;
		return NULL;
	}
	const char *relative = path + root_len;
	if (*relative == '/') {
		relative++;
	}
	const char *leaf = NULL;
	int parent = php_plutobook_open_parent(doc->asset_dir, relative, &leaf);
	if (parent < 0) {
		*status = php_plutobook_open_error(errno);
		return NULL;
	}
	size_t size = 0;
	int reader = -1;
	*status = php_plutobook_open_leaf(parent, leaf, &size, &reader);
	close(parent);
	if (*status != PHP_PLUTOBOOK_ASSET_OK) {
		return NULL;
	}
	if (size > php_plutobook_asset_bytes_left(doc)) {
		close(reader);
		snprintf(detail, detail_size, "plutobook.max_total_asset_size reached");
		*status = PHP_PLUTOBOOK_ASSET_TOO_LARGE;
		return NULL;
	}
	// Give PlutoBook ownership of one native buffer. Reading the verified fd
	// keeps descriptor cleanup independent of Zend.
	char *bytes = malloc(size);
	size_t offset = 0;
	while (bytes && offset < size) {
		ssize_t n = read(reader, bytes + offset, size - offset);
		if (n < 0 && errno == EINTR) {
			continue;
		}
		if (n <= 0) {
			break;
		}
		offset += (size_t)n;
	}
	close(reader);
	plutobook_resource_data_t *result = NULL;
	if (offset == size) {
		const char *mime = php_plutobook_mime_from_path(path, strlen(path));
		result = plutobook_resource_data_create_without_copy(bytes, (unsigned int)size, mime, "",
		                                                     free, bytes);
	}
	if (!result) {
		free(bytes);
		*status = PHP_PLUTOBOOK_ASSET_READ_ERROR;
	} else {
		doc->asset_bytes += size;
	}
	return result;
}

static void php_plutobook_record_skip(php_plutobook_document *doc, const char *url,
                                      php_plutobook_asset_status status, const char *detail)
{
	if (!doc->resource_errors) {
		doc->resource_errors = zend_new_array(0);
	}
	zval entry;
	array_init_size(&entry, 3);
	add_assoc_string(&entry, "url", url);
	add_assoc_string(&entry, "reason", php_plutobook_asset_reasons[status].code);
	add_assoc_string(&entry, "detail",
	                 *detail ? detail : php_plutobook_asset_reasons[status].description);
	zend_hash_next_index_insert(doc->resource_errors, &entry);
}

plutobook_resource_data_t *php_plutobook_fetch_asset(void *closure, const char *url)
{
	php_plutobook_document *doc = closure;
	// Volatile: both are written inside zend_try and read after a possible longjmp.
	plutobook_resource_data_t *volatile resource = NULL;
	volatile php_plutobook_asset_status status = PHP_PLUTOBOOK_ASSET_READ_ERROR;
	char detail[512] = "";
	// After a timeout, skip remaining fetches so the renderer finishes quickly.
	if (!doc->bailed && !EG(exception) && !php_plutobook_timed_out()) {
		zend_try
		{
			php_plutobook_asset_status found = PHP_PLUTOBOOK_ASSET_READ_ERROR;
			plutobook_resource_data_t *loaded =
			    php_plutobook_read_asset(doc, url, &found, detail, sizeof(detail));
			if (!loaded) {
				php_plutobook_record_skip(doc, url, found, detail);
			}
			resource = loaded;
			status = found;
		}
		zend_catch
		{
			doc->bailed = true;
		}
		zend_end_try();
	}
	if (!resource) {
		// PlutoBook prints this message to stderr; the document keeps the structured record.
		const char *description = php_plutobook_asset_reasons[status].description;
		plutobook_set_error_message("%s, %s%s%s%s: %s", PHP_PLUTOBOOK_ASSET_SKIPPED, description,
		                            *detail ? " (" : "", detail, *detail ? ")" : "", url);
	}
	return resource;
}
