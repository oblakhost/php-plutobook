<?php

// Shared helpers for the network PHPTs.

use PlutoBook\Document;

// The pinned DejaVu Sans Bold face, served as a web font so its use is visible in the PDF.
function plutobook_test_font(): string
{
    $config = getenv('FONTCONFIG_FILE');
    return $config ? dirname($config, 3) . '/share/fonts/dejavu/DejaVuSans-Bold.ttf' : '';
}

// Start network-server.php and return its port. The server stops when the test ends.
function plutobook_test_server(string $transport, ?string $certificate = null): int
{
    $command = [PHP_BINARY, '-n', __DIR__ . '/network-server.php', $transport, plutobook_test_font()];
    if ($certificate !== null) {
        $command[] = $certificate;
    }
    $process = proc_open($command, [1 => ['pipe', 'w'], 2 => ['file', '/dev/null', 'w']], $pipes);
    $port = $process ? (int) fgets($pipes[1]) : 0;
    if (!$port) {
        exit("fixture server did not start\n");
    }
    register_shutdown_function(static function () use ($process): void {
        proc_terminate($process);
        proc_close($process);
    });
    return $port;
}

// Render one image reference and describe the outcome as "loaded" or "reason (detail)".
function plutobook_test_image(Document $doc, string $src, array $hosts, ?string $base = null, bool $detail = true): string
{
    $doc->loadHtml("<img src=\"$src\">", $base, allowedHosts: $hosts);
    if (str_contains($doc->toPdf(), '/Subtype /Image')) {
        return 'loaded';
    }
    $errors = array_map(
        static fn (array $error): string => $detail ? "$error[reason] ($error[detail])" : $error['reason'],
        $doc->getResourceErrors(),
    );
    return implode(', ', $errors);
}

// Fill colours used in the PDF's content streams, such as "0 0 1" for blue.
function plutobook_test_fills(string $pdf): array
{
    preg_match_all('/stream\r?\n(.*?)\r?\nendstream/s', $pdf, $streams);
    $fills = [];
    foreach ($streams[1] as $stream) {
        $content = @gzuncompress($stream);
        if ($content !== false && preg_match_all('/(\d+ \d+ \d+) rg/', $content, $match)) {
            array_push($fills, ...$match[1]);
        }
    }
    $fills = array_unique($fills);
    sort($fills);
    return $fills;
}
