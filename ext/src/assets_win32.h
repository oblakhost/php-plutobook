// SPDX-License-Identifier: Apache-2.0
#ifndef PHP_PLUTOBOOK_ASSETS_WIN32_H
#define PHP_PLUTOBOOK_ASSETS_WIN32_H

#include "binding.h"

php_plutobook_asset_status php_plutobook_win_path(const char *decoded, const char *root,
                                                  char path[MAXPATHLEN]);
php_plutobook_directory php_plutobook_open_directory(const char *path);
php_plutobook_asset_status php_plutobook_open_file(php_plutobook_document *doc, const char *path,
                                                   size_t *size, int *reader);

#endif
