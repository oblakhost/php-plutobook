<?php

declare(strict_types=1);

echo json_encode([
    'binary' => PHP_BINARY,
    'version' => PHP_VERSION,
    'sapi' => PHP_SAPI,
    'architecture' => php_uname('m'),
    'debug' => (bool) PHP_DEBUG,
    'zts' => (bool) PHP_ZTS,
    'extension_dir' => ini_get('extension_dir'),
    'ini' => php_ini_loaded_file(),
    'scanned' => php_ini_scanned_files(),
], JSON_THROW_ON_ERROR), "\n";
