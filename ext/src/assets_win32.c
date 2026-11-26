// SPDX-License-Identifier: Apache-2.0
#include "assets_win32.h"

#ifdef PHP_WIN32
#include "main/fopen_wrappers.h"
#include "win32/codepage.h"
#include <fcntl.h>
#include <io.h>
#include <winternl.h>

// Keep drive-qualified filesystem URLs and virtual URL paths unambiguous. Validate after
// percent-decoding, before consulting any Windows API that could interpret DOS special names.
static bool php_plutobook_win_component(const char *name, size_t length)
{
	for (size_t i = 0; i < length; i++) {
		if ((unsigned char)name[i] < ' ' || strchr("\\:\"<>|?*", name[i])) {
			return false;
		}
	}
	if (name[length - 1] == '.' || name[length - 1] == ' ') {
		return false;
	}
	size_t stem = 0;
	while (stem < length && name[stem] != '.') {
		stem++;
	}
	while (stem && name[stem - 1] == ' ') {
		stem--;
	}
	if (stem == 3 && (!strncasecmp(name, "CON", 3) || !strncasecmp(name, "NUL", 3) ||
	                  !strncasecmp(name, "AUX", 3) || !strncasecmp(name, "PRN", 3))) {
		return false;
	}
	if ((stem == 6 && !strncasecmp(name, "CONIN$", 6)) ||
	    (stem == 7 && !strncasecmp(name, "CONOUT$", 7))) {
		return false;
	}
	if (stem >= 4 && (!strncasecmp(name, "COM", 3) || !strncasecmp(name, "LPT", 3))) {
		// DOS also reserves COM/LPT with a superscript 1, 2 or 3, encoded here as UTF-8.
		if ((stem == 4 && name[3] >= '0' && name[3] <= '9') ||
		    (stem == 5 && (unsigned char)name[3] == 0xc2 &&
		     ((unsigned char)name[4] == 0xb9 || (unsigned char)name[4] == 0xb2 ||
		      (unsigned char)name[4] == 0xb3))) {
			return false;
		}
	}
	return true;
}

php_plutobook_asset_status php_plutobook_win_path(const char *decoded, const char *root,
                                                  char path[MAXPATHLEN])
{
	if (decoded[0] != '/' || decoded[1] == '/') {
		return PHP_PLUTOBOOK_ASSET_INVALID_URL;
	}
	const char *part = decoded + 1;
	size_t used;
	if (root) {
		used = strlen(root);
		memcpy(path, root, used + 1);
	} else {
		char drive = *part;
		if (drive >= 'a' && drive <= 'z') {
			drive -= 'a' - 'A';
		}
		if (drive < 'A' || drive > 'Z' || part[1] != ':' || part[2] != '/') {
			return PHP_PLUTOBOOK_ASSET_INVALID_URL;
		}
		memcpy(path, "C:/", 4);
		path[0] = drive;
		used = 3;
		part += 3;
	}
	const size_t floor = used;
	while (*part) {
		size_t length = strcspn(part, "/");
		if (!length) {
			return PHP_PLUTOBOOK_ASSET_INVALID_URL;
		}
		if (length == 2 && !memcmp(part, "..", 2)) {
			if (used <= floor) {
				return PHP_PLUTOBOOK_ASSET_OUTSIDE_BASE;
			}
			while (used > floor && path[used - 1] != '/') {
				used--;
			}
			if (used > floor) {
				used--;
			}
		} else if (length != 1 || *part != '.') {
			if (!php_plutobook_win_component(part, length) || used + length + 2 > MAXPATHLEN) {
				return PHP_PLUTOBOOK_ASSET_INVALID_URL;
			}
			if (path[used - 1] != '/') {
				path[used++] = '/';
			}
			memcpy(path + used, part, length);
			used += length;
		}
		part += length;
		if (*part) {
			part++;
		}
	}
	path[used] = '\0';
	return PHP_PLUTOBOOK_ASSET_OK;
}

// Both opens are relative to a pinned filesystem directory. OBJ_DONT_REPARSE refuses a
// reparse anywhere in the lookup, including junctions, symlinks and cloud recalls.
static php_plutobook_asset_status php_plutobook_win_open(HANDLE parent, const char *relative,
                                                         ACCESS_MASK access, HANDLE *result)
{
	wchar_t wide[MAXPATHLEN];
	int count = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, relative, -1, wide, MAXPATHLEN);
	if (!count) {
		return PHP_PLUTOBOOK_ASSET_INVALID_URL;
	}
	for (int i = 0; i < count; i++) {
		if (wide[i] == L'/') {
			wide[i] = L'\\';
		}
	}
	UNICODE_STRING name = {(USHORT)((count - 1) * sizeof(wchar_t)),
	                       (USHORT)(count * sizeof(wchar_t)), wide};
	OBJECT_ATTRIBUTES attributes;
	InitializeObjectAttributes(&attributes, &name, OBJ_CASE_INSENSITIVE | OBJ_DONT_REPARSE, parent,
	                           NULL);
	IO_STATUS_BLOCK io;
	NTSTATUS status =
	    NtOpenFile(result, access | FILE_READ_ATTRIBUTES | SYNCHRONIZE, &attributes, &io,
	               FILE_SHARE_READ | FILE_SHARE_WRITE, FILE_SYNCHRONOUS_IO_NONALERT);
	if (status >= 0) {
		return PHP_PLUTOBOOK_ASSET_OK;
	}
	switch (RtlNtStatusToDosError(status)) {
	case ERROR_FILE_NOT_FOUND:
	case ERROR_PATH_NOT_FOUND:
		return PHP_PLUTOBOOK_ASSET_MISSING;
	case ERROR_ACCESS_DENIED:
		return PHP_PLUTOBOOK_ASSET_PERMISSION_DENIED;
	default:
		return PHP_PLUTOBOOK_ASSET_VERIFICATION_FAILED;
	}
}

// Check open_basedir against the final name of the handle that supplies the bytes. Use PHP's
// active Windows code page and reject a lossy conversion before passing the path to PHP.
static bool php_plutobook_win_basedir(HANDLE handle)
{
	if (!PG(open_basedir) || !*PG(open_basedir)) {
		return true;
	}
	wchar_t path[MAXPATHLEN];
	DWORD length = GetFinalPathNameByHandleW(handle, path, MAXPATHLEN, FILE_NAME_NORMALIZED);
	if (length <= 4 || length >= MAXPATHLEN || wcsncmp(path, L"\\\\?\\", 4) || path[5] != L':') {
		return false;
	}
	char *local = php_win32_cp_w_to_any(path + 4);
	if (!local) {
		return false;
	}
	wchar_t *roundtrip = php_win32_cp_any_to_w(local);
	bool allowed =
	    roundtrip && !wcscmp(roundtrip, path + 4) && !php_check_open_basedir_ex(local, 0);
	free(roundtrip);
	free(local);
	return allowed;
}

static HANDLE php_plutobook_win_open_absolute(const char *path, ACCESS_MASK access)
{
	wchar_t drive[] = L"C:\\";
	drive[0] = (wchar_t)path[0];
	switch (GetDriveTypeW(drive)) {
	case DRIVE_FIXED:
	case DRIVE_REMOVABLE:
	case DRIVE_CDROM:
	case DRIVE_RAMDISK:
		break;
	default:
		return INVALID_HANDLE_VALUE;
	}
	HANDLE directory =
	    CreateFileW(drive, FILE_TRAVERSE | FILE_READ_ATTRIBUTES, FILE_SHARE_READ | FILE_SHARE_WRITE,
	                NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
	if (directory == INVALID_HANDLE_VALUE) {
		return INVALID_HANDLE_VALUE;
	}
	if (path[3]) {
		HANDLE child;
		php_plutobook_asset_status status =
		    php_plutobook_win_open(directory, path + 3, access, &child);
		CloseHandle(directory);
		if (status != PHP_PLUTOBOOK_ASSET_OK) {
			return INVALID_HANDLE_VALUE;
		}
		directory = child;
	}
	return directory;
}

php_plutobook_directory php_plutobook_open_directory(const char *path)
{
	HANDLE directory = php_plutobook_win_open_absolute(path, FILE_TRAVERSE);
	if (directory == INVALID_HANDLE_VALUE) {
		return INVALID_HANDLE_VALUE;
	}
	BY_HANDLE_FILE_INFORMATION info;
	if (GetFileType(directory) != FILE_TYPE_DISK || !GetFileInformationByHandle(directory, &info) ||
	    !(info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ||
	    (info.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) ||
	    !php_plutobook_win_basedir(directory)) {
		CloseHandle(directory);
		return INVALID_HANDLE_VALUE;
	}
	return directory;
}

php_plutobook_asset_status php_plutobook_open_file(php_plutobook_document *doc, const char *path,
                                                   size_t *size, int *reader)
{
	const char *relative = php_plutobook_relative_path(doc->asset_root, path);
	if (!relative) {
		// Preserve the open_basedir reason when both policies deny an existing file.
		// Query metadata with the same local-only lookup. Never open it for reading.
		if (PG(open_basedir) && *PG(open_basedir)) {
			HANDLE metadata = php_plutobook_win_open_absolute(path, 0);
			if (metadata != INVALID_HANDLE_VALUE) {
				bool allowed = php_plutobook_win_basedir(metadata);
				CloseHandle(metadata);
				if (!allowed) {
					return PHP_PLUTOBOOK_ASSET_OPEN_BASEDIR;
				}
			}
		}
		return PHP_PLUTOBOOK_ASSET_OUTSIDE_BASE;
	}
	if (!*relative) {
		return PHP_PLUTOBOOK_ASSET_NOT_REGULAR_FILE;
	}
	HANDLE handle;
	php_plutobook_asset_status status =
	    php_plutobook_win_open(doc->asset_dir, relative, FILE_READ_DATA, &handle);
	if (status != PHP_PLUTOBOOK_ASSET_OK) {
		return status;
	}
	BY_HANDLE_FILE_INFORMATION info;
	if (!GetFileInformationByHandle(handle, &info)) {
		status = PHP_PLUTOBOOK_ASSET_READ_ERROR;
	} else if (info.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) {
		status = PHP_PLUTOBOOK_ASSET_VERIFICATION_FAILED;
	} else if (GetFileType(handle) != FILE_TYPE_DISK ||
	           (info.dwFileAttributes & (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_DEVICE))) {
		status = PHP_PLUTOBOOK_ASSET_NOT_REGULAR_FILE;
	} else if (!php_plutobook_win_basedir(handle)) {
		status = PHP_PLUTOBOOK_ASSET_OPEN_BASEDIR;
	} else if (info.nFileSizeHigh || info.nFileSizeLow > INT_MAX ||
	           (PLUTOBOOK_G(max_asset_size) && info.nFileSizeLow > PLUTOBOOK_G(max_asset_size))) {
		status = PHP_PLUTOBOOK_ASSET_TOO_LARGE;
	} else if (!info.nFileSizeLow) {
		status = PHP_PLUTOBOOK_ASSET_EMPTY;
	} else {
		*reader = _open_osfhandle((intptr_t)handle, _O_RDONLY | _O_BINARY | _O_NOINHERIT);
		if (*reader >= 0) {
			*size = info.nFileSizeLow;
			return PHP_PLUTOBOOK_ASSET_OK; // Ownership transferred to the CRT descriptor.
		}
		status = PHP_PLUTOBOOK_ASSET_READ_ERROR;
	}
	CloseHandle(handle);
	return status;
}
#endif
