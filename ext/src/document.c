// SPDX-License-Identifier: Apache-2.0
#include "binding.h"
#include "Zend/zend_exceptions.h"
#include "Zend/zend_smart_str.h"
#include "ext/spl/spl_exceptions.h"
#include "ext/standard/info.h"
#include "main/fopen_wrappers.h"
#include "../plutobook_arginfo.h"
#include <unistd.h>

#if __has_include("../release-version.h")
#include "../release-version.h"
#else
#define PHP_PLUTOBOOK_VERSION "0.1.0-dev"
#endif

#ifdef ZTS
#error "This release supports NTS PHP builds only"
#endif

ZEND_DECLARE_MODULE_GLOBALS(plutobook)

static zend_class_entry *php_plutobook_exception;
static zend_object_handlers php_plutobook_handlers;

// Reject malformed sizes. PHP's legacy fallback of 0 would silently remove the cap.
static ZEND_INI_MH(php_plutobook_update_max_asset_size)
{
	zend_string *error = NULL;
	zend_long bytes = zend_ini_parse_quantity(new_value, &error);
	bool invalid = error != NULL;
	if (error) {
		zend_string_release(error);
	}
	if (invalid || bytes < 0 || bytes > INT_MAX || !strpbrk(ZSTR_VAL(new_value), "0123456789")) {
		zend_error(E_WARNING,
		           "Invalid \"%s\" setting. Expected 0 or a byte size up to %d, such as 32M",
		           ZSTR_VAL(entry->name), INT_MAX);
		return FAILURE;
	}
	PLUTOBOOK_G(max_asset_size) = bytes;
	return SUCCESS;
}

PHP_INI_BEGIN()
PHP_INI_ENTRY("plutobook.max_asset_size", "32M", PHP_INI_ALL, php_plutobook_update_max_asset_size)
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
	return &doc->std;
}

static void php_plutobook_free_object(zend_object *object)
{
	php_plutobook_document *doc = php_plutobook_document_from(object);
	if (doc->book) {
		plutobook_destroy(doc->book);
	}
	if (doc->asset_root) {
		close(doc->asset_dir);
		zend_string_release(doc->asset_root);
	}
	if (doc->resource_errors) {
		zend_array_destroy(doc->resource_errors);
	}
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
	ZEND_PARSE_PARAMETERS_START(1, 4)
	Z_PARAM_STR(html)
	Z_PARAM_OPTIONAL
	Z_PARAM_STR_OR_NULL(base)
	Z_PARAM_STR(css)
	Z_PARAM_STR_OR_NULL(document_root)
	ZEND_PARSE_PARAMETERS_END();

	php_plutobook_document *doc = php_plutobook_document_from(Z_OBJ_P(ZEND_THIS));
	if (!php_plutobook_ready(doc, false)) {
		RETURN_THROWS();
	}
	if (css && memchr(ZSTR_VAL(css), 0, ZSTR_LEN(css))) {
		zend_argument_value_error(3, "must not contain null bytes");
		RETURN_THROWS();
	}
	if (ZSTR_LEN(html) > INT_MAX) {
		zend_argument_value_error(1, "exceeds the renderer's maximum HTML length");
		RETURN_THROWS();
	}
	// The asset root confines file reads. With documentRoot it is also the virtual URL root,
	// and PlutoBook resolves URLs against the base directory's path below it.
	int directory = -1;
	zend_string *root = NULL, *virtual_base = NULL;
	if (document_root) {
		root = php_plutobook_asset_root(document_root, &directory, 4);
		virtual_base = root ? php_plutobook_virtual_base(root, base) : NULL;
		if (root && !virtual_base) {
			close(directory);
			zend_string_release(root);
			root = NULL;
		}
	} else if (base) {
		root = php_plutobook_asset_root(base, &directory, 2);
	}
	if ((base || document_root) && !root) {
		RETURN_THROWS();
	}
	if (doc->asset_root) {
		close(doc->asset_dir);
		zend_string_release(doc->asset_root);
	}
	doc->asset_root = root;
	doc->asset_dir = directory;
	doc->virtual_root = virtual_base != NULL;
	if (doc->resource_errors) {
		zend_hash_clean(doc->resource_errors);
	}
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
		zend_timeout();
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
		zend_timeout();
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

PHP_MINIT_FUNCTION(plutobook)
{
	REGISTER_INI_ENTRIES();
	php_plutobook_exception = register_class_PlutoBook_Exception(spl_ce_RuntimeException);
	zend_class_entry *ce = register_class_PlutoBook_Document();
	ce->ce_flags |= ZEND_ACC_NOT_SERIALIZABLE | ZEND_ACC_NO_DYNAMIC_PROPERTIES;
	ce->create_object = php_plutobook_create_object;
	memcpy(&php_plutobook_handlers, zend_get_std_object_handlers(), sizeof(php_plutobook_handlers));
	php_plutobook_handlers.offset = offsetof(php_plutobook_document, std);
	php_plutobook_handlers.free_obj = php_plutobook_free_object;
	php_plutobook_handlers.clone_obj = NULL;
	return SUCCESS;
}

PHP_MSHUTDOWN_FUNCTION(plutobook)
{
	UNREGISTER_INI_ENTRIES();
	return SUCCESS;
}

PHP_MINFO_FUNCTION(plutobook)
{
	php_info_print_table_start();
	php_info_print_table_row(2, "PlutoBook support", "enabled");
	php_info_print_table_row(2, "Extension version", PHP_PLUTOBOOK_VERSION);
	php_info_print_table_row(2, "Rendering library", plutobook_version_string());
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
                                            NULL,
                                            NULL,
                                            NULL,
                                            STANDARD_MODULE_PROPERTIES_EX};

#ifdef COMPILE_DL_PLUTOBOOK
ZEND_GET_MODULE(plutobook)
#endif
