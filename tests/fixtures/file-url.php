<?php

declare(strict_types=1);

/** Convert an absolute path in the local test checkout to a file URL. */
function plutobookTestFileUrl(string $path): string
{
    $path = str_replace(DIRECTORY_SEPARATOR, '/', $path);
    $prefix = 'file://';
    if (PHP_OS_FAMILY === 'Windows') {
        // Keep the drive colon literal and encode only the path components.
        $prefix .= '/' . substr($path, 0, 2);
        $path = substr($path, 2);
    }
    return $prefix . implode('/', array_map('rawurlencode', explode('/', $path)));
}
