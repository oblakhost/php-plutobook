# Debian and Ubuntu

Packages are available for Debian 12 and 13 and Ubuntu 22.04 and 24.04, with PHP
from the distribution or from Ondřej Surý's repository. The packages are named
`phpX.Y-plutobook`, and each works only with the PHP version in its name, such as
`php8.4-plutobook` for PHP 8.4. The commands below
always download the latest release.

1. Download and install the package for your PHP version:

   ```sh
   php=$(php -r 'echo PHP_MAJOR_VERSION, ".", PHP_MINOR_VERSION;')
   arch=$(dpkg --print-architecture)
   version=$(basename "$(curl -fsSLo /dev/null -w '%{url_effective}' https://github.com/oblakhost/php-plutobook/releases/latest)")
   curl -fLO "https://github.com/oblakhost/php-plutobook/releases/download/$version/php$php-plutobook_$version-1_$arch.deb"
   sudo apt install "./php$php-plutobook_$version-1_$arch.deb"
   ```

   The first lines read your PHP version, such as `8.4`, and your architecture,
   `amd64` or `arm64`. The third finds the latest release. If PHP's command line
   isn't installed, set `php=8.4` yourself.

   The package turns the extension on for that PHP's CLI, FPM and Apache module
   with `phpenmod`, and installs the system font configuration and the DejaVu
   fonts if they are missing.

2. Restart PHP-FPM if you use it, and check:

   ```sh
   sudo systemctl restart "php$php-fpm"
   php --ri plutobook
   ```

## A newer PHP than your distribution has

If your distribution doesn't have the PHP version you want (Ubuntu 22.04 ships PHP
8.1, for example), install PHP from [Surý's repository](https://deb.sury.org/) on
Debian or from his [PPA](https://launchpad.net/~ondrej/+archive/ubuntu/php) on
Ubuntu first:

```sh
sudo add-apt-repository ppa:ondrej/php   # Ubuntu
```

## Docker

The official `php` Docker images build their own PHP, so the deb packages don't
install there. Use PIE in your Dockerfile, as
[PIE's Docker instructions](https://github.com/php/pie/blob/HEAD/docs/usage.md#docker-installation)
show, and install the fonts:

```sh
apt-get update && apt-get install -y fontconfig-config fonts-dejavu-core
```

The Alpine variants of the images use musl, so PIE compiles the extension there;
see [Building from source](from-source.md).

## Installed files

| File | Purpose |
| --- | --- |
| `/usr/lib/php/API/plutobook.so` | The extension, where `API` is PHP's API version, such as `20240924` |
| `/etc/php/8.4/mods-available/plutobook.ini` | Loads the extension |

The package depends on that PHP API version (`phpapi-API`, such as
`phpapi-20240924` for PHP 8.4), which is why it installs only next to the PHP it
was built for. To remove it, run `sudo apt remove php8.4-plutobook`, with your PHP
version in place of `8.4`.
