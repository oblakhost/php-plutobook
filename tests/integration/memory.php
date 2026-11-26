<?php

declare(strict_types=1);

use PlutoBook\Document;
use PlutoBook\Exception as RenderException;
use PlutoBook\Tests\RejectPdfWrites;

// Sample the PHP process, including allocations made by the renderer.
$residentKib = static function (): int {
    if (PHP_OS_FAMILY === 'Windows') {
        $pid = getmypid();
        if ($pid === false) {
            throw new RuntimeException('Could not identify the PHP process');
        }
        $windows = getenv('SystemRoot');
        if (!$windows) {
            throw new RuntimeException('Unknown Windows installation directory');
        }
        $powershell = escapeshellarg($windows . '/System32/WindowsPowerShell/v1.0/powershell.exe');
        exec("$powershell -NoProfile -NonInteractive -Command \"(Get-Process -Id $pid).WorkingSet64\"", $rows, $code);
        if ($code !== 0 || count($rows) !== 1 || preg_match('/^[0-9]+$/D', $rows[0]) !== 1) {
            throw new RuntimeException('Could not query the PHP process working set');
        }
        return intdiv((int) $rows[0], 1024);
    }
    $status = file_get_contents('/proc/self/status');
    if ($status === false || preg_match('/^VmRSS:\s+(\d+) kB$/m', $status, $match) !== 1) {
        throw new RuntimeException('Could not read resident memory from /proc/self/status');
    }
    return (int) $match[1];
};

$args = $_SERVER['argv'] ?? null;
if (!is_array($args) || !is_string($args[1] ?? null) || count($args) > 3) {
    throw new InvalidArgumentException('Expected memory.php INPUT.html [RENDERS_PER_SAMPLE]');
}
$argument = $args[2] ?? '10';
if (!is_string($argument)) {
    throw new InvalidArgumentException('Expected RENDERS_PER_SAMPLE to be an integer');
}
$interval = (int) $argument;
if ((string) $interval !== $argument || $interval < 1 || $interval > intdiv(PHP_INT_MAX, 8)) {
    throw new InvalidArgumentException('Expected a positive integer for RENDERS_PER_SAMPLE');
}
$renders = 8 * $interval;
$html = file_get_contents($args[1]);
$fixture = realpath(__DIR__ . '/../fixtures/invoice');
if ($html === false || $fixture === false) {
    throw new RuntimeException('Missing synthetic invoice');
}
require dirname(__DIR__) . '/fixtures/file-url.php';
$baseUrl = plutobookTestFileUrl($fixture . '/');
// Fail inside the renderer's output callback. /dev/full provides ENOSPC on Linux.
if (PHP_OS_FAMILY === 'Windows') {
    require dirname(__DIR__) . '/fixtures/RejectPdfWrites.php';
    $full = fopen('php://temp', 'w+b');
    if (
        $full === false || !stream_filter_register('plutobook.reject', RejectPdfWrites::class)
        || stream_filter_append($full, 'plutobook.reject', STREAM_FILTER_WRITE) === false
    ) {
        throw new RuntimeException('Could not install the failing stream filter');
    }
} else {
    $full = fopen('/dev/full', 'wb');
    if ($full === false) {
        throw new RuntimeException('Could not open /dev/full for the failed stream writes');
    }
}
$rssSamples = [];
$phpSamples = [];
$failedWrites = 0;
for ($iteration = 1; $iteration <= $renders; ++$iteration) {
    $document = new Document();
    $document->loadHtml($html, $baseUrl);
    if ($iteration % 2 === 0) {
        try {
            @$document->writeToStream($full);
            throw new RuntimeException("Stream write unexpectedly succeeded in iteration $iteration");
        } catch (RenderException) {
            ++$failedWrites;
        }
    }
    $pdf = $document->toPdf();
    if (!str_starts_with($pdf, '%PDF-')) {
        throw new RuntimeException("Invalid invoice in iteration $iteration");
    }
    unset($document, $pdf);
    if ($iteration % $interval === 0) {
        $rssSamples[] = $residentKib();
        $phpSamples[] = memory_get_usage();
    }
}
fclose($full);
// Skip the first warm-up sample and compare medians of three early and three late samples.
$medianGrowth = static function (int ...$samples): int {
    $early = array_slice($samples, 1, 3);
    $late = array_slice($samples, -3);
    sort($early);
    sort($late);
    return $late[1] - $early[1];
};
// At 100 renders per sample the two median samples are 400 invoices apart.
$rssLimit = PHP_OS_FAMILY === 'Windows' && $interval >= 100 ? 4096 : 8192;
$rssGrowth = $medianGrowth(...$rssSamples);
$phpGrowth = $medianGrowth(...$phpSamples);
echo json_encode([
    'renders' => $renders,
    'sample_interval' => $interval,
    'failed_writes_followed_by_success' => $failedWrites,
    'php_usage_bytes' => $phpSamples,
    'php_peak_bytes' => memory_get_peak_usage(),
    'php_median_growth_bytes' => $phpGrowth,
    'php_growth_limit_bytes' => 262144,
    'rss_source' => PHP_OS_FAMILY === 'Windows' ? 'WorkingSet64' : 'VmRSS',
    'rss_kib' => $rssSamples,
    'median_growth_kib' => $rssGrowth,
    'limit_kib' => $rssLimit,
], JSON_PRETTY_PRINT | JSON_THROW_ON_ERROR), "\n";
if ($phpGrowth > 262144) {
    throw new RuntimeException("Steady-state PHP memory grew by $phpGrowth bytes; limit is 262144 bytes");
}
// This working-set budget catches large regressions across renderers and allocators.
// The Linux ASan suite checks individual native allocations for leaks.
if ($rssGrowth > $rssLimit) {
    throw new RuntimeException("Steady-state RSS grew by $rssGrowth KiB; limit is $rssLimit KiB");
}
