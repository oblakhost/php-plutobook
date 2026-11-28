<?php

declare(strict_types=1);

use PlutoBook\Document;

// Render the text extraction fixture. tests/integration/inspect-text.py reads the result.
$args = $_SERVER['argv'] ?? null;
if (
    PHP_SAPI !== 'cli' || !is_array($args) || count($args) !== 3
    || !is_string($args[1] ?? null) || !is_string($args[2] ?? null)
) {
    throw new InvalidArgumentException('Expected text-extraction.php FONTS_DIR OUTPUT_DIR');
}
// The staged fonts directory, which holds inter/Inter-Regular.ttf for @font-face.
$fonts = realpath($args[1]);
$html = file_get_contents(dirname(__DIR__) . '/fixtures/text/extraction.html');
if ($fonts === false || $html === false) {
    throw new RuntimeException('Missing text extraction fonts or fixture');
}
require dirname(__DIR__) . '/fixtures/file-url.php';
$document = new Document();
$document->loadHtml($html, plutobookTestFileUrl($fonts . '/'));
$errors = $document->getResourceErrors();
if ($errors !== []) {
    throw new RuntimeException('Text fixture resources failed: ' . json_encode($errors, JSON_THROW_ON_ERROR));
}
$document->writeToFile($args[2] . '/text.pdf');
