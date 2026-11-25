<?php

/** @generate-class-entries */

namespace PlutoBook;

class Exception extends \RuntimeException
{
}

/**
 * @strict-properties
 * @not-serializable
 */
final class Document
{
    public function __construct(string $pageSize = "A4", string $orientation = "portrait", string $margins = "15mm") {}

    public function loadHtml(string $html, ?string $baseUrl = null, string $css = "", ?string $documentRoot = null, array $allowedHosts = []): void {}

    public function toPdf(): string {}

    /** @param resource $stream */
    public function writeToStream($stream): void {}

    public function writeToFile(string $path): void {}

    /** @return list<array{url: string, reason: string, detail: string}> */
    public function getResourceErrors(): array {}
}
