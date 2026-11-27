# Installation

```sh
pie install oblakhost/ext-plutobook
```

That is all most setups need. [PIE](https://github.com/php/pie) is PHP's extension
installer, the successor to PECL. Every
[release](https://github.com/oblakhost/php-plutobook/releases/latest) also has deb
and rpm packages if you'd rather use your system's package manager.

## Pick your platform

Every platform supports PHP 8.2 to 8.6. Linux builds cover x86_64 and arm64
(aarch64), and Windows builds cover x64.

| Platform | Versions | PHP from | Use |
| --- | --- | --- | --- |
| ![](../assets/icons/debian.svg) Debian | 12, 13 | Debian or Surý's repository | [deb package](debian-ubuntu.md) or [PIE](#any-platform-pie) |
| ![](../assets/icons/ubuntu.svg) Ubuntu | 22.04, 24.04 | Ubuntu or Surý's PPA | [deb package](debian-ubuntu.md) or [PIE](#any-platform-pie) |
| ![](../assets/icons/redhat.svg) RHEL, AlmaLinux, Rocky Linux, CloudLinux | 8, 9, 10 | AppStream or Remi's repository | [rpm package](rhel.md), or [PIE](#any-platform-pie) on 9 and 10 |
| ![](../assets/icons/cpanel.svg) cPanel (EasyApache 4) | EL 8, 9, 10; Ubuntu 22.04, 24.04 | `ea-php82` to `ea-php86` | [oblak-ea package](cpanel.md) |
| ![](../assets/icons/docker.svg) Official `php` Docker images | | built into the image | [PIE, plus fonts](debian-ubuntu.md#docker) |
| ![](../assets/icons/linux.svg) Other Linux | glibc 2.28 and libcurl 7.68.0 or newer | any, non-thread-safe | [PIE](other-linux.md) |
| ![](../assets/icons/alpinelinux.svg) Alpine, or thread-safe PHP such as FrankenPHP | | | [PIE, compiling from source](from-source.md) |
| ![](../assets/icons/windows.svg) Windows | x64 | thread-safe or not | [PIE](windows.md) |

macOS isn't supported; develop in a Linux container instead.

## Any platform: PIE

[PIE](https://github.com/php/pie) installs extensions for the PHP it runs with.
Install PIE as its README describes, then run:

```sh
pie install oblakhost/ext-plutobook
php --ri plutobook
```

PIE downloads a ready-made build for Linux (x86_64 or arm64, glibc 2.28 or newer,
non-thread-safe PHP) and for Windows x64. Anywhere else, for example with
thread-safe PHP or musl, or when you pass configure options, it compiles the
extension from the release's source archive.
[Building from source](from-source.md) lists what that needs.

The ready-made builds carry their own copies of PlutoBook and its libraries, so
they don't clash with the ones gd, imagick or intl load. On Linux they use the
system's libcurl, which must be 7.68.0 or newer for remote files to load.

If `php --ri plutobook` says the extension isn't loaded, add `extension=plutobook`
to your `php.ini`. Restart PHP-FPM or your web server afterwards.

PIE finds and downloads the release files through GitHub's API, about two requests
per install. GitHub allows 60 anonymous requests an hour per IP address, so on a
shared address or in CI, give PIE a GitHub token through Composer's
authentication, for example
`COMPOSER_AUTH='{"github-oauth":{"github.com":"TOKEN"}}'`. PIE doesn't read
`GITHUB_TOKEN`.

## Check that it works

```sh
php --ri plutobook
php -r '$d = new PlutoBook\Document(); $d->loadHtml("<h1>It works</h1>"); $d->writeToFile("hello.pdf");'
```

The first command prints the extension's version and settings. The second writes
`hello.pdf` to the current folder. If either fails, see
[Troubleshooting](troubleshooting.md).

## Running under PHP-FPM

- Restart PHP-FPM after installing. The CLI and FPM can read different `php.ini`
  files; see [Troubleshooting](troubleshooting.md#php-fpm-doesnt-see-the-extension).
- Each worker scans the installed fonts on its first PDF unless the PHP user can
  write a font cache; see
  [Troubleshooting](troubleshooting.md#the-first-pdf-in-each-php-process-is-slow).
- PlutoBook writes a `WARNING:` line to stderr for each file it skips. FPM only
  keeps those lines when the pool has `catch_workers_output = yes`.
- Rendering counts toward `max_execution_time`, and FPM's
  `request_terminate_timeout` still applies. See
  [Errors and time limits](../guides/errors-and-time-limits.md).
- If you compiled against unpatched system libraries, set `pm.max_requests` so
  workers restart regularly; see [Limitations](../limitations.md). The same goes
  for long-running queue workers.

## Tested combinations

Every release's packages are installed, used to render a page and removed again in
clean containers for these combinations, on both x86_64 and arm64. Packages exist
for every PHP version from 8.2 to 8.6 on each system and architecture.

| System | PHP | PHP from |
| --- | --- | --- |
| ![](../assets/icons/debian.svg) Debian 12 | 8.2 | Debian |
| ![](../assets/icons/debian.svg) Debian 13 | 8.4 | Debian |
| ![](../assets/icons/debian.svg) Debian 13 | 8.5, 8.6 | Surý's repository |
| ![](../assets/icons/ubuntu.svg) Ubuntu 22.04 | 8.3 | Surý's PPA |
| ![](../assets/icons/ubuntu.svg) Ubuntu 24.04 | 8.3 | Ubuntu |
| ![](../assets/icons/almalinux.svg) AlmaLinux 8 | 8.2 | AppStream |
| ![](../assets/icons/almalinux.svg) AlmaLinux 8 | 8.4 | Remi's repository |
| ![](../assets/icons/almalinux.svg) AlmaLinux 9 | 8.3 | AppStream |
| ![](../assets/icons/almalinux.svg) AlmaLinux 9 | 8.5, 8.6 | Remi's repository |
| ![](../assets/icons/almalinux.svg) AlmaLinux 10 | 8.3 | AppStream |
| ![](../assets/icons/almalinux.svg) AlmaLinux 10 | 8.4 | Remi's repository |

## What the ready-made builds contain

In file names, `VERSION` stands for the release's version number.

**Linux.** One self-contained `plutobook.so` per PHP version and architecture.
PIE's ZIP for PHP 8.4 is
`php_plutobook-VERSION_php8.4-x86_64-linux-glibc-nts.zip`, or
`php_plutobook-VERSION_php8.4-arm64-linux-glibc-nts.zip` on arm64. The module
comes in two variants, which differ in where libcurl comes from:

- **`linux`** runs on glibc 2.28 or newer with the system's libcurl 7.68.0 or
  newer: version 9 and later of the RHEL family, Debian 11 and later, Ubuntu 20.04
  and later. PIE installs it, and the deb packages and the version 9 and 10 rpm
  packages contain it.
- **`el8`** is for AlmaLinux, Rocky Linux and CloudLinux 8, whose libcurl is too
  old; see [Version 8: use the rpm](rhel.md#version-8-use-the-rpm). Only the
  version 8 rpm packages contain it, because PIE can't tell these systems apart
  from other glibc systems.

The module includes PlutoBook, Cairo, pixman, HarfBuzz, FreeType, Fontconfig,
libpng, zlib, Expat, ICU, libjpeg-turbo and libwebp, with fixes for two upstream
memory leaks.
[THIRD_PARTY.md](https://github.com/oblakhost/php-plutobook/blob/master/THIRD_PARTY.md)
lists their licenses. To keep the file small, only the Unicode data PlutoBook needs
is included; text in 25 languages and ten scripts renders the same as with the full
data. From the system, the module needs only glibc, the GCC 8 `libstdc++` and
`libgcc_s`, and libcurl or GnuTLS.

**Windows.** PIE picks the ZIP for your PHP version, thread safety, compiler and
architecture, for example `php_plutobook-VERSION-8.4-nts-vs17-x86_64.zip` for
non-thread-safe PHP 8.4. The extension includes libcurl and zlib, and uses
Windows' own TLS (Schannel) for HTTPS. See [Windows](windows.md).

## Verifying downloads

Every release includes `SHA256SUMS`, signed as `SHA256SUMS.asc`, and the rpm files
are signed too. The signing key is `php-plutobook-signing-key.asc` in each release
and
[`tools/keys/release-signing-key.asc`](https://github.com/oblakhost/php-plutobook/blob/master/tools/keys/release-signing-key.asc)
in the repository. Its fingerprint is
`D80D 92A4 A247 D7F6 BF32 4E1A BFE0 F7EC 429C 222F`.

```sh
gpg --import php-plutobook-signing-key.asc
gpg --verify SHA256SUMS.asc SHA256SUMS
sha256sum --ignore-missing -c SHA256SUMS

sudo rpm --import php-plutobook-signing-key.asc
rpm -K php-plutobook-*.rpm   # reports "digests signatures OK"
```
