<?php

declare(strict_types=1);

use PlutoBook\Document;
use PlutoBook\Exception as RenderException;

// /proc includes native allocations that memory_get_usage() cannot see.
$residentKib = static function (): int {
    $status = file_get_contents('/proc/self/status');
    if ($status === false || preg_match('/^VmRSS:\s+(\d+) kB$/m', $status, $match) !== 1) {
        throw new RuntimeException('Could not read resident memory from /proc/self/status');
    }
    return (int) $match[1];
};

$args = $_SERVER['argv'] ?? null;
if (!is_array($args) || count($args) !== 2 || !is_string($args[1] ?? null)) {
    throw new InvalidArgumentException('Expected memory.php INPUT.html');
}
$html = file_get_contents($args[1]);
$fixture = realpath(__DIR__ . '/../fixtures/invoice');
if ($html === false || $fixture === false) {
    throw new RuntimeException('Missing synthetic invoice');
}
$baseUrl = 'file://' . implode('/', array_map('rawurlencode', explode('/', $fixture))) . '/';
// Linux /dev/full returns ENOSPC on writes, after the renderer has started producing PDF bytes.
$full = fopen('/dev/full', 'wb');
if ($full === false) {
    throw new RuntimeException('Could not open /dev/full for the failed stream writes');
}
$rssSamples = [];
$phpSamples = [];
$failedWrites = 0;
for ($iteration = 1; $iteration <= 80; ++$iteration) {
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
    if ($iteration % 10 === 0) {
        $rssSamples[] = $residentKib();
        $phpSamples[] = memory_get_usage();
    }
}
fclose($full);
// Skip allocator warm-up. Take medians of iterations 20/30/40 and 60/70/80.
$medianGrowth = static function (int ...$samples): int {
    $early = array_slice($samples, 1, 3);
    $late = array_slice($samples, -3);
    sort($early);
    sort($late);
    return $late[1] - $early[1];
};
$rssGrowth = $medianGrowth(...$rssSamples);
$phpGrowth = $medianGrowth(...$phpSamples);
echo json_encode([
    'renders' => 80,
    'sample_interval' => 10,
    'failed_writes_followed_by_success' => $failedWrites,
    'php_usage_bytes' => $phpSamples,
    'php_peak_bytes' => memory_get_peak_usage(),
    'php_median_growth_bytes' => $phpGrowth,
    'php_growth_limit_bytes' => 262144,
    'rss_kib' => $rssSamples,
    'median_growth_kib' => $rssGrowth,
    'limit_kib' => 8192,
], JSON_PRETTY_PRINT | JSON_THROW_ON_ERROR), "\n";
if ($phpGrowth > 262144) {
    throw new RuntimeException("Steady-state PHP memory grew by $phpGrowth bytes; limit is 262144 bytes");
}
// RSS grows about 18 KiB per invoice render through a Cairo leak in PDF page output (see
// tools/asan/render-loop.c), so this budget only bounds gross regressions. The asan flavor's
// LeakSanitizer run is the precise gate for leaks in the binding itself.
if ($rssGrowth > 8192) {
    throw new RuntimeException("Steady-state RSS grew by $rssGrowth KiB; limit is 8192 KiB");
}
