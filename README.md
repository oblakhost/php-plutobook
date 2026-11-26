<div align="center">

# PlutoBook for PHP

[![PHP 8.2–8.6](https://img.shields.io/badge/PHP-8.2%E2%80%938.6-777BB4?logo=php&logoColor=white)](composer.json)
[![Platforms: Linux NTS and Windows x64](https://img.shields.io/badge/platform-Linux%20NTS%20%7C%20Windows%20x64-444)](#verification-and-distribution)
[![PlutoBook 0.20.0](https://img.shields.io/badge/PlutoBook-0.20.0-29688A)](#building-against-system-libraries)

[![CI: PHPT, CLI and FPM](https://img.shields.io/badge/CI-PHPT%20%2F%20CLI%20%2F%20FPM-2088FF?logo=githubactions&logoColor=white)](.github/workflows/toolchains.yml)
[![Unit line coverage gate: 85%](https://img.shields.io/badge/unit%20coverage%20gate-85%25-444)](CONTRIBUTING.md#testing)
[![semantic-release: conventional](https://img.shields.io/badge/semantic--release-conventional-e10079?logo=semantic-release)](release.config.mjs)
[![License: Apache-2.0](https://img.shields.io/badge/license-Apache--2.0-blue)](LICENSE)

</div>

`plutobook` is a compiled PHP extension that turns HTML and CSS into PDF bytes
inside the PHP process. Each `PlutoBook\Document` owns a native PlutoBook
document. The extension targets PHP 8.2–8.6 with PlutoBook 0.20.0 on Linux
(NTS) and Windows x64 (NTS and TS).

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
of missing Expat, TurboJPEG and WebP dependencies. It also builds libcurl, HTTP
and HTTPS only, when the host has no libcurl development files with HTTPS.
PDF inspection uses Poppler.
QA uses Python venv/pip and unzip or 7z for Composer.

All PHP builds, binaries, headers and ini files live under this checkout.
Builds use existing non-PHP host tools and libraries read-only. Each extension
build uses its own absolute `phpize`, `php-config` and CLI. Build configuration,
compiler details and private FPM request evidence are under `artifacts/`.
[Source and tool pins](tools/sources.sha256)

### Native Windows build over SSH

From this checkout, run the full build or select a shorter check:

```sh
scripts/test-winbuild 8.4.28
scripts/test-winbuild 8.4.28 phpt tests/unit/windows-junctions.phpt
scripts/test-winbuild 8.4.28 php --ri plutobook
```

The command connects to the existing `winbuild` OpenSSH alias, including for file
transfer. It sends the working copies of the extension and tests, so local edits
can be tested before a commit. The host needs x64 Visual Studio 2022 C++ Build
Tools with a Windows SDK, CMake, Ninja, Python 3.11+, and Windows tar. The
Linux workstation uses Bash, Git, OpenSSH, GNU tar, Python 3, Poppler and a
container engine. It cross-compiles the patched renderer once and reuses it
across the native PHP builds.

The first run builds a private x64 NTS PHP CLI and caches it with the pinned
dependencies under `%USERPROFILE%\pdf-ext`. Each invocation builds a fresh DLL
against the matching PHP headers. The full run exercises the Windows CLI and
PHPT suite, renders the invoice on Windows, and inspects the resulting PDF on
the workstation. The printed directory under `artifacts/winbuild/8.4.28/`
contains the DLL, build log, PDFs and the portable development ZIP. A verified
copy of the ZIP also lives in `%USERPROFILE%\pdf-ext\artifacts` on the host. Extract the
ZIP on Windows x64 with the Microsoft Visual C++ v14 x64 runtime installed.
From the extracted folder, use the included launcher:

```bat
php-plutobook.cmd --ri plutobook
php-plutobook.cmd your-script.php
```

PHP-FPM and the native line-coverage gate run on Linux.

### Linux with PIE

`pie install oblakhost/php-plutobook` installs the release's pre-built module on
x86_64 Linux with glibc 2.28 or newer, non-thread-safe PHP 8.2 to 8.6 and libcurl
7.68.0 or newer: `php_plutobook-1.0.0_php8.4-x86_64-linux-glibc-nts.zip` for PHP 8.4.
Fontconfig and a font package such as DejaVu must be installed. Elsewhere, for
example with thread-safe PHP, musl or configure options, PIE builds the release's
source archive, which needs PlutoBook 0.20.0 and libcurl development files (see
[Building against system libraries](#building-against-system-libraries)). On EL8,
whose libcurl is too old for remote assets, install the EL8 rpm instead.

### Windows release packages

With [PIE](https://github.com/php/pie) configured for the target PHP installation, run

```sh
pie install oblakhost/php-plutobook
php --ri plutobook
```

PIE selects a ZIP by PHP minor version, thread safety, compiler and architecture.
For example, `php_plutobook-1.0.0-8.4-nts-vs17-x86_64.zip` is for 64-bit
non-thread-safe PHP 8.4 built with Visual Studio 2022. PIE installs
`php_plutobook.dll` in PHP's extension directory, `libplutobook-0.dll` next to
`php.exe`, and the fonts, configuration and license files under
`extras\plutobook`. The binding includes libcurl and zlib and uses Windows
Schannel for HTTPS.

An explicit `FONTCONFIG_FILE` or `FONTCONFIG_PATH` takes precedence over the
packaged fonts. If the PHP directory contains characters outside the Windows
ANSI code page and its volume has 8.3 filenames disabled, copy `fonts.conf`
and its adjacent `fonts` directory to a representable path and point
`FONTCONFIG_PATH` there.

To build a release ZIP from the current checkout and test its installed layout on
`winbuild`, pass the extension version and thread mode explicitly.

```sh
scripts/test-winbuild 8.4.28 release 1.0.0 nts
scripts/test-winbuild 8.4.28 release 1.0.0 ts
```

## Document API

- `__construct(string $pageSize = "A4", string $orientation = "portrait", string $margins = "15mm")`
  accepts A3, A4, A5, B4, B5, Letter, Legal and Ledger, with portrait or landscape
  orientation. Margins use CSS shorthand (top, right, bottom, left) with one to
  four nonnegative lengths in pt, mm, cm, in, px or pc. Unitless zero is valid.
  A stylesheet's `@page` rules can change the initial page settings.
- `loadHtml(string $html, ?string $baseUrl = null, string $css = "", ?string $documentRoot = null, array $allowedHosts = []): void`
  loads UTF-8 HTML and an optional override stylesheet. Calling it again replaces
  the document content. Ordinary imperfect markup follows the renderer's HTML parser.
  `$baseUrl` and `$documentRoot` select local asset directories, as described in
  [Assets and fonts](#assets-and-fonts). `$baseUrl` can also be an `http(s)://` page
  URL, and `$allowedHosts` opts the document into [remote assets](#remote-assets).
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
`open_basedir`. On Linux, symlinks inside the tree work, while those leading out of it are
refused. After those checks it reopens the resolved path one component at a time
without following symlinks, so a path swapped for a symlink in the meantime is
refused. It does not need `/proc`, so it also works in an FPM `chroot`. Devices,
FIFOs and sockets are inspected but never opened for reading. Network requests
are disabled unless the document lists `allowedHosts`. Missing or denied resources are
omitted while surrounding content renders. The renderer also uses Fontconfig for
installed fonts. The fixture scripts select a private Fontconfig file for
repeatable output.

On Windows, use a local drive URL such as `file:///C:/invoices/assets/`.
The loader pins the directory by handle and refuses reparse points (including
junctions and symlinks), UNC paths, device names and alternate data streams.

Each skipped resource appears once in `getResourceErrors()` as
`['url' => …, 'reason' => …, 'detail' => …]`. `reason` is one of the stable codes
below, and `detail` is human-readable text such as `HTTP 404`. Fonts and
background images can be fetched during rendering, so check the list after
`toPdf()` or a write method. To make missing assets fatal, throw when the list is
not empty.

| Reason | Meaning |
| --- | --- |
| `network-disabled` | An `http:` or `https:` URL in a document without `allowedHosts`. |
| `unsupported-scheme` | Any other non-`file:` URL. |
| `no-base-url` | A local file without a `baseUrl` or `documentRoot` directory. |
| `invalid-url` | A malformed URL, such as a `file:` URL with an encoded NUL byte. |
| `missing` | The file does not exist. |
| `open-basedir` | PHP's `open_basedir` blocks the file. |
| `outside-base` | The resolved file lies outside the `documentRoot`, or the `baseUrl` directory without one. |
| `verification-failed` | A directory or the file changed between the checks and the read. |
| `not-regular-file` | A directory, FIFO, socket or device. |
| `empty` | A zero-byte file or response body. |
| `too-large` | The file or response exceeds `plutobook.max_asset_size`. |
| `permission-denied` | The PHP process cannot read the file. |
| `read-error` | Reading or handing the data to the renderer failed. |
| `host-not-allowed` | The host, or a redirect target's host, is not in `allowedHosts`. |
| `blocked-address` | The host resolved to an internal address. |
| `http-error` | The final response status is not 2xx. |
| `redirect-error` | Too many redirects, a redirect from https to http, or to another scheme. |
| `tls-error` | The TLS handshake or certificate verification failed. |
| `timeout` | The request exceeded `plutobook.http_timeout`, or PHP's time limit passed. |
| `network-error` | DNS, connection and other transfer failures, or a libcurl older than 7.68.0. |

PlutoBook 0.20.0 also prints one `WARNING:` line per skipped resource to the
process's stderr. The binding cannot suppress that output. The line names the
reason and URL, for example
`WARNING: Asset skipped, outside the asset directory: file:///…`.

Asset bytes are native allocations outside `memory_limit`. The
`plutobook.max_asset_size` setting caps each local file and each remote response
body after decompression. It defaults to `32M`,
accepts PHP size shorthand (`K`, `M`, `G`) and can be changed with `ini_set()`.
Larger files are skipped like missing resources. `0` removes the binding's cap.
Values must stay at or below 2147483647 bytes, the renderer's own limit, so `2G`
is rejected. Invalid values raise a warning and keep the previous setting.
The current value applies to every fetch, including fonts and images that
PlutoBook loads while it renders.

`plutobook.max_total_asset_size` caps the bytes one document loads across all of
its local and remote assets, including repeated fetches of the same file under
different URLs. It defaults to `128M`, accepts the same shorthand and `0` removes
it. Assets that would cross the cap are skipped as `too-large` with the detail
`plutobook.max_total_asset_size reached`. Each `loadHtml()` starts a new budget.

### Remote assets

Documents load nothing over the network unless `loadHtml()` lists the hosts
they may contact. An empty list, the default, keeps the network off.

```php
$document->loadHtml(
    $html,
    baseUrl: 'https://shop.example/invoices/42/',
    allowedHosts: ['shop.example', 'fonts.gstatic.com', '*.cdn.example'],
);
```

- **Host patterns.** Each entry is an exact host name such as `shop.example`, a
  `*.example.com` pattern that matches its subdomains (but not `example.com`
  itself), a bracketed IPv6 literal such as `[2001:db8::1]`, or `*` for any host.
  Use the ASCII (punycode) form of international domain names.
- **Remote `baseUrl`.** An `http://` or `https://` page URL resolves relative and
  root-relative references the way a browser does. It grants no access to local
  files, and it cannot be combined with `documentRoot`. A local `baseUrl` or
  `documentRoot` can still be combined with `allowedHosts` for absolute
  `https://` assets.
- **Request policy.** Only `http` and `https` are used. Redirects are followed by
  the binding, which checks every hop against `allowedHosts` and refuses
  redirects from https to http or to other schemes. TLS certificates and host
  names are always verified, with TLS 1.2 or newer. Proxy environment variables
  such as `http_proxy` are ignored, and no cookies are stored. Each document
  reuses its own connections across assets.
- **Internal addresses.** Connections to loopback, private (10/8, 172.16/12,
  192.168/16, 100.64/10, fc00::/7), link-local (169.254/16 including cloud
  metadata, fe80::/10), multicast, reserved and unspecified addresses are
  refused. The check uses the address actually connected to, so DNS names
  cannot bypass it. An operator can allow them with
  `plutobook.allow_private_addresses`.
- **Dependencies.** The binding uses the libcurl C library directly. PHP's `curl`
  extension is not needed.

| Setting | Default | Changeable | Meaning |
| --- | --- | --- | --- |
| `plutobook.http_timeout` | `10` | `ini_set()` | Seconds per asset, including redirects (1–3600). |
| `plutobook.http_document_timeout` | `30` | `ini_set()` | Network seconds per document across all its assets (0 = no cap, up to 86400). |
| `plutobook.http_max_redirects` | `5` | `ini_set()` | Redirects followed per asset (0–20). |
| `plutobook.http_ca_file` | empty | `ini_set()` | CA bundle to trust instead of libcurl's default. |
| `plutobook.allow_private_addresses` | `Off` | php.ini, `.user.ini`, FPM pool | Allow internal addresses. |

Network time is wall-clock time, which PHP's `max_execution_time` does not count
on Linux. Each asset gets at most `http_timeout` seconds. All remote assets of a
document share `http_document_timeout` seconds, counted from `loadHtml()` and
including fonts and images fetched while rendering. Once that budget is spent,
the remaining remote assets are skipped as `timeout` with the detail
`plutobook.http_document_timeout reached`.

### Building against system libraries

Distribution packages and PIE installs build the extension with the usual
`phpize`, `./configure --with-plutobook` and `make`. `configure` reads its
dependencies through `pkg-config`:

- **PlutoBook 0.20.0** (`plutobook.pc`). It builds with Meson. This project uses
  `-Ddefault_library=shared -Dturbojpeg=enabled -Dwebp=enabled`. Use `-Dcurl=disabled`:
  the binding fetches remote assets itself and never uses PlutoBook's HTTP client.
- **libcurl 7.68.0 or newer with HTTP and HTTPS** (`libcurl.pc`, for example
  `libcurl4-openssl-dev` or `libcurl-devel`). `configure` stops with an error if
  libcurl is missing or lacks either protocol. If the libcurl loaded at run time is
  older than 7.68.0, remote assets are skipped as `network-error` and `--ri
  plutobook` shows the version found.

Standard `CPPFLAGS`, `CFLAGS`, `LDFLAGS` and `PKG_CONFIG_PATH` variables apply to
`configure`. Meson and CMake take `CFLAGS`, `CXXFLAGS` and `LDFLAGS` the same way.
`scripts/build-plutobook` passes `LDFLAGS` on to the renderer's link step.

### Portable Linux modules

Binary releases and packages use a self-contained `plutobook.so` per PHP minor
version for Linux x86_64. Build them with Podman or Docker:

```sh
scripts/build-portable            # every pinned PHP release
scripts/build-portable 8.3.37     # one release; --skip-tests skips the PHPT and invoice suites
```

The build runs offline in an AlmaLinux 8 container with GCC 14 and tests each
module there with the PHPT and invoice suites. It writes
`build/portable/dist/VARIANT/phpX.Y/plutobook.so` (about 12 MB), a separate
`plutobook.so.debug`, and the license notices of the bundled
libraries in `build/portable/dist/licenses/`. Logs are in
`build/portable/root/artifacts/portable/`. Both variants use the system's glibc
and differ in where libcurl comes from:

- **`linux`** runs on glibc 2.28 or newer (EL9 and later, Debian 10 and later, Ubuntu
  20.04 and later) with the system libcurl 7.68.0 or newer. It compiles against the
  libcurl 7.68.0 headers and its tests run against that release.
- **`el8`** is for AlmaLinux, Rocky and CloudLinux 8, whose libcurl 7.61.1 is older
  than the binding supports. A `linux` module still loads and renders there, but
  skips remote assets as `network-error`. The `el8` module contains libcurl
  8.23.0 (HTTP and HTTPS only) and uses the system GnuTLS, since libcurl 8.18 and
  later need OpenSSL 3.

`PLUTOBOOK_RELEASE_VERSION=1.2.0` embeds the release version and writes PIE's
pre-packaged binaries to `build/portable/release/`, one per PHP minor version:
`php_plutobook-1.2.0_php8.3-x86_64-linux-glibc-nts.zip` holds the `linux` module as
`plutobook.so`, with `LICENSE`, `THIRD_PARTY.md` and the bundled libraries'
`licenses/`. PIE names Linux binaries after the C library they need. PIE cannot tell
EL8 from other glibc systems, so the `el8` modules ship in the EL8 packages only.
`php_plutobook-1.2.0-mpl-sources.tar.xz` holds the source of the MPL-covered code
in the binaries, as [THIRD_PARTY.md](THIRD_PARTY.md) describes.

Each module contains PlutoBook with the static font variations fix (plutobook#84),
Cairo with the font options fix (cairo#1000), pixman, HarfBuzz, FreeType,
Fontconfig, libpng, zlib, Expat, ICU, libjpeg-turbo and libwebp, built from
PlutoBook's Meson wraps. `tools/portable/wraps` replaces Cairo's 2024 libpng, zlib
and pixman wraps with current releases. Versions are in `tools/sources.sha256`.
With both fixes, resident memory stays flat over the invoice suite's 80 renders.
ICU's data shrinks from 31.6 MB to 4.3 MB: PlutoBook needs only the break
iterators with their dictionaries, emoji properties and locale fallback data
(`tools/portable/icu-keep.txt`). Text in 25 languages and ten scripts renders the
same as with the full data.

The module exports only `get_module`, so its copies of these libraries stay
separate from the ones gd, imagick or intl load. It needs only glibc, the GCC 8
`libstdc++` and `libgcc_s`, and libcurl or GnuTLS from the system. The build checks
that, together with symbol versions, full RELRO, a non-executable stack and the
absence of run paths.

The bundled Fontconfig reads the system configuration in `/etc/fonts` and needs a
font package such as DejaVu. Its cache format (version 12) differs from the
system Fontconfig's, so it scans the font directories once per PHP process unless
it can write its own cache, for example in `/var/cache/fontconfig` as root or in
the PHP user's home directory. The scan took 15 ms with 38 fonts and 120 ms with
263 fonts on AlmaLinux 10. A patch keeps it from printing "No writable cache
directories" for every font directory; `FC_DEBUG=16` shows the message again.

### deb and rpm packages

`scripts/build-packages` wraps the portable modules with nFPM, one package per PHP
minor version, in `build/portable/packages/`. It takes the same
`PLUTOBOOK_RELEASE_VERSION` as the modules and refuses modules built without it.

| Package | For | Installs |
| --- | --- | --- |
| `php8.3-plutobook_1.2.0-1_amd64.deb` | Debian and Ubuntu with the distribution's PHP or Ondřej Surý's | `/usr/lib/php/API/plutobook.so`, `/etc/php/8.3/mods-available/plutobook.ini`, enabled with `phpenmod` |
| `php-plutobook-1.2.0-1.php83.el9.x86_64.rpm` | EL8, EL9 and EL10 with PHP in `/usr` (AppStream or Remi's modular streams) | `/usr/lib64/php/modules/plutobook.so`, `/etc/php.d/40-plutobook.ini` |

Packages require the matching PHP API (`phpapi-API`, `php(api) = API-64`), the
system font configuration with DejaVu fonts, and libcurl 7.68.0 or newer, or
GnuTLS and Nettle for the EL8 packages, which carry the `el8` module. Restart
PHP-FPM after installing. `PLUTOBOOK_PACKAGE_MAINTAINER` sets the Maintainer
field. `scripts/test-packages` installs each package in a clean container with
that distribution's PHP, or Surý's or Remi's where the distribution lacks the
version, renders a page, removes the package and checks that nothing is left.

### Verifying downloads

Every release carries `SHA256SUMS`, signed as `SHA256SUMS.asc`, and the rpm files
are signed. The key is `php-plutobook-signing-key.asc` in each release and
[`tools/keys/release-signing-key.asc`](tools/keys/release-signing-key.asc), with
the fingerprint `D80D 92A4 A247 D7F6 BF32 4E1A BFE0 F7EC 429C 222F`.

```sh
gpg --import php-plutobook-signing-key.asc
gpg --verify SHA256SUMS.asc SHA256SUMS
sha256sum --ignore-missing -c SHA256SUMS

sudo rpm --import php-plutobook-signing-key.asc   # EL: rpm -K then reports "signatures OK"
rpm -K php-plutobook-1.2.0-1.php83.el9.x86_64.rpm
```

## Verification and distribution

`scripts/build-php` and `scripts/verify-php` with no arguments build and check
every pinned NTS runtime. Pass its exact PHP version to the extension build
and test commands. `debug`, `ubsan` and `asan` are separate build flavors.
`debug` also fails the extension build on compiler warnings. `asan` builds PHP
and the extension with AddressSanitizer, using GCC when its runtime is installed
and Clang otherwise. Its unit run fails on any LeakSanitizer report outside
`tools/asan/lsan-suppressions.txt`.
`scripts/bootstrap-qa`, `scripts/check-php` and `scripts/unit-coverage` run the
PHP and native quality checks, including the 85% unit line-coverage gate.
[Development conventions](CONTRIBUTING.md)

Cairo 1.18 leaks PDF font options, and PlutoBook 0.20.0 passes a variation
string even for fonts with no variation axes. With the upstream Windows
renderer, the process working set grew by about 19 KiB per invoice over 800
renders. The Linux portable modules and Windows packages carry both fixes.
`tools/asan/render-loop.c` reproduces the native leak independently of PHP.
For installations using affected system libraries, recycle FPM workers with
`pm.max_requests`.
[Cairo #1000](https://gitlab.freedesktop.org/cairo/cairo/-/issues/1000)
· [PlutoBook #84](https://github.com/plutoprint/plutobook/issues/84)

The Linux NTS and Windows NTS and TS matrices cover synthetic invoices. JavaScript
and CSS Grid are roadmap items in the pinned renderer. Comparison with the
original WHMCS/Gotenberg output awaits the real fixture.
[PlutoBook 0.20.0 feature overview](https://github.com/plutoprint/plutobook/blob/v0.20.0/README.md)

The PIE package name is `oblakhost/php-plutobook`. After the CI matrix passes,
each release gets the source archive, the Linux and Windows PIE binaries, the deb
and rpm packages, the MPL sources and signed checksums. Packagist lists the
package once the repository is registered there. PECL directs new extensions to PIE.
[PECL publishing policy](https://pecl.php.net/account-request.php)

The binding uses Apache-2.0. The separately linked PlutoBook library uses MPL-2.0.
[License](LICENSE) · [Third-party notices](THIRD_PARTY.md)
