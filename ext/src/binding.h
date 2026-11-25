// SPDX-License-Identifier: Apache-2.0
#ifndef PHP_PLUTOBOOK_BINDING_H
#define PHP_PLUTOBOOK_BINDING_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "php.h"
#include <plutobook.h>

typedef struct {
	plutobook_t *book;
	zend_string *asset_root;
	int asset_dir;
	bool loaded;
	bool busy;
	bool bailed;
	zend_object std;
} php_plutobook_document;

bool php_plutobook_page_options(zend_string *paper, zend_string *orientation, zend_string *margins,
                                plutobook_page_size_t *size, plutobook_page_margins_t *insets);
zend_string *php_plutobook_asset_root(zend_string *base, int *directory);
plutobook_resource_data_t *php_plutobook_fetch_asset(void *closure, const char *url);

#endif
