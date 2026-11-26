<?php

declare(strict_types=1);

use PlutoBook\Document;

$fixture = realpath(__DIR__ . '/../fixtures/invoice');
if ($fixture === false) {
    throw new RuntimeException('Missing invoice assets');
}
require dirname(__DIR__) . '/fixtures/file-url.php';
$baseUrl = plutobookTestFileUrl($fixture . '/');

if (PHP_SAPI === 'fpm-fcgi') {
    if (($_SERVER['REQUEST_METHOD'] ?? '') !== 'POST') {
        http_response_code(405);
        exit;
    }
    $html = file_get_contents('php://input');
    if ($html === false || $html === '') {
        throw new RuntimeException('Expected an HTML invoice in the POST body');
    }
    $document = new Document();
    $document->loadHtml($html, $baseUrl);
    header('Content-Type: application/pdf');
    echo $document->toPdf();
    return;
}

$args = $_SERVER['argv'] ?? null;
if (
    PHP_SAPI !== 'cli' || !is_array($args) || count($args) !== 3
    || !is_string($args[1] ?? null) || !is_string($args[2] ?? null)
) {
    throw new InvalidArgumentException('Expected invoice.php INPUT.html OUTPUT_DIR');
}
$html = file_get_contents($args[1]);
if ($html === false) {
    throw new RuntimeException('Could not read invoice HTML');
}
$document = new Document();
$document->loadHtml($html, $baseUrl);
$document->writeToFile($args[2] . '/cli.pdf');

// Missing relative images should leave the surrounding content and the document usable.
$document->loadHtml('<p>Pre nedostupne slike</p><img src="absent.svg"><p>Posle nedostupne slike</p>', $baseUrl);
$document->writeToFile($args[2] . '/missing.pdf');

// Alternate paper, optional sections, and CSS supplied by the application.
$short = file_get_contents($fixture . '/short.html');
if ($short === false) {
    throw new RuntimeException('Missing short quotation fixture');
}
$quotation = new Document(pageSize: 'Letter', orientation: 'landscape', margins: '10mm 20mm');
$quotation->loadHtml(
    $short,
    css: 'body { font: 11pt "DejaVu Sans"; } th, td { text-align: left; padding: 5pt; } '
        . 'footer::after { content: "Valid for 30 days"; }',
);
$quotation->writeToFile($args[2] . '/short.pdf');
