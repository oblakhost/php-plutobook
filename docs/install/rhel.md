# RHEL, AlmaLinux, Rocky Linux and CloudLinux

Packages are available for versions 8, 9 and 10, with PHP installed in `/usr` from
AppStream or from Remi's repository. Each package works only with the PHP version
and system version in its name, `php-plutobook-VERSION-1.phpXY.elN.ARCH.rpm`, where
`VERSION` is the release and `ARCH` is `x86_64` or `aarch64`. The commands below
always download the latest release.

On a cPanel server, PHP comes from EasyApache 4 instead; use the
[cPanel packages](cpanel.md).

1. Download the package for your system and PHP version, check its signature and
   install it:

   ```sh
   el=$(rpm -E %rhel)
   php=$(php -r 'echo PHP_MAJOR_VERSION, PHP_MINOR_VERSION;')
   arch=$(uname -m)
   version=$(basename "$(curl -fsSLo /dev/null -w '%{url_effective}' https://github.com/oblakhost/php-plutobook/releases/latest)")
   rpm=php-plutobook-$version-1.php$php.el$el.$arch.rpm
   curl -fLO "https://github.com/oblakhost/php-plutobook/releases/download/$version/$rpm"
   sudo rpm --import "https://github.com/oblakhost/php-plutobook/releases/download/$version/php-plutobook-signing-key.asc"
   rpm -K "$rpm"   # reports "digests signatures OK"
   sudo dnf install "./$rpm"
   ```

   The first lines read your system version (8, 9 or 10), your PHP version (such
   as `84` for PHP 8.4), your architecture (`x86_64` or `aarch64`) and the latest
   release. If PHP's command line isn't installed, set `php=84` yourself.

2. Restart PHP-FPM and check:

   ```sh
   sudo systemctl restart php-fpm
   php --ri plutobook
   ```

## Version 8: use the rpm

**On version 8, use the rpm rather than PIE.** The system's libcurl 7.61.1 is
too old for the extension's downloads. The `el8` packages include libcurl 8.23.0
(HTTP and HTTPS only) and use the system's GnuTLS. The PIE build loads there too,
but skips every remote image and font as `network-error`.

## A newer PHP than AppStream has

For a newer PHP than AppStream offers, set up PHP from
[Remi's repository](https://rpms.remirepo.net/wizard/) first, using its modular
"single version" setup. The packages need PHP in `/usr`, so Remi's parallel
`php84-php` builds in `/opt/remi` don't work with them.

## Installed files

| File | Purpose |
| --- | --- |
| `/usr/lib64/php/modules/plutobook.so` | The extension |
| `/etc/php.d/40-plutobook.ini` | Loads the extension |

The package depends on that PHP API version (`php(api) = API-64`, such as
`php(api) = 20240924-64` for PHP 8.4), which is why it installs only next to the
PHP it was built for. To remove it, run `sudo dnf remove php-plutobook`.
