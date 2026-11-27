<div align="center">

# PlutoBook for PHP

[![PHP 8.2–8.6](https://img.shields.io/badge/PHP-8.2%E2%80%938.6-777BB4?logo=php&logoColor=white)](composer.json)
[![Platforms: Linux NTS and Windows x64](https://img.shields.io/badge/platform-Linux%20NTS%20%7C%20Windows%20x64-444)](#installation)
[![PlutoBook 0.20.0](https://img.shields.io/badge/PlutoBook-0.20.0-29688A)](#building-against-system-libraries)
[![Packagist: oblakhost/ext-plutobook](https://img.shields.io/packagist/v/oblakhost/ext-plutobook?label=PIE&logo=packagist&logoColor=white)](https://packagist.org/packages/oblakhost/ext-plutobook)

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

## Installation

Install the extension with [PIE](https://github.com/php/pie), configured for the
PHP installation that should load it:

```sh
pie install oblakhost/ext-plutobook
php --ri plutobook
```

PIE installs a pre-built module when the release has one for that PHP build and
otherwise compiles the release's source archive. Every
[GitHub release](https://github.com/oblakhost/php-plutobook/releases) carries the
source archive, the Linux and Windows PIE binaries, deb and rpm packages, the
source of the MPL-covered code in the binaries and signed checksums.

### Linux with PIE

PIE installs the release's pre-built module on x86_64 Linux with glibc 2.28 or
newer, non-thread-safe PHP 8.2 to 8.6 and libcurl 7.68.0 or newer:
`php_plutobook-1.0.0_php8.4-x86_64-linux-glibc-nts.zip` for PHP 8.4.
Fontconfig and a font package such as DejaVu must be installed. Elsewhere, for
example with thread-safe PHP, musl or configure options, PIE builds the release's
source archive, which needs PlutoBook 0.20.0 and libcurl development files (see
[Building against system libraries](#building-against-system-libraries)). On EL8,
whose libcurl is too old for remote assets, install the EL8 rpm instead.

### deb and rpm packages

Download the package for your PHP minor version from the
[latest release](https://github.com/oblakhost/php-plutobook/releases/latest) and
install it with the system package manager:

```sh
sudo apt install ./php8.4-plutobook_1.0.0-1_amd64.deb            # Debian and Ubuntu
sudo dnf install ./php-plutobook-1.0.0-1.php84.el9.x86_64.rpm    # EL8, EL9 and EL10
```

| Package | For | Installs |
| --- | --- | --- |
| `php8.4-plutobook_1.0.0-1_amd64.deb` | Debian and Ubuntu with the distribution's PHP or Ondřej Surý's | `/usr/lib/php/API/plutobook.so`, `/etc/php/8.4/mods-available/plutobook.ini`, enabled with `phpenmod` |
| `php-plutobook-1.0.0-1.php84.el9.x86_64.rpm` | EL8, EL9 and EL10 with PHP in `/usr` (AppStream or Remi's modular streams), with one file per EL release | `/usr/lib64/php/modules/plutobook.so`, `/etc/php.d/40-plutobook.ini` |

Packages require the matching PHP API (`phpapi-API`, `php(api) = API-64`), the
system font configuration with DejaVu fonts, and libcurl 7.68.0 or newer, or
GnuTLS and Nettle for the EL8 packages, which carry the `el8` module. Restart
PHP-FPM after installing.

### Pre-built Linux modules

The PIE binaries and the packages carry a self-contained `plutobook.so` for
Linux x86_64, one per PHP minor version. Both variants use the system's glibc
and differ in where libcurl comes from:

- **`linux`** runs on glibc 2.28 or newer (EL9 and later, Debian 10 and later, Ubuntu
  20.04 and later) with the system libcurl 7.68.0 or newer. PIE installs it, and the
  deb packages and the EL9 and EL10 rpm packages carry it.
- **`el8`** is for AlmaLinux, Rocky and CloudLinux 8, whose libcurl 7.61.1 is older
  than the binding supports. A `linux` module still loads and renders there, but
  skips remote assets as `network-error`. The `el8` module contains libcurl
  8.23.0 (HTTP and HTTPS only) and uses the system GnuTLS. PIE cannot tell EL8
  from other glibc systems, so the `el8` modules ship in the EL8 packages only.

Each module contains PlutoBook, Cairo, pixman, HarfBuzz, FreeType, Fontconfig,
libpng, zlib, Expat, ICU, libjpeg-turbo and libwebp, with fixes for two upstream
memory leaks (see [Limitations](#limitations)). [THIRD_PARTY.md](THIRD_PARTY.md)
lists their licenses. ICU's data is trimmed to the parts PlutoBook uses, and text
in 25 languages and ten scripts renders the same as with the full data. The module
exports only `get_module`, so its copies of these libraries stay separate from the
ones gd, imagick or intl load. It needs only glibc, the GCC 8 `libstdc++` and
`libgcc_s`, and libcurl or GnuTLS from the system.

The bundled Fontconfig reads the system configuration in `/etc/fonts` and needs a
font package such as DejaVu. Its cache format (version 12) differs from the
system Fontconfig's, so it scans the font directories once per PHP process unless
it can write its own cache, for example in `/var/cache/fontconfig` as root or in
the PHP user's home directory. The scan took 15 ms with 38 fonts and 120 ms with
263 fonts on AlmaLinux 10. A patch keeps it from printing "No writable cache
directories" for every font directory; `FC_DEBUG=16` shows the message again.

### Windows with PIE

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
System Cairo 1.18 and PlutoBook 0.20.0 leak memory on every render unless they
carry the fixes described under [Limitations](#limitations).

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
rpm -K php-plutobook-1.0.0-1.php84.el9.x86_64.rpm
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
installed fonts.

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

## Limitations

JavaScript and CSS Grid are roadmap items in the pinned renderer and do not work
in PlutoBook 0.20.0.
[PlutoBook 0.20.0 feature overview](https://github.com/plutoprint/plutobook/blob/v0.20.0/README.md)

Cairo 1.18 leaks PDF font options, and PlutoBook 0.20.0 passes a variation
string even for fonts with no variation axes. With the upstream libraries, the
process working set grew by about 19 KiB per invoice over 800 renders. The Linux
portable modules and Windows packages carry both fixes, and resident memory stays
flat over repeated renders. For installations using affected system libraries,
recycle FPM workers with `pm.max_requests`.
[Cairo #1000](https://gitlab.freedesktop.org/cairo/cairo/-/issues/1000)
· [PlutoBook #84](https://github.com/plutoprint/plutobook/issues/84)

## Contributing

Building from a checkout, the test suites, the binary and package builds and the
release process are described in [CONTRIBUTING.md](CONTRIBUTING.md).

## License

The binding uses Apache-2.0. The separately linked PlutoBook library uses MPL-2.0.
[License](LICENSE) · [Third-party notices](THIRD_PARTY.md)
