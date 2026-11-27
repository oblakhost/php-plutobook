# API reference

The extension adds one class and one exception in the `PlutoBook` namespace:

```php
namespace PlutoBook;

final class Document
{
    public function __construct(string $pageSize = 'A4', string $orientation = 'portrait', string $margins = '15mm');
    public function loadHtml(string $html, ?string $baseUrl = null, string $css = '', ?string $documentRoot = null, array $allowedHosts = []): void;
    public function toPdf(): string;
    public function writeToFile(string $path): void;
    public function writeToStream($stream): void;  // an open, writable PHP stream
    /** @return list<array{url: string, reason: string, detail: string}> */
    public function getResourceErrors(): array;
}

class Exception extends \RuntimeException {}
```

Invalid arguments throw `ValueError` or `TypeError`. Rendering and writing
failures throw `PlutoBook\Exception`, and so does calling a render method before
`loadHtml()`. Documents can't be cloned or serialized.

## Document::__construct()

Creates an empty document with its page settings.

| Parameter | What it takes |
| --- | --- |
| `$pageSize` | `A3`, `A4`, `A5`, `B4`, `B5`, `Letter`, `Legal` or `Ledger` |
| `$orientation` | `portrait` or `landscape` |
| `$margins` | Like CSS `margin`: one to four lengths (top, right, bottom, left) in `pt`, `mm`, `cm`, `in`, `px` or `pc`. `0` works without a unit. |

An `@page` rule in the document's CSS can change these. See
[Page setup](../guides/page-setup.md).

## Document::loadHtml()

Loads the content, replacing anything loaded before.

| Parameter | What it takes |
| --- | --- |
| `$html` | UTF-8 HTML. |
| `$baseUrl` | Where relative paths start: a folder as a `file:///` URL ending in `/`, or a web page URL. |
| `$css` | Extra CSS that beats the document's own styles, except their `!important` rules. |
| `$documentRoot` | A folder, as a `file:///` URL ending in `/`, that acts as `/` for paths that start with `/`. Can't be combined with a web `baseUrl`. |
| `$allowedHosts` | Hosts the document may download from. Empty, the default, means no network. |

See [HTML and CSS](../guides/html-and-css.md),
[Local images, stylesheets and fonts](../guides/local-files.md) and
[Images and fonts from the web](../guides/remote-files.md).

## Document::toPdf()

Renders the document and returns the PDF as a string.

## Document::writeToFile()

Renders the document into the file at `$path`, creating it or replacing it. The
path must be local: anything containing `://` throws `ValueError`, so use
`writeToStream()` for stream wrappers.

## Document::writeToStream()

Renders the document into an open, writable PHP stream, starting at its current
position. The stream must be in binary mode (no `t` in its mode). It stays open;
flush or close it yourself.

If rendering fails halfway, the file or stream can hold a partial PDF. See
[Saving the PDF](../guides/saving.md).

## Document::getResourceErrors()

Returns the images, stylesheets and fonts that were skipped. Each `loadHtml()`
clears the list and rendering adds to it, with each resource listed once, as
`['url' => string, 'reason' => string, 'detail' => string]`. See
[Missing images and fonts](../guides/missing-resources.md) and
[Resource errors](resource-errors.md).

## PlutoBook\Exception

Thrown when rendering or writing fails. It extends `RuntimeException`, and its
message says what went wrong when that is known. Exceptions thrown by a PHP stream
wrapper reach your code unchanged. See
[Errors and time limits](../guides/errors-and-time-limits.md).
