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
A successful push to master can then call the reusable release workflow.

Semantic-release creates a numeric SemVer tag and GitHub Release. Its prepare
step packages the tested Git revision and embeds the same version in
`ext/release-version.h` in the PIE source archive. The binding reports
this version in release builds and a development version in a checkout.
The release archive includes this project's license and third-party notices.
For public distribution, register the accessible repository with Packagist and
enable tag updates. The renderer remains a separately supplied native dependency.
