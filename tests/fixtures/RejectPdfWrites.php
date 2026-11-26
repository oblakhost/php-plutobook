<?php

declare(strict_types=1);

namespace PlutoBook\Tests;

use php_user_filter;

/** Simulate a full destination stream on platforms without /dev/full. */
final class RejectPdfWrites extends php_user_filter
{
    public function filter($in, $out, &$consumed, bool $closing): int
    {
        return PSFS_ERR_FATAL;
    }
}
