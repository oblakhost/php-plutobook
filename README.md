<div align="center">

# PlutoBook for PHP

**Make PDFs from HTML and CSS, right inside PHP.**

[![Packagist](https://img.shields.io/packagist/v/oblakhost/ext-plutobook?logo=packagist&logoColor=white)](https://packagist.org/packages/oblakhost/ext-plutobook)
[![PHP 8.2–8.6](https://img.shields.io/badge/PHP-8.2%E2%80%938.6-777BB4?logo=php&logoColor=white)](#installation)
[![License: Apache-2.0](https://img.shields.io/badge/license-Apache--2.0-blue)](LICENSE)

[![Linux x86_64 and arm64](https://img.shields.io/badge/Linux-x86__64%20%7C%20arm64-FCC624?logo=linux&logoColor=black)](#installation)
[![Windows x64](https://img.shields.io/badge/Windows-x64-0078D4?logo=data:image/svg%2Bxml;base64,PHN2ZyB4bWxucz0iaHR0cDovL3d3dy53My5vcmcvMjAwMC9zdmciIHZpZXdCb3g9IjAgMCAyNCAyNCI+PHBhdGggZmlsbD0iI2ZmZiIgZD0iTTAgMGgxMXYxMUgwem0xMyAwaDExdjExSDEzek0wIDEzaDExdjExSDB6bTEzIDBoMTF2MTFIMTN6Ii8+PC9zdmc+)](#installation)

[![CI](https://github.com/oblakhost/php-plutobook/actions/workflows/toolchains.yml/badge.svg?branch=master)](https://github.com/oblakhost/php-plutobook/actions/workflows/toolchains.yml)
[![Coverage](https://codecov.io/gh/oblakhost/php-plutobook/graph/badge.svg)](https://codecov.io/gh/oblakhost/php-plutobook)
[![semantic-release: conventional](https://img.shields.io/badge/semantic--release-conventional-e10079?logo=semantic-release)](release.config.mjs)

</div>

Write your invoice, report or ticket as an HTML page, and this extension turns it
into a PDF. It runs inside PHP, so it works in your web app, CLI scripts and queue
workers without a headless browser, a separate PDF service or shell commands.

It is built on [PlutoBook](https://github.com/plutoprint/plutobook), a lightweight
engine made for laying out pages for print. It is not a browser engine like
Chromium or WebKit.

```php
$document = new PlutoBook\Document(pageSize: 'A4', margins: '15mm');
$document->loadHtml(
    '<h1>Invoice 42</h1><p>Total: 120.00 EUR</p>',
    css: 'h1 { color: #194572 }',
);
$document->writeToFile('invoice-42.pdf');
```

## Features

- **Runs inside PHP.** Install the extension and render. Nothing else to run or call.
- **CSS for paper.** Page sizes and margins, running headers and footers, page
  numbers, page breaks, flexbox, tables, multiple columns and SVG.
- **Any language.** Right-to-left and complex scripts such as Arabic, Hebrew and
  Hindi, plus emoji.
- **Your images and fonts.** PNG, JPEG, WebP and SVG images, stylesheets and web
  fonts, from folders you choose or websites you allow.
- **Safe by default.** No network access, and no files outside the folders you
  pass in, unless you allow it.
- **Tells you what's missing.** `getResourceErrors()` lists every image, font or
  stylesheet that couldn't be loaded, and why.
- **Ready-made builds.** For PHP 8.2 to 8.6 on Linux (x86_64 and arm64) and Windows
  (x64), plus deb and rpm packages.

## Installation

```sh
pie install oblakhost/ext-plutobook
```

[PIE](https://github.com/php/pie) is PHP's extension installer, the successor to
PECL. It downloads a ready-made build for your PHP when there is one, and compiles
from source when there isn't. Every
[release](https://github.com/oblakhost/php-plutobook/releases/latest) also has deb
and rpm packages if you'd rather use your system's package manager.

All platforms below support PHP 8.2 to 8.6. Linux builds cover x86_64 and arm64
(aarch64), and Windows builds cover x64.

| Platform | Versions | PHP from | Install with |
| --- | --- | --- | --- |
| ![Debian](https://img.shields.io/badge/Debian-A81D33?logo=debian&logoColor=white) | 12, 13 | Debian or [Surý's repository](https://deb.sury.org/) | [deb package](INSTALL.md#debian-and-ubuntu) or PIE |
| ![Ubuntu](https://img.shields.io/badge/Ubuntu-E95420?logo=ubuntu&logoColor=white) | 22.04, 24.04 | Ubuntu or [Surý's PPA](https://launchpad.net/~ondrej/+archive/ubuntu/php) | [deb package](INSTALL.md#debian-and-ubuntu) or PIE |
| ![RHEL, AlmaLinux, Rocky Linux](https://img.shields.io/badge/RHEL%20%C2%B7%20AlmaLinux%20%C2%B7%20Rocky-EE0000?logo=redhat&logoColor=white) | 8, 9, 10 | AppStream or [Remi's repository](https://rpms.remirepo.net/) | [rpm package](INSTALL.md#rhel-almalinux-rocky-linux-and-cloudlinux), or PIE on 9 and 10 |
| ![Linux](https://img.shields.io/badge/Other%20Linux-FCC624?logo=linux&logoColor=black) | glibc 2.28 or newer | any non-thread-safe build | [PIE](INSTALL.md#other-linux-distributions) |
| ![Windows](https://img.shields.io/badge/Windows-0078D4?logo=data:image/svg%2Bxml;base64,PHN2ZyB4bWxucz0iaHR0cDovL3d3dy53My5vcmcvMjAwMC9zdmciIHZpZXdCb3g9IjAgMCAyNCAyNCI+PHBhdGggZmlsbD0iI2ZmZiIgZD0iTTAgMGgxMXYxMUgwem0xMyAwaDExdjExSDEzek0wIDEzaDExdjExSDB6bTEzIDBoMTF2MTFIMTN6Ii8+PC9zdmc+) | x64 | thread-safe or not | [PIE](INSTALL.md#windows) |

On Linux with thread-safe PHP or musl (Alpine), PIE compiles the extension, which
needs the PlutoBook library installed first. macOS isn't supported.
[INSTALL.md](INSTALL.md) has step-by-step instructions for every platform, how to
verify downloads and how to build from source.

## Usage

The whole API is one class and one exception:

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

### Page size and margins

```php
$document = new PlutoBook\Document(
    pageSize: 'Letter',        // A3, A4 (default), A5, B4, B5, Letter, Legal or Ledger
    orientation: 'landscape',  // portrait (default) or landscape
    margins: '20mm 15mm',      // like CSS margin: 1 to 4 lengths, default 15mm
);
```

Margins take lengths in `pt`, `mm`, `cm`, `in`, `px` or `pc`, and `0` works
without a unit. An `@page` rule in your CSS can change all of these. PlutoBook uses
the size and margins of the first page for the whole document.

### HTML and CSS

`loadHtml()` takes UTF-8 HTML and, optionally, extra CSS. Calling it again
replaces the content. PlutoBook's HTML parser copes with ordinary imperfect markup.

```php
$document->loadHtml($html, css: <<<'CSS'
    @page {
        @bottom-center { content: "Page " counter(page); }
    }
    h1 { break-before: page; }
    CSS);
```

The CSS you pass this way beats the document's own `<style>` tags, linked
stylesheets and `style` attributes, however specific their selectors are. The one
exception is the document's `!important` rules, which still beat yours unless you
mark yours `!important` too. This is PlutoBook's behavior, and browsers treat user
stylesheets differently.

### Saving the PDF

```php
$pdf = $document->toPdf();               // as a string
$document->writeToFile('/tmp/out.pdf');  // to a file, replacing it
$document->writeToStream($stream);       // to an open PHP stream
```

`writeToStream()` starts at the stream's current position and leaves the stream
open. Flush or close it yourself, and don't use it elsewhere until the call
returns.

Open files in binary mode. A file opened with `t` in its mode, such as `wt`, is
refused, because Windows would change the line endings inside the PDF.

Write filters, added with `stream_filter_append()` or a `php://filter` URL, hide
failed writes. PHP only reports what the first filter took in, and when the stream
behind the filter refuses data, PHP drops it without an error. A filter can also
hold back its last bytes until you close the stream. When you need to know the
whole PDF arrived, write to a stream without filters, which throws when data is
refused, or change the PDF yourself and check what `fwrite()` returns:

```php
$data = base64_encode($document->toPdf());
if (fwrite($stream, $data) !== strlen($data)) {
    throw new RuntimeException('The PDF was not written completely');
}
```

If rendering fails halfway, the file or stream can hold a partial PDF. To never
serve a broken file, write to a temporary file in the same folder and `rename()`
it when the write succeeds.

### Images, stylesheets and fonts

By default a document can only use resources embedded as `data:` URLs. To use
files from disk, tell `loadHtml()` which folder they are in, as an absolute
`file:///` URL that ends with `/`.

For relative paths such as `logo.svg` or `css/print.css`, pass the folder as
`baseUrl`. Percent-encode each part of the path:

```php
$assets = realpath(__DIR__ . '/assets');
if ($assets === false) {
    throw new RuntimeException('Missing invoice assets directory');
}
$baseUrl = 'file://' . implode('/', array_map('rawurlencode', explode('/', $assets))) . '/';
$document->loadHtml('<img src="logo.svg"><p>Statement</p>', baseUrl: $baseUrl);
```

Templates written for a website often use paths that start with `/`, such as
`/assets/logo.svg`. For those, pass the site's public folder as `documentRoot`. It
then acts as `/`, the way it does on a web server. `baseUrl` becomes optional: it
must be a folder inside the document root and sets where relative paths start.

```php
$document->loadHtml(
    '<img src="/assets/logo.svg"><img src="stamp.png">',
    baseUrl: 'file:///srv/shop/public/invoices/',  // stamp.png -> public/invoices/stamp.png
    documentRoot: 'file:///srv/shop/public/',      // /assets/… -> public/assets/…
);
```

With a document root, every `file:` URL stays inside it, even
`file:///srv/shop/…`, and `..` can't climb out. The same template renders the same
way from your local files as from the live site.

On Windows, use a drive URL such as `file:///C:/invoices/assets/`.

PlutoBook also uses the fonts installed on the system, through Fontconfig.

### Images and fonts from the web

Nothing is downloaded unless you list the hosts a document may contact:

```php
$document->loadHtml(
    $html,
    baseUrl: 'https://shop.example/invoices/42/',
    allowedHosts: ['shop.example', 'fonts.gstatic.com', '*.cdn.example'],
);
```

An entry is a host name, a `*.example.com` pattern for its subdomains (not
`example.com` itself), a bracketed IPv6 address such as `[2001:db8::1]`, or `*`
for any host. Write international domain names in their ASCII (punycode) form.

A web `baseUrl` resolves relative paths the way a browser does and gives no access
to local files, so it can't be combined with `documentRoot`. You can still use
`allowedHosts` with a local `baseUrl` or `documentRoot` for absolute `https://`
links. The extension downloads with libcurl directly and doesn't need PHP's `curl`
extension. [Security](#security) lists what it refuses to fetch.

A stylesheet or SVG that redirects keeps the address you linked. PlutoBook looks
up its relative links, such as `url(fonts/face.ttf)` or `@import`, next to that
address, not next to the one it redirected to. Link to the final address, or use
absolute URLs inside files that get redirected.

### Missing images and fonts

If an image, stylesheet or font can't be loaded, it is left out and the rest of
the page still renders. `getResourceErrors()` tells you what was skipped:

```php
$document->writeToFile('invoice.pdf');

foreach ($document->getResourceErrors() as $error) {
    // ['url' => 'https://cdn.example/logo.png', 'reason' => 'http-error', 'detail' => 'HTTP 404']
    error_log("{$error['url']}: {$error['reason']} ({$error['detail']})");
}
```

Check the list after rendering, because fonts and background images load while
the PDF is written. It covers the latest `loadHtml()` and any rendering since,
with each resource listed once. To treat missing assets as an error, throw when
the list isn't empty. With a document root, URLs are reported as the document sees
them, for example `file:///assets/missing.png` rather than the path on the server.

<details>
<summary>All reasons</summary>

| Reason | Meaning |
| --- | --- |
| `network-disabled` | An `http:` or `https:` URL in a document without `allowedHosts`. |
| `unsupported-scheme` | Any other non-`file:` URL. |
| `no-base-url` | A local file without a `baseUrl` or `documentRoot`. |
| `invalid-url` | A malformed URL, such as a `file:` URL with an encoded NUL byte. |
| `missing` | The file does not exist. |
| `open-basedir` | PHP's `open_basedir` blocks the file. |
| `outside-base` | The file is outside the `documentRoot`, or outside the `baseUrl` folder when there is no document root. |
| `verification-failed` | A folder or the file changed between the checks and the read. |
| `not-regular-file` | A folder, FIFO, socket or device. |
| `empty` | A zero-byte file or response. |
| `too-large` | The file or response is larger than `plutobook.max_asset_size`, or would take the document past `plutobook.max_total_asset_size`. |
| `permission-denied` | PHP can't read the file. |
| `read-error` | Reading the file, or handing it to PlutoBook, failed. |
| `host-not-allowed` | The host, or a redirect's host, isn't in `allowedHosts`. |
| `blocked-address` | The host resolved to an internal address. |
| `http-error` | The final response status isn't 2xx. |
| `redirect-error` | Too many redirects, or a redirect from https to http or to another scheme. |
| `tls-error` | The TLS handshake or certificate check failed. |
| `timeout` | The download took longer than `plutobook.http_timeout`, the document used up `plutobook.http_document_timeout`, or PHP's time limit passed. |
| `network-error` | DNS, connection and other transfer failures, or a libcurl older than 7.68.0. |

</details>

PlutoBook also prints a `WARNING:` line to stderr for each skipped resource, such as
`WARNING: Asset skipped, outside the asset directory: file:///…`. The extension
can't turn this off.

### Errors

- Invalid arguments, such as an unknown page size, throw `ValueError` or `TypeError`.
- Rendering and writing failures throw `PlutoBook\Exception`, a
  `RuntimeException`. Its message says what went wrong when that is known, for
  example `Unable to write the PDF output: the destination stream did not accept the data`.
- Exceptions thrown by a PHP stream wrapper reach your code unchanged.
- Documents can't be cloned or serialized.

### Time limits

Rendering counts toward `max_execution_time`. PHP can't stop native code midway,
so the extension checks the limit each time PlutoBook loads a resource or writes
part of the PDF. Once the limit has passed, rendering stops with PHP's usual
`Maximum execution time` error, and shutdown functions still run.

The layout step before the first page is written can't be stopped this way. If it
runs past the limit plus `hard_timeout`, PHP ends the process. For very large
documents, raise the limit with `set_time_limit()` or render them in a queue worker.

## Configuration

All settings can go in `php.ini`. The last one can't be changed with `ini_set()`;
it belongs in `php.ini`, `.user.ini` or an FPM pool.

| Setting | Default | What it does |
| --- | --- | --- |
| `plutobook.max_asset_size` | `32M` | Largest single file or download, after decompression. |
| `plutobook.max_total_asset_size` | `128M` | Most bytes one document may load in total. |
| `plutobook.http_timeout` | `10` | Seconds per download, redirects included (1–3600). |
| `plutobook.http_document_timeout` | `30` | Seconds of network time per document (up to 86400). |
| `plutobook.http_max_redirects` | `5` | Redirects followed per download (0–20). |
| `plutobook.http_ca_file` | empty | CA bundle to trust instead of libcurl's default. |
| `plutobook.allow_private_addresses` | `Off` | Allow downloads from internal addresses. |

- **Sizes** accept `K`, `M` and `G`, and `0` removes the limit.
  `plutobook.max_asset_size` can be at most 2147483647 bytes, PlutoBook's own
  limit, so `2G` is one byte too many.
- **Invalid values** raise a warning and keep the previous setting.
- **Downloaded and loaded files** use memory outside `memory_limit`, which is why
  the size limits exist. Each `loadHtml()` starts a new total. Files that would go
  over it are skipped as `too-large` with the detail
  `plutobook.max_total_asset_size reached`.
- **Network time** is real time, which `max_execution_time` doesn't count on Linux.
  The document's budget starts at `loadHtml()` and includes fonts and images
  fetched while rendering. `0` turns it off. When it runs out, the remaining
  downloads are skipped as `timeout` with the detail
  `plutobook.http_document_timeout reached`.
- **DNS lookups** count toward both timeouts only when libcurl can look up names
  in the background, as most libcurl builds can. `php --ri plutobook` shows
  `DNS lookups => bounded by the download timeouts` when it can. Otherwise a slow
  DNS server can hold a download past its timeout, and the `timeout` error's
  detail says how long the lookup took.
- **Changes** made with `ini_set()` apply to every later load, including the ones
  that happen while rendering.

## Security

Templates often contain user data, so the extension is strict by default.

**Local files**

- Files are only read from the `baseUrl` or `documentRoot` folder and its
  subfolders, and PHP's `open_basedir` is respected.
- On Linux, symlinks within the folder work, and symlinks leading out of it are
  refused. A path swapped for a symlink after the checks is refused too. None of
  this needs `/proc`, so it works in an FPM `chroot`.
- On Windows, the folder is pinned while in use, and junctions, symlinks, UNC
  paths, device names and alternate data streams are refused.
- Devices, FIFOs and sockets are never read. On Linux with `/proc` mounted, a file
  swapped for one after the checks isn't even opened. Without `/proc`, such as in an
  FPM `chroot`, it can be opened for a moment, without waiting on it, and is then
  closed unread.

**Downloads**

- Only `http` and `https` are used, and only for hosts in `allowedHosts`.
- Every redirect is checked against `allowedHosts`. Redirects from https to http,
  or to other schemes, are refused.
- TLS 1.2 or newer is required, and certificates and host names are always verified.
- Proxy variables such as `http_proxy` are ignored, and no cookies are kept. Each
  document reuses its own connections.
- Internal addresses are refused: loopback, private networks (10/8, 172.16/12,
  192.168/16, 100.64/10, fc00::/7), link-local addresses (169.254/16, including
  cloud metadata, and fe80::/10), multicast, reserved and unspecified addresses.
  The check uses the address actually connected to, so DNS tricks don't get
  around it. An administrator can allow them with `plutobook.allow_private_addresses`.

## Limitations

- **No JavaScript and no CSS Grid.** Both are on
  [PlutoBook's roadmap](https://github.com/plutoprint/plutobook/blob/v0.20.0/README.md#roadmap).
- **One page size per document.** Only the first page's size and margins are used,
  and `@page :first` is the only page selector that works reliably.
- **Memory with older system libraries.** Cairo 1.18 and PlutoBook 0.20.0 leak
  memory on every render
  ([Cairo #1000](https://gitlab.freedesktop.org/cairo/cairo/-/issues/1000),
  [PlutoBook #84](https://github.com/plutoprint/plutobook/issues/84)), about
  19 KiB per invoice. The ready-made Linux and Windows builds include the fixes. If
  you compiled against unpatched system libraries, restart FPM workers regularly
  with `pm.max_requests`.

## Contributing

[CONTRIBUTING.md](CONTRIBUTING.md) covers building from a checkout, the tests, the
binary and package builds, and the release process.

## License

The extension is licensed under Apache-2.0. PlutoBook, which it links to, is
licensed under MPL-2.0. See [LICENSE](LICENSE) and
[THIRD_PARTY.md](THIRD_PARTY.md).
