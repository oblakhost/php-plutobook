// SPDX-License-Identifier: Apache-2.0
#include "binding.h"
#include "ext/standard/url.h"
#include "main/fopen_wrappers.h"
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

// Only local file URLs reach the filesystem. Inline data URLs are decoded by PlutoBook.
static bool php_plutobook_resolve_file(const char *url, char resolved[MAXPATHLEN])
{
	if (strncmp(url, "file:///", 8) != 0) {
		return false;
	}
	const char *path = url + sizeof("file://") - 1;
	size_t length = strcspn(path, "?#");
	char decoded[MAXPATHLEN];
	if (length >= sizeof(decoded)) {
		return false;
	}
	memcpy(decoded, path, length);
	decoded[length] = '\0';
	size_t decoded_length = php_raw_url_decode(decoded, length);
	return strlen(decoded) == decoded_length && VCWD_REALPATH(decoded, resolved) != NULL &&
	       php_check_open_basedir_ex(resolved, 0) == 0;
}

// O_PATH pins the file identity without opening a device, pipe, or socket for I/O.
// Verify the kernel's descriptor name before any content read; directory entries
// can change between realpath/open_basedir checks and openat().
static int php_plutobook_pin_file(int parent, const char *name, const char *expected, int flags)
{
	int fd = openat(parent, name, O_PATH | O_CLOEXEC | flags);
	if (fd >= 0) {
		char proc[64], actual[MAXPATHLEN];
		snprintf(proc, sizeof(proc), "/proc/self/fd/%d", fd);
		ssize_t length = readlink(proc, actual, sizeof(actual));
		if (length >= 0 && (size_t)length == strlen(expected) &&
		    !memcmp(expected, actual, length)) {
			return fd;
		}
		close(fd);
	}
	return -1;
}

zend_string *php_plutobook_asset_root(zend_string *base, int *directory)
{
	const char *url = ZSTR_VAL(base);
	char resolved[MAXPATHLEN];
	if (ZSTR_LEN(base) && strlen(url) == ZSTR_LEN(base) && !strpbrk(url, "?#") &&
	    url[ZSTR_LEN(base) - 1] == '/' && php_plutobook_resolve_file(url, resolved)) {
		zend_string *root = zend_string_init(resolved, strlen(resolved), 0);
		*directory = php_plutobook_pin_file(AT_FDCWD, resolved, resolved, O_DIRECTORY);
		if (*directory >= 0) {
			return root;
		}
		zend_string_release(root);
	}
	zend_argument_value_error(
	    2, "must be a file:/// URL ending in / for an accessible local directory");
	return NULL;
}

static plutobook_resource_data_t *php_plutobook_read_asset(php_plutobook_document *doc,
                                                           const char *url)
{
	char path[MAXPATHLEN];
	if (!doc->asset_root || !php_plutobook_resolve_file(url, path)) {
		return NULL;
	}
	const size_t root_len = ZSTR_LEN(doc->asset_root);
	if (strncmp(path, ZSTR_VAL(doc->asset_root), root_len) ||
	    (root_len != 1 && path[root_len] != '/')) {
		return NULL;
	}
	const char *relative = path + root_len;
	if (*relative == '/') {
		relative++;
	}
	int pinned = php_plutobook_pin_file(doc->asset_dir, relative, path, 0);
	if (pinned < 0) {
		return NULL;
	}
	struct stat st;
	int reader = -1;
	if (fstat(pinned, &st) == 0 && S_ISREG(st.st_mode) && st.st_size > 0 && st.st_size <= INT_MAX) {
		char proc[64];
		snprintf(proc, sizeof(proc), "/proc/self/fd/%d", pinned);
		reader = open(proc, O_RDONLY | O_CLOEXEC);
	}
	close(pinned);
	if (reader < 0) {
		return NULL;
	}
	// Give PlutoBook ownership of one native buffer. Reading the verified fd
	// avoids reopening the pathname and keeps descriptor cleanup independent of Zend.
	size_t size = (size_t)st.st_size;
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
		const char *suffix = strrchr(path, '.');
		const char *mime = suffix && !strcasecmp(suffix, ".svg") ? "image/svg+xml" : "";
		result = plutobook_resource_data_create_without_copy(bytes, (unsigned int)size, mime, "",
		                                                     free, bytes);
	}
	if (!result) {
		free(bytes);
	}
	return result;
}

plutobook_resource_data_t *php_plutobook_fetch_asset(void *closure, const char *url)
{
	php_plutobook_document *doc = closure;
	plutobook_resource_data_t *resource = NULL;
	if (!doc->bailed && !EG(exception)) {
		zend_try
		{
			resource = php_plutobook_read_asset(doc, url);
		}
		zend_catch
		{
			doc->bailed = true;
		}
		zend_end_try();
	}
	if (!resource) {
		plutobook_set_error_message("Resource unavailable under the document's local asset policy");
	}
	return resource;
}
