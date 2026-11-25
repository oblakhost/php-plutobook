// SPDX-License-Identifier: Apache-2.0
#ifndef PHP_PLUTOBOOK_BINDING_H
#define PHP_PLUTOBOOK_BINDING_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "php.h"
#include <plutobook.h>

// Expanded ZEND_BEGIN/END_MODULE_GLOBALS, spelled out so clang-format can indent the members.
typedef struct _zend_plutobook_globals {
	zend_long max_asset_size;
} zend_plutobook_globals;

ZEND_EXTERN_MODULE_GLOBALS(plutobook)
#define PLUTOBOOK_G(v) ZEND_MODULE_GLOBALS_ACCESSOR(plutobook, v)

// Prefix of the renderer message the fetcher sets for every skipped resource.
#define PHP_PLUTOBOOK_ASSET_SKIPPED "Asset skipped"

typedef struct {
	plutobook_t *book;
	zend_string *asset_root;
	HashTable *resource_errors;
	int asset_dir;
	bool loaded;
	bool busy;
	bool bailed;
	// With documentRoot, every file: URL path is relative to asset_root, like a web server's.
	bool virtual_root;
	zend_object std;
} php_plutobook_document;

// PHP's execution timer only sets this flag. The VM raises the error at its next check, which
// never happens while the renderer runs, so callbacks poll it and stop the renderer early.
static inline bool php_plutobook_timed_out(void)
{
	return zend_atomic_bool_load_ex(&EG(timed_out));
}

bool php_plutobook_page_options(zend_string *paper, zend_string *orientation, zend_string *margins,
                                plutobook_page_size_t *size, plutobook_page_margins_t *insets);
zend_string *php_plutobook_asset_root(zend_string *base, int *directory, uint32_t arg);
zend_string *php_plutobook_virtual_base(zend_string *root, zend_string *base);
plutobook_resource_data_t *php_plutobook_fetch_asset(void *closure, const char *url);

#endif
