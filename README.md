# php-pdf

`plutobook` is a compiled PHP extension that turns HTML and CSS into PDF bytes
inside the PHP process. Each `PlutoBook\Document` owns a native PlutoBook
document. The initial target is Linux NTS PHP 8.2–8.6 and PlutoBook 0.20.0.

```php
$document = new PlutoBook\Document(pageSize: 'A4', margins: '15mm');
$document->loadHtml(
    '<h1>Invoice 42</h1><p>Total: 120.00 EUR</p>',
    css: 'h1 { color: #194572 }',
);

$pdf = $document->toPdf();                    // Binary PHP string
$document->writeToFile(__DIR__ . '/42.pdf');  // Local filesystem path
// With an already open writable PHP stream: $document->writeToStream($stream);
```

## Build and run locally

From the repository root:

```sh
scripts/build-php 8.2.36
scripts/build-plutobook
scripts/build-extension 8.2.36
.local/php/8.2.36/nts/bin/php -n \
  -d "extension=$PWD/build/extension/8.2.36/nts/modules/plutobook.so" --ri plutobook
scripts/test-extension 8.2.36
scripts/test-invoice 8.2.36
```

The last command leaves a two-page A4 invoice, a one-page landscape Letter quotation,
page images, extracted text and memory measurements in the printed `artifacts/invoice/` directory. It exercises
both CLI and a real FastCGI POST. The synthetic fixture includes 32 line items,
a local SVG logo, Serbian Latin and Cyrillic text, and pinned DejaVu fonts.

Build prerequisites include Bash, GCC/G++ with C++20, Make, Autoconf, pkg-config,
CMake, Ninja, Python 3, curl, tar, flock, jq, and development headers for
OpenSSL, zlib, libxml2, iconv, Cairo (PDF/PNG/FreeType/Fontconfig), FreeType,
Fontconfig, HarfBuzz and ICU. The renderer script supplies pinned private builds
of missing Expat, TurboJPEG and WebP dependencies. PDF inspection uses Poppler.
QA uses Python venv/pip and unzip or 7z for Composer.

All PHP builds, binaries, headers and ini files live under this checkout.
Builds use existing non-PHP host tools and libraries read-only. Each extension
build uses its own absolute `phpize`, `php-config` and CLI. Build configuration,
compiler details and private FPM request evidence are under `artifacts/`.
[Source and tool pins](tools/sources.sha256)

## Document API

- `__construct(string $pageSize = "A4", string $orientation = "portrait", string $margins = "15mm")`
  accepts A3, A4, A5, B4, B5, Letter, Legal and Ledger, with portrait or landscape
  orientation. Margins use CSS shorthand (top, right, bottom, left) with one to
  four nonnegative lengths in pt, mm, cm, in, px or pc. Unitless zero is valid.
  A stylesheet's `@page` rules can change the initial page settings.
- `loadHtml(string $html, ?string $baseUrl = null, string $css = "", ?string $documentRoot = null): void`
  loads UTF-8 HTML and an optional override stylesheet. Calling it again replaces
  the document content. Ordinary imperfect markup follows the renderer's HTML parser.
  `$baseUrl` and `$documentRoot` select local asset directories, as described in
  [Assets and fonts](#assets-and-fonts).
  PlutoBook ranks `$css` above the document's own styles. Its normal declarations
  override `<style>` elements, linked stylesheets and inline `style` attributes,
  whatever their specificity. Document `!important` declarations still beat normal
  `$css` declarations. Mark a `$css` declaration `!important` to override those as
  well. This differs from the CSS specification's user stylesheet rules, so
  `tests/unit/css-precedence.phpt` pins the PlutoBook 0.20.0 behavior.
- `toPdf(): string`, `writeToStream($stream): void`, and `writeToFile(string $path): void`
  render through native PDF write callbacks. Stream output starts at the current
  position. The caller retains the stream and handles final flushing or closing.
  The destination is borrowed exclusively while writing. File output uses normal
  create/truncate semantics, like gd, Imagick and `DOMDocument::save()`. An I/O
  failure or time limit can leave partial output. To replace a file only after a
  complete render, write to a temporary path in the same directory and `rename()` it.
- Rendering CPU time counts toward `max_execution_time`. PHP cannot interrupt native
  code, so the binding checks the time limit whenever the renderer fetches an asset
  or writes output. Once the limit passes, it stops rendering and raises PHP's
  normal `Maximum execution time` fatal error, and shutdown functions still run.
  Layout before the first output byte cannot be interrupted. If layout outlasts the
  limit plus `hard_timeout`, PHP terminates the process. Raise `set_time_limit()`
  for very large documents, or render them in a queue worker.
- `getResourceErrors(): array` lists the resources skipped by the most recent
  `loadHtml()` and any rendering since, as `['url' => string, 'reason' => string]`
  entries. See [Assets and fonts](#assets-and-fonts) for the reasons.
- Invalid options raise `ValueError` or `TypeError`. Rendering and output failures
  raise `PlutoBook\Exception`, a `RuntimeException`. Its message names the cause
  when the binding or renderer knows it, for example
  `Unable to write the PDF output: the destination stream did not accept the data`.
  Exceptions from a PHP stream wrapper propagate to the caller.
  Documents support neither cloning nor serialization.

### Assets and fonts

By default documents can use inline `data:` resources. To load local CSS, images
or fonts, pass an existing directory as an absolute `file:///` URI with a trailing
slash. Percent-encode individual path components where needed.

```php
$assets = realpath(__DIR__ . '/assets');
if ($assets === false) {
    throw new RuntimeException('Missing invoice assets directory');
}
$baseUrl = 'file://' . implode('/', array_map('rawurlencode', explode('/', $assets))) . '/';
$document->loadHtml('<img src="logo.svg"><p>Statement</p>', baseUrl: $baseUrl);
```

With only `baseUrl`, `file:` URLs are filesystem paths, so `/assets/logo.svg`
means `/assets/logo.svg` on the server and is refused. Templates written for a
website usually use such root-relative URLs. For those, pass the site's public
directory as `documentRoot`, again a `file:///` URI ending in `/`. That directory
then acts as `/`, as on a web server. `baseUrl` becomes optional and must point to
a directory inside it. It defaults to the document root and determines what
relative URLs such as `logo.svg` resolve against.

```php
$document->loadHtml(
    '<img src="/assets/logo.svg"><img src="stamp.png">',
    baseUrl: 'file:///srv/shop/public/invoices/',  // stamp.png -> public/invoices/stamp.png
    documentRoot: 'file:///srv/shop/public/',      // /assets/… -> public/assets/…
);
```

In this mode every `file:` URL in the document is relative to the document root,
including absolute ones such as `file:///srv/shop/…`. `..` cannot climb above it.
The same template therefore resolves the same way whether it is rendered from
these local files or from the live site. `getResourceErrors()` reports the URLs as
the document saw them, for example `file:///assets/missing.png`.

The asset loader confines regular files to the selected tree and checks PHP's
`open_basedir`. Symlinks inside the tree work, while those leading out of it are
refused. After those checks it reopens the resolved path one component at a time
without following symlinks, so a path swapped for a symlink in the meantime is
refused. It does not need `/proc`, so it also works in an FPM `chroot`. Devices,
FIFOs and sockets are inspected but never opened for reading. Network requests
are disabled. Missing or denied resources are
omitted while surrounding content renders. The renderer also uses Fontconfig for
installed fonts. The fixture scripts select a private Fontconfig file for
repeatable output.

Each skipped resource appears once in `getResourceErrors()` with one of these
reasons. Fonts and background images can be fetched during rendering, so check
the list after `toPdf()` or a write method. To make missing assets fatal, throw
when the list is not empty.

| Reason | Meaning |
| --- | --- |
| `network-disabled` | An `http:` or `https:` URL. Network requests are disabled. |
| `unsupported-scheme` | Any other non-`file:` URL. |
| `no-base-url` | A local file without a `baseUrl` or `documentRoot` directory. |
| `invalid-url` | A malformed `file:` URL, such as one with an encoded NUL byte. |
| `missing` | The file does not exist. |
| `open-basedir` | PHP's `open_basedir` blocks the file. |
| `outside-base` | The resolved file lies outside the `documentRoot`, or the `baseUrl` directory without one. |
| `verification-failed` | A directory or the file changed between the checks and the read. |
| `not-regular-file` | A directory, FIFO, socket or device. |
| `empty` | A zero-byte file. |
| `too-large` | The file exceeds `plutobook.max_asset_size`. |
| `permission-denied` | The PHP process cannot read the file. |
| `read-error` | Reading or handing the file to the renderer failed. |

PlutoBook 0.20.0 also prints one `WARNING:` line per skipped resource to the
process's stderr. The binding cannot suppress that output. The line names the
reason and URL, for example
`WARNING: Asset skipped, outside the asset directory: file:///…`.

Asset bytes are native allocations outside `memory_limit`. The
`plutobook.max_asset_size` setting caps each local file. It defaults to `32M`,
accepts PHP size shorthand (`K`, `M`, `G`) and can be changed with `ini_set()`.
Larger files are skipped like missing resources. `0` removes the binding's cap.
Values must stay at or below 2147483647 bytes, the renderer's own limit, so `2G`
is rejected. Invalid values raise a warning and keep the previous setting.
The current value applies to every fetch, including fonts and images that
PlutoBook loads while it renders.

## Verification and distribution

`scripts/build-php` and `scripts/verify-php` with no arguments build and check
every pinned NTS runtime. Pass its exact PHP version to the extension build
and test commands. `debug` and `ubsan` are separate build flavors.
`scripts/bootstrap-qa`, `scripts/check-php` and `scripts/unit-coverage` run the
PHP and native quality checks, including the 85% unit line-coverage gate.
[Development conventions](CONTRIBUTING.md)

The Linux NTS matrix covers synthetic invoices. ZTS is unsupported. JavaScript
and CSS Grid are roadmap items in the pinned renderer. Comparison with the
original WHMCS/Gotenberg output awaits the real fixture.
[PlutoBook 0.20.0 feature overview](https://github.com/plutoprint/plutobook/blob/v0.20.0/README.md)

The PIE package name is `oblakhost/php-pdf`. Semantic-release prepares GitHub
source releases after the CI matrix passes. Public Packagist registration
requires an accessible repository. PECL directs new extensions to PIE.
[PECL publishing policy](https://pecl.php.net/account-request.php)

The binding uses Apache-2.0. The separately linked PlutoBook library uses MPL-2.0.
[License](LICENSE) · [Third-party notices](THIRD_PARTY.md)
