# Troubleshooting

Start with [Check that it works](index.md#check-that-it-works): it shows whether
PHP loads the extension and can write a PDF.

## PIE hits GitHub's request limit

GitHub allows 60 anonymous API requests an hour per IP address, and each PIE
install uses about two. On a shared address or in CI, give PIE a token; see
[Any platform: PIE](index.md#any-platform-pie).

## `undefined symbol: smart_str_ferealloc`

PHP prints this warning at startup and runs without the extension. Releases up to
1.2.0 load only on PHP 8.2.35, 8.3.36, 8.4.27, 8.5.12 or newer, and on any 8.6.
Update the extension: later releases load on every patch release of PHP 8.2 to
8.6. Updating PHP to its newest patch release works too.

## PHP-FPM doesn't see the extension

Restart PHP-FPM after installing. The CLI and FPM can read different `php.ini`
files, so check both with `php --ini` and `php-fpm -i` (`php-fpm8.4 -i` on Debian
and Ubuntu).

## Text is missing

No fonts are installed. Install Fontconfig and a font package such as DejaVu; see
[Fonts](other-linux.md#fonts). On Windows, check that `FONTCONFIG_FILE` and
`FONTCONFIG_PATH` aren't pointing somewhere unexpected.

## The first PDF in each PHP process is slow

The Linux build brings its own Fontconfig, which can't use the system's font
cache: its cache format (version 12) is different. So each PHP process scans the
font folders on its first PDF, unless it can save its own cache, for example in
`/var/cache/fontconfig` when running as root or in the PHP user's home folder. The
scan took 15 ms with 38 fonts and 120 ms with 263 fonts on AlmaLinux 10.

Its cache files sit next to the system's and leave them alone, so `fc-list`,
Imagick and other programs keep using their own cache. A patch stops it from
printing "No writable cache directories" for every font folder; set
`FC_DEBUG=16` to see that message again.

## Remote images and fonts are missing on version 8 of the RHEL family

Install the rpm package instead of using PIE. See
[Version 8: use the rpm](rhel.md#version-8-use-the-rpm).

## The error log has `WARNING: Asset skipped` lines

PlutoBook prints one for each image, stylesheet or font it skips. See
[Missing images and fonts](../guides/missing-resources.md) to find out why each one
was skipped.
