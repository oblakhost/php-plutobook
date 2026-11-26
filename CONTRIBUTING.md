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

Use `scripts/build-php` and `scripts/verify-php` from the README. Select
`bin/phpize` and `bin/php-config` from the same exact prefix as the runtime.
Keep a staged extension build per ABI and flavor in `build/extension/`.
All PHP invocations, including PHARs, PHPT runners and fixture generators,
use that private CLI. Keep system binaries, libraries, services, symlinks
and configuration read-only. Source archives are checksum-pinned. Update the
manifest explicitly against the [supported branches](https://www.php.net/supported-versions.php)
and [official PHP downloads](https://www.php.net/downloads.php?source=Y).

## Testing

Place each assertion at the lowest level that can detect the failure reliably.

| Suite | Boundary and representative cases |
| --- | --- |
| Unit (`tests/unit/*.phpt`, including subdirectories) | Check public argument validation, empty and binary buffers, independent object state, repeat destruction paths, and short or failed writes. Exercise native helpers directly only where a PHP-level assertion cannot isolate the behavior. |
| Integration | Render synthetic invoices with pinned fonts and assets. Extract text and page properties to check Serbian Latin and Cyrillic, currencies, line items, totals, long descriptions, pagination, missing resources and explicit base locations. Check lifecycle behavior with process memory and native diagnostics. |
| End to end | Send an HTML invoice through a real private FPM worker, load the ABI-matched extension and inspect the returned PDF. Run CLI and FPM on every supported stable NTS branch. The existing toolchain smoke test checks the runtimes before the extension exists. |

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

Require **at least 85% executable line coverage** for handwritten native binding
code from the unit suite alone. Integration and end-to-end runs have separate
processes and reports. Focus on distinct contracts and failures. Add a regression
case for a confirmed bug when existing coverage would miss it.

Build the pinned PlutoBook under
`.local/plutobook/0.20.0/` and run `scripts/unit-coverage 8.2.36`. It creates
a fresh instrumented extension build, applies the C format and analysis checks,
runs only the unit PHPTs and writes HTML and JSON reports to `artifacts/coverage/`.
The gate rejects missing execution data, an empty denominator or less than 85%.
When a real WHMCS fixture becomes available, keep its customer data and assets
private and compare content, clipping and pagination against its Gotenberg PDF.

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
2. **linux** runs `scripts/build-portable`, `scripts/build-packages` and
   `scripts/test-packages` with `PLUTOBOOK_RELEASE_VERSION` set to that version,
   and uploads the PIE archives, the MPL sources and the packages.
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
if the imported key differs. For public distribution, register the repository
with Packagist and enable tag updates.
