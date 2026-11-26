// SPDX-License-Identifier: Apache-2.0
#include "binding.h"
#include "Zend/zend_exceptions.h"
#include "Zend/zend_smart_str.h"
#include "ext/spl/spl_exceptions.h"
#include "ext/standard/info.h"
#include "main/fopen_wrappers.h"
#include "../plutobook_arginfo.h"
#include <errno.h>

#ifdef PHP_WIN32
#include "main/SAPI.h"
#elif defined(ZTS)
#error "Unix builds require NTS PHP"
#endif

ZEND_DECLARE_MODULE_GLOBALS(plutobook)

static zend_class_entry *php_plutobook_exception;
static zend_object_handlers php_plutobook_handlers;

// Reject malformed sizes. PHP's legacy fallback of 0 would silently remove the cap.
static bool php_plutobook_ini_size(const zend_ini_entry *entry, zend_string *value, zend_long max,
                                   const char *expected, zend_long *result)
{
	zend_string *error = NULL;
	zend_long bytes = zend_ini_parse_quantity(value, &error);
	bool invalid = error != NULL;
	if (error) {
		zend_string_release(error);
	}
	if (invalid || bytes < 0 || bytes > max || !strpbrk(ZSTR_VAL(value), "0123456789")) {
		zend_error(E_WARNING, "Invalid \"%s\" setting. Expected %s", ZSTR_VAL(entry->name),
		           expected);
		return false;
	}
	*result = bytes;
	return true;
}

static ZEND_INI_MH(php_plutobook_update_max_asset_size)
{
	return php_plutobook_ini_size(entry, new_value, INT_MAX,
	                              "0 or a byte size up to 2147483647, such as 32M",
	                              &PLUTOBOOK_G(max_asset_size))
	           ? SUCCESS
	           : FAILURE;
}

static ZEND_INI_MH(php_plutobook_update_max_total_asset_size)
{
	return php_plutobook_ini_size(entry, new_value, ZEND_LONG_MAX, "0 or a byte size such as 128M",
	                              &PLUTOBOOK_G(max_total_asset_size))
	           ? SUCCESS
	           : FAILURE;
}

static bool php_plutobook_ini_integer(const zend_ini_entry *entry, const zend_string *value,
                                      zend_long min, zend_long max, zend_long *result)
{
	const char *text = ZSTR_VAL(value);
	char *end = NULL;
	errno = 0;
	zend_long number = ZEND_STRTOL(text, &end, 10);
	if (end == text || *end || errno || number < min || number > max) {
		zend_error(E_WARNING,
		           "Invalid \"%s\" setting. Expected an integer from " ZEND_LONG_FMT
		           " to " ZEND_LONG_FMT,
		           ZSTR_VAL(entry->name), min, max);
		return false;
	}
	*result = number;
	return true;
}

static ZEND_INI_MH(php_plutobook_update_http_timeout)
{
	return php_plutobook_ini_integer(entry, new_value, 1, 3600, &PLUTOBOOK_G(http_timeout))
	           ? SUCCESS
	           : FAILURE;
}

static ZEND_INI_MH(php_plutobook_update_http_document_timeout)
{
	return php_plutobook_ini_integer(entry, new_value, 0, 86400,
	                                 &PLUTOBOOK_G(http_document_timeout))
	           ? SUCCESS
	           : FAILURE;
}

static ZEND_INI_MH(php_plutobook_update_http_max_redirects)
{
	return php_plutobook_ini_integer(entry, new_value, 0, 20, &PLUTOBOOK_G(http_max_redirects))
	           ? SUCCESS
	           : FAILURE;
}

PHP_INI_BEGIN()
PHP_INI_ENTRY("plutobook.max_asset_size", "32M", PHP_INI_ALL, php_plutobook_update_max_asset_size)
PHP_INI_ENTRY("plutobook.max_total_asset_size", "128M", PHP_INI_ALL,
              php_plutobook_update_max_total_asset_size)
PHP_INI_ENTRY("plutobook.http_timeout", "10", PHP_INI_ALL, php_plutobook_update_http_timeout)
PHP_INI_ENTRY("plutobook.http_document_timeout", "30", PHP_INI_ALL,
              php_plutobook_update_http_document_timeout)
PHP_INI_ENTRY("plutobook.http_max_redirects", "5", PHP_INI_ALL,
              php_plutobook_update_http_max_redirects)
STD_PHP_INI_ENTRY("plutobook.http_ca_file", "", PHP_INI_ALL, OnUpdateString, http_ca_file,
                  zend_plutobook_globals, plutobook_globals)
// Reaching internal services is an operator decision, so scripts cannot turn it on.
STD_PHP_INI_BOOLEAN("plutobook.allow_private_addresses", "0", PHP_INI_SYSTEM | PHP_INI_PERDIR,
                    OnUpdateBool, allow_private_addresses, zend_plutobook_globals,
                    plutobook_globals)
PHP_INI_END()

static php_plutobook_document *php_plutobook_document_from(zend_object *object)
{
	return (php_plutobook_document *)((char *)object - offsetof(php_plutobook_document, std));
}

static zend_object *php_plutobook_create_object(zend_class_entry *ce)
{
	php_plutobook_document *doc = zend_object_alloc(sizeof(*doc), ce);
	zend_object_std_init(&doc->std, ce);
	object_properties_init(&doc->std, ce);
	doc->std.handlers = &php_plutobook_handlers;
	doc->asset_dir = PHP_PLUTOBOOK_INVALID_DIRECTORY;
	return &doc->std;
}

static void php_plutobook_free_object(zend_object *object)
{
	php_plutobook_document *doc = php_plutobook_document_from(object);
	if (doc->book) {
		plutobook_destroy(doc->book);
	}
	if (doc->asset_root) {
		php_plutobook_close_directory(doc->asset_dir);
		zend_string_release(doc->asset_root);
	}
	if (doc->resource_errors) {
		zend_array_destroy(doc->resource_errors);
	}
	if (doc->allowed_hosts) {
		zend_array_destroy(doc->allowed_hosts);
	}
	php_plutobook_network_release(doc);
	zend_object_std_dtor(object);
}

static void php_plutobook_fail(const char *message)
{
	if (!EG(exception)) {
		zend_throw_exception(php_plutobook_exception, message, 0);
	}
}

static void php_plutobook_fail_because(const char *message, const char *detail)
{
	if (!detail || !*detail) {
		php_plutobook_fail(message);
	} else if (!EG(exception)) {
		zend_throw_exception_ex(php_plutobook_exception, 0, "%s: %s", message, detail);
	}
}

// The renderer's last message, unless it is empty or a skipped-asset notice from the fetcher.
// Those are reported by getResourceErrors() and would misattribute an unrelated failure.
static const char *php_plutobook_renderer_detail(void)
{
	const char *message = plutobook_get_error_message();
	if (!message ||
	    !strncmp(message, PHP_PLUTOBOOK_ASSET_SKIPPED, sizeof(PHP_PLUTOBOOK_ASSET_SKIPPED) - 1)) {
		return NULL;
	}
	return message;
}

static bool php_plutobook_ready(php_plutobook_document *doc, bool needs_content)
{
	if (doc->busy) {
		php_plutobook_fail("The document is already rendering");
		return false;
	}
	if (!doc->book || (needs_content && !doc->loaded)) {
		php_plutobook_fail("Initialize the document and load HTML before rendering");
		return false;
	}
	return true;
}

PHP_METHOD(PlutoBook_Document, __construct)
{
	zend_string *paper = NULL, *orientation = NULL, *margins = NULL;
	ZEND_PARSE_PARAMETERS_START(0, 3)
	Z_PARAM_OPTIONAL
	Z_PARAM_STR(paper)
	Z_PARAM_STR(orientation)
	Z_PARAM_STR(margins)
	ZEND_PARSE_PARAMETERS_END();

	php_plutobook_document *doc = php_plutobook_document_from(Z_OBJ_P(ZEND_THIS));
	if (doc->book) {
		php_plutobook_fail("The document is already initialized");
		RETURN_THROWS();
	}
	plutobook_page_size_t size;
	plutobook_page_margins_t insets;
	if (!php_plutobook_page_options(paper, orientation, margins, &size, &insets)) {
		RETURN_THROWS();
	}
	doc->book = plutobook_create(size, insets, PLUTOBOOK_MEDIA_TYPE_PRINT);
	if (!doc->book) {
		php_plutobook_fail("Unable to create a PlutoBook document");
		RETURN_THROWS();
	}
	plutobook_set_custom_resource_fetcher(doc->book, php_plutobook_fetch_asset, doc);
}

PHP_METHOD(PlutoBook_Document, loadHtml)
{
	zend_string *html, *base = NULL, *css = NULL, *document_root = NULL;
	HashTable *hosts = NULL;
	ZEND_PARSE_PARAMETERS_START(1, 5)
	Z_PARAM_STR(html)
	Z_PARAM_OPTIONAL
	Z_PARAM_STR_OR_NULL(base)
	Z_PARAM_STR(css)
	Z_PARAM_STR_OR_NULL(document_root)
	Z_PARAM_ARRAY_HT(hosts)
	ZEND_PARSE_PARAMETERS_END();

	php_plutobook_document *doc = php_plutobook_document_from(Z_OBJ_P(ZEND_THIS));
	if (!php_plutobook_ready(doc, false)) {
		RETURN_THROWS();
	}
	// A bailout ended the previous load's request. Only a shutdown function can get here, and
	// it may load the document again.
	doc->bailed = false;
	if (css && memchr(ZSTR_VAL(css), 0, ZSTR_LEN(css))) {
		zend_argument_value_error(3, "must not contain null bytes");
		RETURN_THROWS();
	}
	if (ZSTR_LEN(html) > INT_MAX) {
		zend_argument_value_error(1, "exceeds the renderer's maximum HTML length");
		RETURN_THROWS();
	}
	// A remote baseUrl only resolves relative URLs. It never grants access to local files.
	const bool remote_base = base && php_plutobook_is_remote_url(base);
	if (remote_base && document_root) {
		zend_argument_value_error(2, "must be a file:/// URL when documentRoot is given");
		RETURN_THROWS();
	}
	if (remote_base && !php_plutobook_check_remote_base(base, 2)) {
		RETURN_THROWS();
	}
	bool failed = false;
	HashTable *allowed = hosts ? php_plutobook_parse_allowed_hosts(hosts, 5, &failed) : NULL;
	if (failed) {
		RETURN_THROWS();
	}
	// The asset root confines file reads. With documentRoot it is also the virtual URL root,
	// and PlutoBook resolves URLs against the base directory's path below it.
	const bool local_base = base && !remote_base;
	php_plutobook_directory directory = PHP_PLUTOBOOK_INVALID_DIRECTORY;
	zend_string *root = NULL, *virtual_base = NULL;
	if (document_root) {
		root = php_plutobook_asset_root(document_root, &directory, 4);
		virtual_base = root ? php_plutobook_virtual_base(root, base) : NULL;
		if (root && !virtual_base) {
			php_plutobook_close_directory(directory);
			zend_string_release(root);
			root = NULL;
		}
	} else if (local_base) {
		root = php_plutobook_asset_root(base, &directory, 2);
	}
	if ((local_base || document_root) && !root) {
		if (allowed) {
			zend_array_destroy(allowed);
		}
		RETURN_THROWS();
	}
	if (doc->asset_root) {
		php_plutobook_close_directory(doc->asset_dir);
		zend_string_release(doc->asset_root);
	}
	doc->asset_root = root;
	doc->asset_dir = directory;
	doc->virtual_root = virtual_base != NULL;
	if (doc->allowed_hosts) {
		zend_array_destroy(doc->allowed_hosts);
	}
	doc->allowed_hosts = allowed;
	if (doc->resource_errors) {
		zend_hash_clean(doc->resource_errors);
	}
	doc->asset_bytes = 0;
	doc->network_ms = 0;
	const char *base_url = virtual_base ? ZSTR_VAL(virtual_base)
	                       : base       ? ZSTR_VAL(base)
	                                    : "file:///";
	doc->busy = true;
	plutobook_clear_error_message();
	doc->loaded = plutobook_load_html(doc->book, ZSTR_VAL(html), (int)ZSTR_LEN(html),
	                                  css ? ZSTR_VAL(css) : "", "", base_url);
	doc->busy = false;
	if (virtual_base) {
		zend_string_release(virtual_base);
	}
	if (doc->bailed) {
		zend_bailout();
	}
	if (php_plutobook_timed_out()) {
		php_plutobook_raise_timeout();
	}
	if (!doc->loaded) {
		php_plutobook_fail_because("PlutoBook could not load the HTML document",
		                           php_plutobook_renderer_detail());
	}
}

#define PHP_PLUTOBOOK_STREAM_CLOSED "the destination stream was closed during rendering"

typedef struct {
	php_stream *stream;
	zend_resource *borrowed;
	smart_str bytes;
	const char *failure;
	bool failed;
	bool bailed;
} php_plutobook_output;

// Runs inside zend_try. Keeping the cursor in this frame means a longjmp cannot clobber it.
static void php_plutobook_write_stream(php_plutobook_output *out, const char *data, size_t length)
{
	while (length) {
		ssize_t wrote = php_stream_write(out->stream, data, length);
		if (out->borrowed && out->borrowed->ptr != out->stream) {
			out->failure = PHP_PLUTOBOOK_STREAM_CLOSED;
		} else if (wrote <= 0 || (size_t)wrote > length || EG(exception)) {
			out->failure = "the destination stream did not accept the data";
		} else {
			data += wrote;
			length -= (size_t)wrote;
			continue;
		}
		out->failed = true;
		return;
	}
}

static plutobook_stream_status_t php_plutobook_write(void *closure, const char *data,
                                                     unsigned int length)
{
	php_plutobook_output *out = closure;
	// A write error makes Cairo drop the remaining output, so a timeout ends rendering quickly.
	if (out->failed || EG(exception) || php_plutobook_timed_out()) {
		return PLUTOBOOK_STREAM_STATUS_WRITE_ERROR;
	}
	// Let the renderer unwind its C++ stack before propagating a Zend bailout.
	zend_try
	{
		if (out->stream) {
			php_plutobook_write_stream(out, data, length);
		} else {
			smart_str_appendl(&out->bytes, data, length);
		}
	}
	zend_catch
	{
		out->failed = true;
		out->bailed = true;
	}
	zend_end_try();
	return out->failed ? PLUTOBOOK_STREAM_STATUS_WRITE_ERROR : PLUTOBOOK_STREAM_STATUS_SUCCESS;
}

static bool php_plutobook_render(php_plutobook_document *doc, php_plutobook_output *out)
{
	// Keep the PHP resource usable by write filters while the native pointer
	// is borrowed. All supported stream destructors honor the recursion guard
	// in php_stream_free(); depth 2 also guards the enclosed-stream special case.
	bool regular = out->borrowed && out->borrowed->type == php_file_le_stream();
	bool had_no_fclose = false;
	if (out->borrowed) {
		if (out->stream->in_free) {
			php_plutobook_fail("The destination stream is already in use");
			return false;
		}
		had_no_fclose = (out->stream->flags & PHP_STREAM_FLAG_NO_FCLOSE) != 0;
		out->stream->in_free = 2;
		out->stream->flags |= PHP_STREAM_FLAG_NO_FCLOSE;
	}
	doc->busy = true;
	plutobook_clear_error_message();
	bool rendered = plutobook_write_to_pdf_stream(doc->book, php_plutobook_write, out);
	if (out->borrowed) {
		out->stream->in_free = 0;
		if (!had_no_fclose) {
			out->stream->flags &= ~PHP_STREAM_FLAG_NO_FCLOSE;
		}
		if (out->borrowed->ptr != out->stream) {
			out->failed = true;
			out->failure = PHP_PLUTOBOOK_STREAM_CLOSED;
			// Complete the regular resource destructor deferred during the call.
			// Persistent resources have no per-request list destructor.
			if (regular) {
				php_stream_free(out->stream, PHP_STREAM_FREE_CLOSE | PHP_STREAM_FREE_RSRC_DTOR);
			}
		}
	}
	doc->busy = false;
	if (out->bailed || doc->bailed) {
		smart_str_free(&out->bytes);
		zend_bailout();
	}
	// Raise the time limit error now, while shutdown functions can still run, instead of
	// letting PHP's hard timeout kill the process after a long uninterruptible render.
	if (php_plutobook_timed_out()) {
		smart_str_free(&out->bytes);
		php_plutobook_raise_timeout();
	}
	// PlutoBook 0.20.0 can return true after Cairo reports a callback error.
	if (!rendered || out->failed || EG(exception)) {
		php_plutobook_fail_because("Unable to write the PDF output",
		                           out->failure ? out->failure : php_plutobook_renderer_detail());
		return false;
	}
	return true;
}

PHP_METHOD(PlutoBook_Document, toPdf)
{
	ZEND_PARSE_PARAMETERS_NONE();
	php_plutobook_document *doc = php_plutobook_document_from(Z_OBJ_P(ZEND_THIS));
	php_plutobook_output out = {0};
	if (!php_plutobook_ready(doc, true) || !php_plutobook_render(doc, &out)) {
		smart_str_free(&out.bytes);
		RETURN_THROWS();
	}
	RETURN_STR(smart_str_extract(&out.bytes));
}

PHP_METHOD(PlutoBook_Document, writeToStream)
{
	zval *resource;
	ZEND_PARSE_PARAMETERS_START(1, 1)
	Z_PARAM_RESOURCE(resource)
	ZEND_PARSE_PARAMETERS_END();

	php_plutobook_document *doc = php_plutobook_document_from(Z_OBJ_P(ZEND_THIS));
	if (!php_plutobook_ready(doc, true)) {
		RETURN_THROWS();
	}
	php_plutobook_output out = {0};
	out.borrowed = Z_RES_P(resource);
	php_stream_from_zval_no_verify(out.stream, resource);
	if (!out.stream) {
		RETURN_THROWS();
	}
	if (!strpbrk(out.stream->mode, "waxc+")) {
		php_plutobook_fail("Expected a writable PHP stream");
		RETURN_THROWS();
	}
	php_plutobook_render(doc, &out);
}

PHP_METHOD(PlutoBook_Document, writeToFile)
{
	zend_string *path;
	ZEND_PARSE_PARAMETERS_START(1, 1)
	Z_PARAM_PATH_STR(path)
	ZEND_PARSE_PARAMETERS_END();

	php_plutobook_document *doc = php_plutobook_document_from(Z_OBJ_P(ZEND_THIS));
	if (!php_plutobook_ready(doc, true)) {
		RETURN_THROWS();
	}
	if (!ZSTR_LEN(path) || strstr(ZSTR_VAL(path), "://")) {
		zend_argument_value_error(1, "must be a local filesystem path");
		RETURN_THROWS();
	}
	php_plutobook_output out = {0};
	if (!php_check_open_basedir_ex(ZSTR_VAL(path), 0)) {
		out.stream = php_stream_fopen(ZSTR_VAL(path), "wb", NULL);
	}
	if (!out.stream) {
		php_plutobook_fail("Unable to open the PDF output file");
		RETURN_THROWS();
	}
	bool ok = php_plutobook_render(doc, &out);
	int flushed = php_stream_flush(out.stream);
	int closed = php_stream_close(out.stream);
	if (ok && (flushed || closed)) {
		php_plutobook_fail("Unable to finish writing the PDF output file");
	}
}

PHP_METHOD(PlutoBook_Document, getResourceErrors)
{
	ZEND_PARSE_PARAMETERS_NONE();
	php_plutobook_document *doc = php_plutobook_document_from(Z_OBJ_P(ZEND_THIS));
	if (!doc->resource_errors) {
		RETURN_EMPTY_ARRAY();
	}
	RETURN_ARR(zend_array_dup(doc->resource_errors));
}

#ifdef PHP_WIN32
// PIE installs supplementary files next to the PHP runtime. A SAPI host executable
// (e.g. Apache) may live elsewhere, so locate the DLL that exports sapi_module.
static void php_plutobook_set_packaged_fonts(void)
{
	if (getenv("FONTCONFIG_FILE") || getenv("FONTCONFIG_PATH")) {
		return;
	}
	HMODULE runtime;
	if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
	                            GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
	                        (LPCWSTR)&sapi_module, &runtime)) {
		return;
	}
	wchar_t path[MAXPATHLEN];
	DWORD length = GetModuleFileNameW(runtime, path, MAXPATHLEN);
	if (!length || length >= MAXPATHLEN) {
		return;
	}
	const wchar_t suffix[] = L"extras\\plutobook\\fonts.conf";
	wchar_t *name = wcsrchr(path, L'\\');
	if (!name || name - path + 1 + sizeof(suffix) / sizeof(*suffix) > MAXPATHLEN) {
		return;
	}
	memcpy(name + 1, suffix, sizeof(suffix));
	DWORD attributes = GetFileAttributesW(path);
	if (attributes == INVALID_FILE_ATTRIBUTES || (attributes & FILE_ATTRIBUTE_DIRECTORY)) {
		return;
	}
	*wcsrchr(path, L'\\') = L'\0';
	// Fontconfig uses ANSI filesystem APIs on Windows. The short path also works for
	// names outside the system code page on volumes that provide 8.3 aliases.
	wchar_t short_path[MAXPATHLEN];
	length = GetShortPathNameW(path, short_path, MAXPATHLEN);
	const wchar_t *config = length && length < MAXPATHLEN ? short_path : path;
	char directory[MAXPATHLEN];
	BOOL substituted = FALSE;
	if (WideCharToMultiByte(CP_ACP, 0, config, -1, directory, sizeof(directory), NULL,
	                        GetACP() == CP_UTF8 ? NULL : &substituted) &&
	    !substituted) {
		plutobook_set_fontconfig_path(directory);
	}
}
#endif

static PHP_GINIT_FUNCTION(plutobook)
{
#if defined(COMPILE_DL_PLUTOBOOK) && defined(ZTS)
	ZEND_TSRMLS_CACHE_UPDATE();
#endif
	memset(plutobook_globals, 0, sizeof(*plutobook_globals));
}

PHP_MINIT_FUNCTION(plutobook)
{
#ifdef PHP_WIN32
	php_plutobook_set_packaged_fonts();
#endif
	REGISTER_INI_ENTRIES();
	if (!php_plutobook_network_startup()) {
		UNREGISTER_INI_ENTRIES();
		return FAILURE;
	}
	php_plutobook_exception = register_class_PlutoBook_Exception(spl_ce_RuntimeException);
	zend_class_entry *ce = register_class_PlutoBook_Document();
	ce->create_object = php_plutobook_create_object;
	memcpy(&php_plutobook_handlers, zend_get_std_object_handlers(), sizeof(php_plutobook_handlers));
	php_plutobook_handlers.offset = offsetof(php_plutobook_document, std);
	php_plutobook_handlers.free_obj = php_plutobook_free_object;
	php_plutobook_handlers.clone_obj = NULL;
	return SUCCESS;
}

PHP_MSHUTDOWN_FUNCTION(plutobook)
{
	php_plutobook_network_shutdown();
	UNREGISTER_INI_ENTRIES();
	return SUCCESS;
}

PHP_MINFO_FUNCTION(plutobook)
{
	php_info_print_table_start();
	php_info_print_table_row(2, "PlutoBook support", "enabled");
	php_info_print_table_row(2, "Extension version", PHP_PLUTOBOOK_VERSION);
	php_info_print_table_row(2, "Rendering library", plutobook_version_string());
	php_info_print_table_row(2, "HTTP client", php_plutobook_network_version());
	php_info_print_table_end();
	DISPLAY_INI_ENTRIES();
}

zend_module_entry plutobook_module_entry = {STANDARD_MODULE_HEADER,
                                            "plutobook",
                                            NULL,
                                            PHP_MINIT(plutobook),
                                            PHP_MSHUTDOWN(plutobook),
                                            NULL,
                                            NULL,
                                            PHP_MINFO(plutobook),
                                            PHP_PLUTOBOOK_VERSION,
                                            PHP_MODULE_GLOBALS(plutobook),
                                            PHP_GINIT(plutobook),
                                            NULL,
                                            NULL,
                                            STANDARD_MODULE_PROPERTIES_EX};

#ifdef COMPILE_DL_PLUTOBOOK
#ifdef ZTS
ZEND_TSRMLS_CACHE_DEFINE()
#endif
ZEND_GET_MODULE(plutobook)
#endif
