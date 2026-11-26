// SPDX-License-Identifier: Apache-2.0
#ifndef PHP_PLUTOBOOK_BINDING_H
#define PHP_PLUTOBOOK_BINDING_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "php.h"
#include <plutobook.h>

#if __has_include("../release-version.h")
#include "../release-version.h"
#else
#define PHP_PLUTOBOOK_VERSION "0.1.0-dev"
#endif

// Expanded ZEND_BEGIN/END_MODULE_GLOBALS, spelled out so clang-format can indent the members.
typedef struct _zend_plutobook_globals {
	zend_long max_asset_size;
	zend_long max_total_asset_size;
	zend_long http_timeout;
	zend_long http_document_timeout;
	zend_long http_max_redirects;
	char *http_ca_file;
	bool allow_private_addresses;
} zend_plutobook_globals;

ZEND_EXTERN_MODULE_GLOBALS(plutobook)
#define PLUTOBOOK_G(v) ZEND_MODULE_GLOBALS_ACCESSOR(plutobook, v)

// Prefix of the renderer message the fetcher sets for every skipped resource.
#define PHP_PLUTOBOOK_ASSET_SKIPPED "Asset skipped"

// Why a resource was skipped. The matching reason codes in assets.c are public API through
// Document::getResourceErrors().
typedef enum {
	PHP_PLUTOBOOK_ASSET_OK,
	PHP_PLUTOBOOK_ASSET_NETWORK_DISABLED,
	PHP_PLUTOBOOK_ASSET_UNSUPPORTED_SCHEME,
	PHP_PLUTOBOOK_ASSET_NO_BASE_URL,
	PHP_PLUTOBOOK_ASSET_INVALID_URL,
	PHP_PLUTOBOOK_ASSET_MISSING,
	PHP_PLUTOBOOK_ASSET_OPEN_BASEDIR,
	PHP_PLUTOBOOK_ASSET_OUTSIDE_BASE,
	PHP_PLUTOBOOK_ASSET_VERIFICATION_FAILED,
	PHP_PLUTOBOOK_ASSET_NOT_REGULAR_FILE,
	PHP_PLUTOBOOK_ASSET_EMPTY,
	PHP_PLUTOBOOK_ASSET_TOO_LARGE,
	PHP_PLUTOBOOK_ASSET_PERMISSION_DENIED,
	PHP_PLUTOBOOK_ASSET_READ_ERROR,
	PHP_PLUTOBOOK_ASSET_HOST_NOT_ALLOWED,
	PHP_PLUTOBOOK_ASSET_BLOCKED_ADDRESS,
	PHP_PLUTOBOOK_ASSET_HTTP_ERROR,
	PHP_PLUTOBOOK_ASSET_REDIRECT_ERROR,
	PHP_PLUTOBOOK_ASSET_TLS_ERROR,
	PHP_PLUTOBOOK_ASSET_TIMEOUT,
	PHP_PLUTOBOOK_ASSET_NETWORK_ERROR,
} php_plutobook_asset_status;

#ifdef PHP_WIN32
typedef HANDLE php_plutobook_directory;
#define PHP_PLUTOBOOK_INVALID_DIRECTORY INVALID_HANDLE_VALUE
#else
typedef int php_plutobook_directory;
#define PHP_PLUTOBOOK_INVALID_DIRECTORY (-1)
#endif

typedef struct {
	plutobook_t *book;
	zend_string *asset_root;
	HashTable *resource_errors;
	// Lower-case host patterns from allowedHosts. NULL keeps network access off.
	HashTable *allowed_hosts;
	// The document's libcurl easy handle, created on first use so connections are reused.
	void *http;
	// Budgets since the last loadHtml(): asset bytes handed to the renderer, network time spent.
	size_t asset_bytes;
	int64_t network_ms;
	php_plutobook_directory asset_dir;
	bool loaded;
	bool busy;
	bool bailed;
	// With documentRoot, every file: URL path is relative to asset_root, like a web server's.
	bool virtual_root;
	zend_object std;
} php_plutobook_document;

// Native rendering runs between VM interrupt checks. On Windows the timeout flag can
// stay set after the VM has delivered the fatal error (in particular on ZTS builds).
static inline bool php_plutobook_timed_out(void)
{
#ifdef PHP_WIN32
	return zend_atomic_bool_load_ex(&EG(vm_interrupt)) && zend_atomic_bool_load_ex(&EG(timed_out));
#else
	return zend_atomic_bool_load_ex(&EG(timed_out));
#endif
}

static inline void php_plutobook_raise_timeout(void)
{
#ifdef PHP_WIN32
	// Consume the pending interrupt just as the VM does before invoking zend_timeout().
	zend_atomic_bool_store_ex(&EG(vm_interrupt), false);
#endif
	zend_timeout();
}

// Bytes the document may still load under plutobook.max_total_asset_size; 0 removes the cap.
static inline size_t php_plutobook_asset_bytes_left(const php_plutobook_document *doc)
{
	const zend_long total = PLUTOBOOK_G(max_total_asset_size);
	if (!total) {
		return SIZE_MAX;
	}
	return doc->asset_bytes < (size_t)total ? (size_t)total - doc->asset_bytes : 0;
}

bool php_plutobook_page_options(zend_string *paper, zend_string *orientation, zend_string *margins,
                                plutobook_page_size_t *size, plutobook_page_margins_t *insets);
void php_plutobook_close_directory(php_plutobook_directory directory);
zend_string *php_plutobook_asset_root(zend_string *base, php_plutobook_directory *directory,
                                      uint32_t arg);
const char *php_plutobook_relative_path(const zend_string *root, const char *path);
zend_string *php_plutobook_virtual_base(zend_string *root, zend_string *base);
const char *php_plutobook_mime_from_path(const char *path, size_t length);
plutobook_resource_data_t *php_plutobook_fetch_asset(void *closure, const char *url);

bool php_plutobook_network_startup(void);
void php_plutobook_network_shutdown(void);
const char *php_plutobook_network_version(void);
void php_plutobook_network_release(php_plutobook_document *doc);
bool php_plutobook_is_remote_url(const zend_string *url);
bool php_plutobook_check_remote_base(zend_string *base, uint32_t arg);
HashTable *php_plutobook_parse_allowed_hosts(HashTable *hosts, uint32_t arg, bool *failed);
plutobook_resource_data_t *php_plutobook_fetch_remote(php_plutobook_document *doc, const char *url,
                                                      php_plutobook_asset_status *status,
                                                      char *detail, size_t detail_size);

#endif
