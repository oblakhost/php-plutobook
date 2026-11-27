# Installing PlutoBook for PHP

The commands below always download the
[latest release](https://github.com/oblakhost/php-plutobook/releases/latest). In
file names, `VERSION` stands for its version number.

| Platform | Versions | PHP from | Use |
| --- | --- | --- | --- |
| Debian | 12, 13 | Debian or Surý's repository | [deb package](#debian-and-ubuntu) or [PIE](#any-platform-pie) |
| Ubuntu | 22.04, 24.04 | Ubuntu or Surý's PPA | [deb package](#debian-and-ubuntu) or [PIE](#any-platform-pie) |
| RHEL, AlmaLinux, Rocky Linux, CloudLinux | 8, 9, 10 | AppStream or Remi's repository | [rpm package](#rhel-almalinux-rocky-linux-and-cloudlinux), or [PIE](#any-platform-pie) on 9 and 10 |
| Other Linux on x86_64 | glibc 2.28 or newer | any non-thread-safe build | [PIE](#other-linux-distributions) |
| Linux with thread-safe PHP or musl | | | [PIE, compiling from source](#building-from-source) |
| Windows | x64 | thread-safe or not | [PIE](#windows) |

Every platform supports PHP 8.2 to 8.6. macOS isn't supported.

- [Any platform: PIE](#any-platform-pie)
- [Debian and Ubuntu](#debian-and-ubuntu)
- [RHEL, AlmaLinux, Rocky Linux and CloudLinux](#rhel-almalinux-rocky-linux-and-cloudlinux)
- [Other Linux distributions](#other-linux-distributions)
- [Windows](#windows)
- [Building from source](#building-from-source)
- [Verifying downloads](#verifying-downloads)
- [Checking that it works](#checking-that-it-works)
- [Troubleshooting](#troubleshooting)

## Any platform: PIE

[PIE](https://github.com/php/pie) installs extensions for the PHP it runs with.
Install PIE as its README describes, then run:

```sh
pie install oblakhost/ext-plutobook
php --ri plutobook
```

PIE downloads a ready-made build when one matches your PHP:

- **Linux:** x86_64 with glibc 2.28 or newer, non-thread-safe PHP 8.2 to 8.6, and
  libcurl 7.68.0 or newer. For PHP 8.4 that is
  `php_plutobook-VERSION_php8.4-x86_64-linux-glibc-nts.zip`.
- **Windows:** x64 with PHP 8.2 to 8.6, thread-safe or not. For non-thread-safe
  PHP 8.4 that is `php_plutobook-VERSION-8.4-nts-vs17-x86_64.zip`.

Anywhere else, for example with thread-safe PHP or musl on Linux, or when you pass
configure options, PIE compiles the extension from the release's source archive.
[Building from source](#building-from-source) lists what that needs.

If `php --ri plutobook` says the extension isn't loaded, add `extension=plutobook`
to your `php.ini`. Restart PHP-FPM or your web server afterwards.

## Debian and Ubuntu

Packages are available for Debian 12 and 13 and Ubuntu 22.04 and 24.04, with PHP
from the distribution or from Ondřej Surý's repository. Each PHP version has its
own package, named `phpX.Y-plutobook`. It installs only next to the PHP it was
built for, because it depends on that PHP's API version (`phpapi-API`, such as
`phpapi-20240924` for PHP 8.4).

1. Download and install the package for your PHP version:

   ```sh
   php=$(php -r 'echo PHP_MAJOR_VERSION, ".", PHP_MINOR_VERSION;')
   version=$(basename "$(curl -fsSLo /dev/null -w '%{url_effective}' https://github.com/oblakhost/php-plutobook/releases/latest)")
   curl -fLO "https://github.com/oblakhost/php-plutobook/releases/download/$version/php$php-plutobook_$version-1_amd64.deb"
   sudo apt install "./php$php-plutobook_$version-1_amd64.deb"
   ```

   The first line reads your PHP version, such as `8.4`, and the second finds the
   latest release. If PHP's command line isn't installed, set `php=8.4` yourself.

   The package turns the extension on for that PHP's CLI, FPM and Apache module
   with `phpenmod`, and installs the system font configuration and the DejaVu
   fonts if they are missing.

2. Restart PHP-FPM if you use it, and check:

   ```sh
   sudo systemctl restart "php$php-fpm"
   php --ri plutobook
   ```

If your distribution doesn't have the PHP version you want (Ubuntu 22.04 ships PHP
8.1, for example), install PHP from [Surý's repository](https://deb.sury.org/) on
Debian or from his [PPA](https://launchpad.net/~ondrej/+archive/ubuntu/php) on
Ubuntu first:

```sh
sudo add-apt-repository ppa:ondrej/php   # Ubuntu
```

| File | Purpose |
| --- | --- |
| `/usr/lib/php/API/plutobook.so` | The extension, where `API` is PHP's API version, such as `20240924` |
| `/etc/php/8.4/mods-available/plutobook.ini` | Loads the extension |

To remove it, run `sudo apt remove php8.4-plutobook`.

## RHEL, AlmaLinux, Rocky Linux and CloudLinux

Packages are available for versions 8, 9 and 10, with PHP installed in `/usr` from
AppStream or from Remi's repository. Each PHP version and system version has its
own package, named `php-plutobook-VERSION-1.phpXY.elN.x86_64.rpm`. It installs only
next to the PHP it was built for, because it depends on that PHP's API version
(`php(api) = API-64`, such as `php(api) = 20240924-64` for PHP 8.4).

1. Download the package for your system and PHP version, check its signature and
   install it:

   ```sh
   el=$(rpm -E %rhel)
   php=$(php -r 'echo PHP_MAJOR_VERSION, PHP_MINOR_VERSION;')
   version=$(basename "$(curl -fsSLo /dev/null -w '%{url_effective}' https://github.com/oblakhost/php-plutobook/releases/latest)")
   rpm=php-plutobook-$version-1.php$php.el$el.x86_64.rpm
   curl -fLO "https://github.com/oblakhost/php-plutobook/releases/download/$version/$rpm"
   sudo rpm --import "https://github.com/oblakhost/php-plutobook/releases/download/$version/php-plutobook-signing-key.asc"
   rpm -K "$rpm"   # reports "digests signatures OK"
   sudo dnf install "./$rpm"
   ```

   The first three lines read your system version (8, 9 or 10), your PHP version
   (such as `84` for PHP 8.4) and the latest release. If PHP's command line isn't
   installed, set `php=84` yourself.

2. Restart PHP-FPM and check:

   ```sh
   sudo systemctl restart php-fpm
   php --ri plutobook
   ```

**On version 8, use the rpm rather than PIE.** The system's libcurl 7.61.1 is
too old for the extension's downloads. The `el8` packages include libcurl 8.23.0
(HTTP and HTTPS only) and use the system's GnuTLS. The PIE build loads there too,
but skips every remote image and font as `network-error`.

For a newer PHP than AppStream offers, set up PHP from
[Remi's repository](https://rpms.remirepo.net/wizard/) first, using its modular
"single version" setup. The packages need PHP in `/usr`, so Remi's parallel
`php84-php` builds in `/opt/remi` don't work with them.

| File | Purpose |
| --- | --- |
| `/usr/lib64/php/modules/plutobook.so` | The extension |
| `/etc/php.d/40-plutobook.ini` | Loads the extension |

To remove it, run `sudo dnf remove php-plutobook`.

### Tested combinations

Every release's packages are installed, used to render a page and removed again in
clean containers for these combinations. Packages exist for every PHP version from
8.2 to 8.6 on each system.

| System | PHP | PHP from |
| --- | --- | --- |
| Debian 12 | 8.2 | Debian |
| Debian 13 | 8.4 | Debian |
| Debian 13 | 8.5, 8.6 | Surý's repository |
| Ubuntu 22.04 | 8.3 | Surý's PPA |
| Ubuntu 24.04 | 8.3 | Ubuntu |
| AlmaLinux 8 | 8.2 | AppStream |
| AlmaLinux 8 | 8.4 | Remi's repository |
| AlmaLinux 9 | 8.3 | AppStream |
| AlmaLinux 9 | 8.5, 8.6 | Remi's repository |
| AlmaLinux 10 | 8.3 | AppStream |
| AlmaLinux 10 | 8.4 | Remi's repository |

## Other Linux distributions

On other x86_64 distributions with glibc 2.28 or newer, such as Fedora, Arch Linux
or openSUSE, install with [PIE](#any-platform-pie). You need:

- non-thread-safe PHP 8.2 to 8.6: `php -i | grep 'Thread Safety'` shows `disabled`
- libcurl 7.68.0 or newer
- Fontconfig and a font package such as DejaVu

| Distribution | Fonts |
| --- | --- |
| Fedora | `sudo dnf install fontconfig dejavu-sans-fonts` |
| Arch Linux | `sudo pacman -S fontconfig ttf-dejavu` |
| openSUSE | `sudo zypper install fontconfig dejavu-fonts` |

### What's in the Linux build

PIE and the packages install a self-contained `plutobook.so`, one per PHP version.
It comes in two variants, which differ in where libcurl comes from:

- **`linux`** runs on glibc 2.28 or newer (version 9 and later of the RHEL family,
  Debian 10 and later, Ubuntu 20.04 and later) and uses the system's libcurl
  7.68.0 or newer. PIE installs it, and the deb packages and the version 9 and 10
  rpm packages contain it.
- **`el8`** is for AlmaLinux, Rocky Linux and CloudLinux 8. It includes libcurl
  8.23.0 and uses the system's GnuTLS. Only the version 8 rpm packages contain it,
  because PIE can't tell these systems apart from other glibc systems.

The module includes PlutoBook, Cairo, pixman, HarfBuzz, FreeType, Fontconfig,
libpng, zlib, Expat, ICU, libjpeg-turbo and libwebp, with fixes for two upstream
memory leaks. [THIRD_PARTY.md](THIRD_PARTY.md) lists their licenses. ICU's data is
trimmed to what PlutoBook uses, and text in 25 languages and ten scripts renders
the same as with the full data. These copies stay private to the extension, so
they don't clash with the ones gd, imagick or intl load. From the system, the
module needs only glibc, the GCC 8 `libstdc++` and `libgcc_s`, and libcurl or
GnuTLS.

The bundled Fontconfig reads the system configuration in `/etc/fonts`. Its cache
format (version 12) differs from the system Fontconfig's, so each PHP process
scans the font folders once unless it can write its own cache, for example in
`/var/cache/fontconfig` when running as root or in the PHP user's home folder. The
scan took 15 ms with 38 fonts and 120 ms with 263 fonts on AlmaLinux 10. Its cache
files sit next to the system's and leave them alone, so `fc-list`, Imagick and other
programs keep using their own cache. A patch stops it from printing "No writable
cache directories" for every font folder; set `FC_DEBUG=16` to see that message again.

## Windows

Install with [PIE](#any-platform-pie). It picks the ZIP for your PHP version,
thread safety, compiler and architecture. For example,
`php_plutobook-VERSION-8.4-nts-vs17-x86_64.zip` is for 64-bit non-thread-safe PHP 8.4
built with Visual Studio 2022.

| File | Location |
| --- | --- |
| `php_plutobook.dll` | PHP's extension folder |
| `libplutobook-0.dll` | Next to `php.exe` |
| Fonts, font configuration and licenses | `extras\plutobook` |

The extension includes libcurl and zlib, and uses Windows' own TLS (Schannel) for
HTTPS. Restart your web server after installing.

The extension uses its packaged fonts unless `FONTCONFIG_FILE` or `FONTCONFIG_PATH`
is set. If the PHP folder's path contains characters outside the Windows ANSI code
page and its drive has 8.3 short names turned off, copy `fonts.conf` and the
`fonts` folder next to it to a plain path and point `FONTCONFIG_PATH` there.

## Building from source

PIE compiles the extension when no ready-made build fits, and distribution
packagers can build it the same way, with `phpize`, `./configure --with-plutobook`
and `make`. `configure` finds the libraries through `pkg-config`:

- **PlutoBook 0.20.0** (`plutobook.pc`). It builds with Meson. This project uses
  `-Ddefault_library=shared -Dturbojpeg=enabled -Dwebp=enabled`. Add
  `-Dcurl=disabled`: the extension downloads remote files itself and never uses
  PlutoBook's own HTTP client.
- **libcurl 7.68.0 or newer with HTTP and HTTPS** (`libcurl.pc`, from a package such
  as `libcurl4-openssl-dev` or `libcurl-devel`). `configure` stops with an error if
  libcurl is missing or lacks either protocol. If the libcurl loaded at run time is
  older than 7.68.0, remote files are skipped as `network-error`, and
  `php --ri plutobook` shows the version it found.

The usual `CPPFLAGS`, `CFLAGS`, `LDFLAGS` and `PKG_CONFIG_PATH` variables apply to
`configure`. Meson and CMake take `CFLAGS`, `CXXFLAGS` and `LDFLAGS` the same way.

Unpatched Cairo 1.18 and PlutoBook 0.20.0 leak memory on every render. See
[Limitations](README.md#limitations) in the README.

## Verifying downloads

Every release includes `SHA256SUMS`, signed as `SHA256SUMS.asc`, and the rpm files
are signed too. The signing key is `php-plutobook-signing-key.asc` in each release
and [`tools/keys/release-signing-key.asc`](tools/keys/release-signing-key.asc) in
this repository. Its fingerprint is
`D80D 92A4 A247 D7F6 BF32 4E1A BFE0 F7EC 429C 222F`.

```sh
gpg --import php-plutobook-signing-key.asc
gpg --verify SHA256SUMS.asc SHA256SUMS
sha256sum --ignore-missing -c SHA256SUMS

sudo rpm --import php-plutobook-signing-key.asc
rpm -K php-plutobook-*.rpm   # reports "digests signatures OK"
```

## Checking that it works

```sh
php --ri plutobook
php -r '$d = new PlutoBook\Document(); $d->loadHtml("<h1>It works</h1>"); $d->writeToFile("hello.pdf");'
```

The first command prints the extension's version and settings. The second writes
`hello.pdf` to the current folder.

## Troubleshooting

**PHP-FPM doesn't see the extension.** Restart PHP-FPM after installing. The CLI
and FPM can read different `php.ini` files, so check both with `php --ini` and
`php-fpm -i` (`php-fpm8.4 -i` on Debian and Ubuntu).

**Text is missing.** No fonts are installed. Install Fontconfig and a font package
such as DejaVu (see [Other Linux distributions](#other-linux-distributions)). On
Windows, check that `FONTCONFIG_FILE` and `FONTCONFIG_PATH` aren't pointing
somewhere unexpected.

**The first PDF in each PHP process is slow.** The bundled Fontconfig is scanning
the fonts because it can't write its cache. See
[What's in the Linux build](#whats-in-the-linux-build).

**Remote images and fonts are missing on version 8 of the RHEL family.** Install
the rpm package instead of using PIE. See
[RHEL, AlmaLinux, Rocky Linux and CloudLinux](#rhel-almalinux-rocky-linux-and-cloudlinux).

**The error log has `WARNING: Asset skipped` lines.** PlutoBook prints one for
each image, stylesheet or font it skips, and the extension can't turn them off.
Use `getResourceErrors()` to find out why each one was skipped.
