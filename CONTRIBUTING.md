# Contributing

## Native code and PHP API

- Put the extension build configuration and typed PHP stubs in `ext/` and
  handwritten native binding code in `ext/src/`. Use C11 and the php-src
  conventions for indentation, braces, and pointer placement. Prefix binding-owned
  C symbols with `php_plutobook_` and give internal helpers static linkage.
  [PHP coding standards](https://github.com/php/php-src/blob/master/CODING_STANDARDS.md)
- Use the `PlutoBook` namespace, PascalCase classes, and camelCase methods and
  parameters. Named argument names are part of the public API. Generate arginfo
  from typed stubs with a private source-built PHP.
- Each document object owns one native document. Pair acquisitions with releases
  on initialization failure, callback failure and destruction. Use explicit
  byte lengths for PDFs, handle short writes and preserve stream exceptions.
  Route asset access through the binding's loading policy and PHP restrictions.
- EditorConfig specifies LF, UTF-8, final newlines, tabs for C and Makefiles,
  and four spaces for PHP. PHPT expectations preserve trailing whitespace.
  Use clang-format and clang-tidy on maintained C sources, and PHPCS (PSR-12)
  and PHPStan at maximum level on maintained PHP helpers. Stubs receive PHP
  syntax checks. Generated arginfo and upstream source have their own conventions.

## Local builds

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
The fixture scripts select a private Fontconfig file for repeatable output.

Build prerequisites include Bash, GCC/G++ with C++20, Make, Autoconf, pkg-config,
CMake, Ninja, Python 3, curl, tar, flock, jq, and development headers for
OpenSSL, zlib, libxml2, iconv, Cairo (PDF/PNG/FreeType/Fontconfig), FreeType,
Fontconfig, HarfBuzz and ICU. The renderer script supplies pinned private builds
of missing Expat, TurboJPEG and WebP dependencies. It also builds libcurl, HTTP
and HTTPS only, when the host has no libcurl development files with HTTPS.
`scripts/build-plutobook` passes `LDFLAGS` on to the renderer's link step.
PDF inspection uses Poppler.
QA uses Python venv/pip and unzip or 7z for Composer.

All PHP builds, binaries, headers and ini files live under this checkout.
Builds use existing non-PHP host tools and libraries read-only. Each extension
build uses its own absolute `phpize`, `php-config` and CLI. Build configuration,
compiler details and private FPM request evidence are under `artifacts/`.
[Source and tool pins](tools/sources.sha256)

`scripts/build-php` and `scripts/verify-php` with no arguments build and check
every pinned NTS runtime. Pass its exact PHP version to the extension build
and test commands. `debug`, `ubsan` and `asan` are separate build flavors.
`debug` also fails the extension build on compiler warnings. `asan` builds PHP
and the extension with AddressSanitizer, using GCC when its runtime is installed
and Clang otherwise. Its unit run fails on any LeakSanitizer report outside
`tools/asan/lsan-suppressions.txt`.
`scripts/bootstrap-qa`, `scripts/check-php` and `scripts/unit-coverage` run the
PHP and native quality checks, including the 85% unit line-coverage gate.

Select `bin/phpize` and `bin/php-config` from the same exact prefix as the runtime.
Keep a staged extension build per ABI and flavor in `build/extension/`.
All PHP invocations, including PHARs, PHPT runners and fixture generators,
use that private CLI. Keep system binaries, libraries, services, symlinks
and configuration read-only. Source archives are checksum-pinned. Update the
manifest explicitly against the [supported branches](https://www.php.net/supported-versions.php)
and [official PHP downloads](https://www.php.net/downloads.php?source=Y).

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

To build a release ZIP from the current checkout and test its installed layout on
`winbuild`, pass the extension version and thread mode explicitly.

```sh
scripts/test-winbuild 8.4.28 release 1.0.0 nts
scripts/test-winbuild 8.4.28 release 1.0.0 ts
```

PHP-FPM and the native line-coverage gate run on Linux.

## Testing

Place each assertion at the lowest level that can detect the failure reliably.

| Suite | Boundary and representative cases |
| --- | --- |
| Unit (`tests/unit/*.phpt`, including subdirectories) | Check public argument validation, empty and binary buffers, independent object state, repeat destruction paths, and short or failed writes. Exercise native helpers directly only where a PHP-level assertion cannot isolate the behavior. |
| Integration | Render synthetic invoices with pinned fonts and assets. Extract text and page properties to check Serbian Latin and Cyrillic, currencies, line items, totals, long descriptions, pagination, missing resources and explicit base locations. Check lifecycle behavior with process memory and native diagnostics. |
| End to end | Send an HTML invoice through a real private FPM worker, load the ABI-matched extension and inspect the returned PDF. Run CLI and FPM on every supported stable NTS branch. The existing toolchain smoke test checks the runtimes before the extension exists. |

The Linux NTS and Windows NTS and TS matrices cover synthetic invoices.

Network PHPTs never leave the machine. `tests/unit/network-server.php` serves
fixtures over HTTP or TLS on 127.0.0.1, with a certificate generated at test time.
The internal-address tests stop before connecting. Keep new remote-asset cases on
that fixture rather than public hosts.

Native leaks are gated by `scripts/build-php VERSION asan`, then
`scripts/build-extension VERSION asan` and `scripts/test-extension VERSION asan`.
The run fails on any LeakSanitizer report. Add a suppression to
`tools/asan/lsan-suppressions.txt` only for a reproduced upstream leak, scoped
to its library, with a comment naming the reproducer. Tests that cannot run under
AddressSanitizer skip themselves when `SKIP_ASAN` is set.

Cairo 1.18 leaks PDF font options, and PlutoBook 0.20.0 passes a variation
string even for fonts with no variation axes. With the upstream Windows
renderer, the process working set grew by about 19 KiB per invoice over 800
renders. `tools/asan/render-loop.c` reproduces the native leak independently of
PHP. The Linux portable modules and the Windows renderer carry both fixes.
[Cairo #1000](https://gitlab.freedesktop.org/cairo/cairo/-/issues/1000)
· [PlutoBook #84](https://github.com/plutoprint/plutobook/issues/84)

Require **at least 85% executable line coverage** for handwritten native binding
code from the unit suite alone. Integration and end-to-end runs have separate
processes and reports. Focus on distinct contracts and failures. Add a regression
case for a confirmed bug when existing coverage would miss it.

Build the pinned PlutoBook under
`.local/plutobook/0.20.0/` and run `scripts/unit-coverage 8.2.36`. It creates
a fresh instrumented extension build, applies the C format and analysis checks,
runs only the unit PHPTs and writes HTML, JSON and Cobertura XML reports to
`artifacts/coverage/`. The gate rejects missing execution data, an empty
denominator or less than 85%. CI uploads the Cobertura report to Codecov with the
organization's `CODECOV_TOKEN`. `codecov.yml` maps the build copy's `src/` paths
back to `ext/src/` and keeps Codecov's own checks informational.

## Binaries and packages

### Portable Linux modules

Binary releases and packages use a self-contained `plutobook.so` per PHP minor
version for Linux x86_64. [INSTALL.md](INSTALL.md#whats-in-the-linux-build)
describes the `linux` and `el8` variants for users. Build them with Podman or Docker:

```sh
scripts/build-portable            # every pinned PHP release
scripts/build-portable 8.3.37     # one release; --skip-tests skips the PHPT and invoice suites
```

The build runs offline in an AlmaLinux 8 container with GCC 14 and tests each
module there with the PHPT and invoice suites. It writes
`build/portable/dist/VARIANT/phpX.Y/plutobook.so` (about 12 MB), a separate
`plutobook.so.debug`, and the license notices of the bundled
libraries in `build/portable/dist/licenses/`. Logs are in
`build/portable/root/artifacts/portable/`. The `linux` variant compiles against
the libcurl 7.68.0 headers and its tests run against that release. The `el8`
variant contains libcurl 8.23.0 with the system GnuTLS, since libcurl 8.18 and
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

### deb and rpm packages

`scripts/build-packages` wraps the portable modules with nFPM, one package per PHP
minor version, in `build/portable/packages/`. It takes the same
`PLUTOBOOK_RELEASE_VERSION` as the modules and refuses modules built without it.
The deb packages and the EL9 and EL10 rpm packages carry the `linux` module, and
the EL8 rpm packages carry the `el8` module. `PLUTOBOOK_PACKAGE_MAINTAINER` sets
the Maintainer field. `scripts/test-packages` installs each package in a clean
container with that distribution's PHP, or Surý's or Remi's where the
distribution lacks the version, renders a page, removes the package and checks
that nothing is left.

## Release

Use Conventional Commits with the release rules shared with our WooCommerce
plugins. `feat:` produces a minor release. `fix:`, `perf:`, `compat:`,
`refactor:` and `style:` produce a patch. `BREAKING CHANGE:`,
`BREAKING CHANGES:` or `!:` produce a major release. `chore:` and routine
documentation or test-only commits do not request a release. Release notes
group commits under the plugins' sections. The release configuration follows
the GitHub default branch, `master`.

Release development uses the Node version in `.node-version` and `npm ci`.
The GitHub Actions workflow builds the private toolchains, runs the PHPT and
CLI/FPM invoice suites for each configured ABI and flavor, and checks coverage.
A successful push to master then calls the reusable release workflow
(`.github/workflows/release.yml`):

1. **version** runs semantic-release as a dry run. `scripts/release-version.mjs`
   writes the next version to the job outputs. No version, no release.
2. **linux** (`.github/workflows/portable.yml`) runs `scripts/build-portable`,
   `scripts/build-packages` and `scripts/test-packages` with
   `PLUTOBOOK_RELEASE_VERSION` set to that version, natively on x86_64
   (`ubuntu-24.04`) and aarch64 (`ubuntu-24.04-arm`), and uploads the PIE archives,
   the MPL sources and the packages. Pull requests that touch the build run the same
   workflow with a placeholder version.
3. **renderer** runs `scripts/build-windows-renderer` in the Linux job's private
   MinGW-w64 build environment. It builds the Windows x64 renderer once from the
   pinned graphics sources and patches and supplies the same DLL, header, and
   notices to all Windows jobs.
4. **windows** builds a source-matched PHP CLI and extension for each pinned
   PHP minor, in both NTS and TS for x64. PHP 8.2 and 8.3 use the VS2019 v142
   compiler on `windows-2022`, PHP 8.4 and 8.5 use VS2022 on the same image,
   and PHP 8.6 uses `windows-2025-vs2026`. Each job runs the PHPT suite, renders
   the invoice, verifies the PIE package against its PHP ABI and uploads its ZIP
   and invoice for the publish job. GitHub lists the installed toolsets in its
   [Windows Server 2022 image inventory](https://github.com/actions/runner-images/blob/main/images/windows/Windows2022-Readme.md)
   and [Windows Server 2025 with VS2026 image inventory](https://github.com/actions/runner-images/blob/main/images/windows/Windows2025-VS2026-Readme.md).
5. **publish** inspects each Windows invoice with Poppler and checks the exact
   set of Linux and Windows asset names against the version and PHP pins. The
   Windows ZIPs follow `php_plutobook-VERSION-MAJOR.MINOR-THREAD-COMPILER-x86_64.zip`,
   for example `php_plutobook-1.2.0-8.3-nts-vs16-x86_64.zip`. It imports the
   organization's `GPG_PRIVATE_KEY`, and runs semantic-release. The version
   plugin stops the release if the version changed since the build. Prepare steps
   then add the PIE source archive (`scripts/release-source.mjs`) and sign
   (`scripts/sign-release`): every rpm with rpmsign, and `SHA256SUMS` over all
   assets as `SHA256SUMS.asc`. Only then does semantic-release tag, and its GitHub
   plugin uploads to a draft release before publishing it.

The source archive embeds the version in `ext/release-version.h`. The binding
reports this version in release builds and a development version in a checkout.
Every archive and package includes this project's license and the third-party
notices. Releases are signed with `tools/keys/release-signing-key.asc`
(fingerprint `D80D 92A4 A247 D7F6 BF32 4E1A BFE0 F7EC 429C 222F`); signing fails
if the imported key differs.

PIE finds releases through the
[`oblakhost/ext-plutobook`](https://packagist.org/packages/oblakhost/ext-plutobook)
package on Packagist. PECL directs new extensions to PIE, so the extension is not
published there.
[PECL publishing policy](https://pecl.php.net/account-request.php)
