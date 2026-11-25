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
- `loadHtml(string $html, ?string $baseUrl = null, string $css = ""): void`
  loads UTF-8 HTML and optional additional CSS. Calling it again replaces the
  document content. Ordinary imperfect markup follows the renderer's HTML parser.
- `toPdf(): string`, `writeToStream($stream): void`, and `writeToFile(string $path): void`
  render through native PDF write callbacks. Stream output starts at the current
  position. The caller retains the stream and handles final flushing or closing.
  The destination is borrowed exclusively while writing. File output uses normal
  create/truncate semantics. An I/O failure can leave partial output.
- Invalid options raise `ValueError` or `TypeError`. Rendering and output failures
  raise `PlutoBook\Exception`, a `RuntimeException`. Exceptions from a PHP stream
  wrapper propagate to the caller. Documents support neither cloning nor serialization.

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

The asset loader confines regular files to the selected tree, checks PHP's
`open_basedir`, and verifies Linux file descriptors through `/proc/self/fd`
before reading. Network requests are disabled. Missing or denied resources can
be omitted while surrounding content renders. PlutoBook can log these omissions
to stderr. The renderer also uses Fontconfig for installed fonts. The fixture
scripts select a private Fontconfig file for repeatable output.

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
