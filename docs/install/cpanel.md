# cPanel (EasyApache 4)

On a cPanel server, PHP comes from EasyApache 4: `ea-php82` to `ea-php86`, each
installed under `/opt/cpanel/ea-phpNN/root`. The [rpm](rhel.md) and
[deb](debian-ubuntu.md) packages install into `/usr` and never reach those PHP
builds, so cPanel servers get their own package for each EasyApache PHP version:
`oblak-ea-phpNN-plutobook`, such as `oblak-ea-php84-plutobook` for `ea-php84`.

Packages are available for AlmaLinux, Rocky Linux and CloudLinux 8, 9 and 10
(rpm), and for Ubuntu 22.04 and 24.04 (one deb for both), on x86_64, the only
architecture cPanel runs on. Each package depends on its `ea-php` (the rpm on that
PHP's API version, the deb on `ea-phpNN-php-common`), so install it for the PHP
versions your server already runs.

## Why `oblak-ea` and not `ea-`

Package names that start with `ea-` belong to cPanel. When you provision an
EasyApache profile, cPanel removes `ea-` packages that aren't in it, and it could
ship a package with the same name one day. The `oblak-ea` prefix keeps the package
out of cPanel's way and shows who ships it. It stays installed until you remove it,
or until you remove the `ea-php` version it belongs to.

## Install on AlmaLinux, Rocky Linux and CloudLinux

Set `php` to the EasyApache PHP version, then download the package for your system
version, check its signature and install it:

```sh
php=84   # ea-php version: 82, 83, 84, 85 or 86
el=$(rpm -E %rhel)
version=$(basename "$(curl -fsSLo /dev/null -w '%{url_effective}' https://github.com/oblakhost/php-plutobook/releases/latest)")
rpm=oblak-ea-php$php-plutobook-$version-1.el$el.x86_64.rpm
curl -fLO "https://github.com/oblakhost/php-plutobook/releases/download/$version/$rpm"
sudo rpm --import "https://github.com/oblakhost/php-plutobook/releases/download/$version/php-plutobook-signing-key.asc"
rpm -K "$rpm"   # reports "digests signatures OK"
sudo dnf install "./$rpm"
```

## Install on Ubuntu

```sh
php=84   # ea-php version: 82, 83, 84, 85 or 86
version=$(basename "$(curl -fsSLo /dev/null -w '%{url_effective}' https://github.com/oblakhost/php-plutobook/releases/latest)")
deb=oblak-ea-php$php-plutobook_$version-1_amd64.deb
curl -fLO "https://github.com/oblakhost/php-plutobook/releases/download/$version/$deb"
sudo apt install "./$deb"
```

## Several PHP versions

Repeat the download for each `ea-php` version, then install all the files in one
`dnf install` or `apt install` command. The steps below then run once for the
whole command instead of once per package.

## What the package does for you

The extension goes into that `ea-php`'s extension folder, and `20-plutobook.ini`
into its `php.d` folder, so the CLI, PHP-FPM and LSAPI (mod_lsapi) all load it.
When the install command finishes, the package also does what cPanel does after
changing an `ea-php`, once:

1. **CageFS** (CloudLinux, when CageFS is in use): refreshes it with
   `cagefsctl --update --force-update`, so the new ini and the DejaVu fonts are
   visible inside your users' cages. Without the fonts there, PDFs rendered by
   websites come out without text.
2. **PHP-FPM**: restarts every `ea-php` PHP-FPM pool with cPanel's
   `restartsrv_apache_php_fpm`.
3. **Apache**: restarts Apache with cPanel's `restartsrv_httpd`, which also
   replaces running lsphp processes under mod_lsapi so they pick up the new ini.

Each step prints a line starting with `PlutoBook:` and is skipped when its tool
isn't there. If one fails, the package still installs and prints
`PlutoBook: warning: … failed; run it by hand` with the command. LiteSpeed Web
Server isn't restarted for you; restart it yourself.

## Check that it works

```sh
/opt/cpanel/ea-php84/root/usr/bin/php --ri plutobook
```

Use the `ea-php` version you installed for. To try a render, run the one-liner from
[Check that it works](index.md#check-that-it-works) with that PHP binary.

## Installed files

The paths are the same on every system, Ubuntu included.

| File | Purpose |
| --- | --- |
| `/opt/cpanel/ea-phpNN/root/usr/lib64/php/modules/plutobook.so` | The extension |
| `/opt/cpanel/ea-phpNN/root/etc/php.d/20-plutobook.ini` | Loads the extension |
| `/etc/cagefs/conf.d/oblak-ea-phpNN-plutobook.cfg` | Makes the DejaVu fonts and their font settings visible inside CageFS; does nothing without CageFS |

Put `plutobook.*` settings in your own ini file or in cPanel's MultiPHP INI
Editor, not in `20-plutobook.ini`, which belongs to the package: on Ubuntu,
`apt remove` deletes it, and on EL your edits end up in `.rpmnew` or `.rpmsave`
files. See [Configuration](../reference/configuration.md) for the settings.

## Removing it

```sh
sudo dnf remove oblak-ea-php84-plutobook   # or: sudo apt remove oblak-ea-php84-plutobook
```

On CloudLinux, CageFS is refreshed so cages stop loading the extension. Services
aren't restarted: running PHP processes keep the extension until they restart, and
new ones no longer load it.

Removing an `ea-php` version through EasyApache also removes its
`oblak-ea-phpNN-plutobook` package. Provisioning a profile leaves the package in
place as long as its `ea-php` stays.

## Coming from PIE

If you installed the extension with PIE before, the package warns that PIE's
`80-plutobook.ini` also loads it:
`PlutoBook: warning: … also loads plutobook. Remove it, then run: …`. Delete that
file from `/opt/cpanel/ea-phpNN/root/etc/php.d/` so PHP doesn't load the extension
twice; the package's `plutobook.so` has already replaced PIE's. The package's own
refresh ran before you removed the file, so run the refresh once more yourself:

```sh
/usr/sbin/cagefsctl --update --force-update   # CloudLinux only
/usr/local/cpanel/scripts/restartsrv_apache_php_fpm
/usr/local/cpanel/scripts/restartsrv_httpd
```

## Upgrading the system with ELevate

ELevate won't upgrade a server while packages installed from a file, rather than
from a repository, are present, and it removes all `ea-` packages during the
upgrade. Remove the `oblak-ea` packages before you start, and afterwards install
the ones for the new system version.

## Not covered

CloudLinux alt-php (`/opt/alt/phpNN`) and PHP Selector aren't covered by these
packages.

## Tested combinations

Every release's cPanel packages are installed, used to render a page and removed
again in clean containers with cPanel's public EasyApache 4 repository:

| System | EasyApache PHP |
| --- | --- |
| ![](../assets/icons/almalinux.svg) AlmaLinux 8 | `ea-php82` |
| ![](../assets/icons/almalinux.svg) AlmaLinux 9 | `ea-php84` |
| ![](../assets/icons/almalinux.svg) AlmaLinux 10 | `ea-php86` |
| ![](../assets/icons/ubuntu.svg) Ubuntu 22.04 | `ea-php83` |

Ubuntu 24.04 uses the same deb as 22.04, but its test is paused for now: cPanel's
EasyApache 4 repository for Ubuntu 24.04 currently serves package lists that don't
match its signed index, so `apt` refuses to install `ea-php` from it. The test comes
back once cPanel fixes the repository.

The CageFS refresh, the restarts and rendering inside a website's cage were also
checked on a real CloudLinux 9 cPanel server running mod_lsapi.
