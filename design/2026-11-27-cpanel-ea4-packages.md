# cPanel EasyApache 4 packages

Beads: `pdf-ext-zcs.6` (epic `pdf-ext-zcs`). Status: design, 2026-11-27.

## Why

cPanel servers run PHP from EasyApache 4 (EA4): `ea-php82` … `ea-php86`, installed
under `/opt/cpanel/ea-phpNN/root`. The general packages (`php-plutobook`,
`phpX.Y-plutobook`) install into `/usr` and never reach those PHP builds. PIE works
there but needs flags (`--with-php-config`) and leaves CloudLinux CageFS, fonts and
running PHP processes for the admin to sort out (seen on oblak.test, see the
2026-11-26 note on the bead). cPanel admins should be able to download one file per
PHP version and install it with the system package manager, the same way they add
cPanel's own PHP modules, and have it work for websites straight away.

## Goals

- One package per EA4 PHP version (8.2 to 8.6) for:
  - AlmaLinux, Rocky Linux and CloudLinux 8, 9 and 10 (rpm, one per EL major);
  - Ubuntu 22.04 and 24.04 (deb, one file serving both).
- Installing makes the extension load in that ea-php's CLI, PHP-FPM and LSAPI
  (mod_lsapi) without further steps, including inside CloudLinux CageFS.
- Removing it leaves PHP as it was: no stray ini that loads a missing module.
- Built, tested in clean containers, signed and published with every release, like
  the general packages.
- Install docs for cPanel admins, in the plain style of the existing install pages.

## Non-goals

- A yum or apt repository (the epic leaves repo hosting for later).
- Registering an EA4 vendor prefix in `/etc/cpanel/ea4/additional-pkg-prefixes/`.
  It makes profile provisioning keep *or remove* our packages, and without a
  repository there is nothing to reinstall them from.
- aarch64: cPanel only runs on x86_64.
- CloudLinux alt-php (`/opt/alt/phpNN`) and PHP Selector.
- LiteSpeed Web Server restarts (docs tell the admin to restart it).
- Warming the bundled Fontconfig cache after install (optional per the bead notes;
  without it each PHP process scans the fonts once, 15 to 120 ms).
- Automatic testing of CageFS and LSAPI (they need a real CloudLinux/cPanel host).

## Anti-goals

- Installing the package must never fail, or leave dpkg/rpm half-configured, because
  a restart or CageFS refresh failed. Those steps warn and carry on.
- Nothing may touch `ea-*` packages, EA4 profiles or cPanel configuration.
- No restarts or CageFS work on machines that are not cPanel/CloudLinux (containers,
  plain AlmaLinux with EA4 repos).

## Decided

**Names (maintainer, 2026-11-27): prefix `oblak-ea`.**

| Kind | Package name | File |
| --- | --- | --- |
| rpm | `oblak-ea-php83-plutobook` | `oblak-ea-php83-plutobook-VERSION-1.el9.x86_64.rpm` (`el8`, `el9`, `el10`) |
| deb | `oblak-ea-php83-plutobook` | `oblak-ea-php83-plutobook_VERSION-1_amd64.deb` |

rpm `release` is `1.elN`; deb revision `1`. 15 rpms + 5 debs = 20 new release assets.

**Facts from research (2026-11-27, cPanel EA4 repos and CpanelInc sources):**

- Paths are the same on EL8/9/10 and Ubuntu (Ubuntu also uses `lib64`):
  extension dir `/opt/cpanel/ea-phpNN/root/usr/lib64/php/modules`, scan dir
  `/opt/cpanel/ea-phpNN/root/etc/php.d`, both owned by `ea-phpNN-php-common`.
- EL `ea-phpNN-php-common` (cPanel's and CloudLinux's rebuild, which has Epoch 1)
  provides `ea-phpNN-php(api) = API-64`: 82 `20220829`, 83 `20230831`,
  84 `20240924`, 85 `20250925`, 86 `20261112`. Ubuntu debs have no API provide.
- ea-php's curl extension links the system `libcurl.so.4` everywhere; no cPanel
  libcurl exists, so our module and ea-php share one libcurl.
- EA4's dnf/apt "universal hooks" restart services only for package names that
  start with `ea-`. For `ea-*` transactions cPanel runs, in this order:
  `/usr/sbin/cagefsctl --update --force-update` (only when CloudLinux is installed
  and `/etc/cagefs/cagefs.mp` has mount lines), `/usr/local/cpanel/scripts/restartsrv_apache_php_fpm`,
  `/usr/local/cpanel/scripts/restartsrv_httpd`.
- ELevate blocks an OS upgrade while any rpm that came from no repository is
  installed (`dnf list extras`), and removes all `ea-*` packages during the upgrade
  (dnf removes our dependent package with them; it is not reinstalled).
- Provisioning an EA4 profile removes `ea-*` packages not in the profile. Our
  packages are outside that namespace and are only removed if their `ea-phpNN` is.
- All five ea-php versions are published for all five systems today (8.6.0, 8.5.13,
  8.4.28, 8.3.37, 8.2.36).

## Package contents

| Path | rpm | deb |
| --- | --- | --- |
| `/opt/cpanel/ea-phpNN/root/usr/lib64/php/modules/plutobook.so` | 0755 | 0644 |
| `/opt/cpanel/ea-phpNN/root/etc/php.d/20-plutobook.ini` | `config\|noreplace` | plain file, **not** a conffile |
| `/etc/cagefs/conf.d/oblak-ea-phpNN-plutobook.cfg` | plain file | plain file |
| License files | `/usr/share/licenses/oblak-ea-phpNN-plutobook/…` and `/usr/share/doc/…` as the general rpm does | `/usr/share/doc/oblak-ea-phpNN-plutobook/` as the general deb does |

- The ini is the existing one-liner (`extension=plutobook.so`), named `20-` like
  cPanel's own bundled extensions.
- deb ini is not a conffile because `apt remove` keeps conffiles until purge, which
  would leave PHP warning about a missing `plutobook.so` on every start. Admins put
  settings in their own ini (or cPanel's MultiPHP INI Editor); docs say so.
- The CageFS file makes the DejaVu font directory visible inside cages (on
  oblak.test the cage had no DejaVu, and web PDFs came out without text). Its exact
  content follows CloudLinux's documented `conf.d` format (`[name]`, `comment=`,
  `paths=`), listing that system's DejaVu directory (EL8 `/usr/share/fonts/dejavu/`,
  EL9/10 `/usr/share/fonts/dejavu-sans-fonts/`, Ubuntu
  `/usr/share/fonts/truetype/dejavu/`; the builder confirms them from the font
  packages). Shipping it per package name keeps files unique, so installing several
  PHP versions does not conflict. It is inert where CageFS is absent.

Modules: EL8 rpms carry the `el8` module; EL9, EL10 and the deb carry the `linux`
module. Each must embed the release version (`module_for` check in
`scripts/build-packages`).

## Dependencies

rpm:

- `ea-phpNN-php(api) = API-64`, where `API` comes from the module's `phpapi` file
  (same as the general rpm's `php(api)`).
- `fontconfig`, `dejavu-sans-fonts`.
- EL9/10: `libcurl.so.4()(64bit)`. EL8: `libgnutls.so.30()(64bit)`,
  `libnettle.so.6()(64bit)` (the el8 module carries its own libcurl).

deb:

- `ea-phpNN-php-common` (the ea-php version fixes the API).
- `libc6 (>= 2.28)`, `libstdc++6 (>= 8)`, `libgcc-s1 | libgcc1`,
  `libcurl4 (>= 7.68.0) | libcurl4t64 (>= 7.68.0)`, `fontconfig-config`,
  `fonts-dejavu-core` (same as the general deb, minus `phpX.Y-common`/`phpapi-*`).

## Install and remove scripts

The steps mirror cPanel's own hook order. Each is skipped when its tool is
missing, and none can fail the package operation:

1. CageFS: if `/usr/sbin/cagefsctl` is executable and `/etc/cagefs/cagefs.mp` has a
   mount line (same test as cPanel's hook), run
   `/usr/sbin/cagefsctl --update --force-update`. This copies the new php.d ini and
   the font directory into cages, or drops them after removal.
2. PHP-FPM: if `/usr/local/cpanel/scripts/restartsrv_apache_php_fpm` is executable,
   run it (cPanel's own script; restarts every ea-php FPM pool, as cPanel does after
   any ea-php change).
3. Apache: if `/usr/local/cpanel/scripts/restartsrv_httpd` is executable, run it
   (no arguments, as cPanel's hook does). This replaces running lsphp backends
   under mod_lsapi, which otherwise keep the old configuration. Confirmed on
   oblak.test on 2026-11-27: the running lsphp group was terminated and the next
   request ran in a new one.

Each step prints one line saying what it did; on failure it prints a warning with
the command the admin can run by hand, and the script exits 0.

**When they run (designer's call, from the builder check):**

- **Install and upgrade: all three steps, once per transaction, after it
  completes.** Installing five ea-php versions in one `dnf`/`apt` command must not
  run `cagefsctl --update --force-update` (minutes on large servers) and the
  restarts five times, and must not restart services while other packages are
  half-installed. Each package's install script only records that a refresh is
  due (a flag file under `/run`); the end-of-transaction hook runs the steps if the
  flag is present and removes it, so the first package to get there does the work
  and the rest find nothing to do.
  - rpm: `%post` sets the flag; `%posttrans` (nFPM `rpm.scripts.posttrans`) acts.
  - deb: `postinst configure` sets the flag and runs
    `dpkg-trigger --no-await oblak-ea-plutobook-refresh`; every cPanel package
    declares `interest-noawait oblak-ea-plutobook-refresh` (nFPM `deb.triggers`),
    and `postinst triggered` acts.
- **Final removal: CageFS step only, from the removal script** (rpm `%postun` when
  `$1 = 0`; deb `postrm remove`). No restarts: running PHP processes keep the
  module they already loaded, and new ones no longer load it. The CageFS refresh is
  still needed, because the copy of `20-plutobook.ini` inside each cage would make
  new processes there warn about the missing module. This also keeps service
  restarts out of removals that EasyApache itself drives (removing an ea-php
  version or provisioning a profile without it), where cPanel's own end-of-
  transaction hooks restart services anyway. Removing several of our packages at
  once refreshes CageFS once per package; that is rare enough to accept.
- rpm erases do not run `%posttrans`, and an upgrade runs the new package's
  `%post`/`%posttrans`, so the old package's `%postun` (`$1 >= 1`) does nothing.

## Build

`scripts/build-packages` gains `cpanel_rpm EL MINOR` and `cpanel_deb MINOR`,
called from the existing per-minor loop only when the host is x86_64. They reuse
the nFPM binary, license staging (`$work/doc`) and `module_for`. Output goes to
`build/portable/packages/` with the general packages, so `portable.yml` uploads
them and the release signs every `.rpm` and lists every file in `SHA256SUMS`
without workflow changes. Static files (ini, CageFS cfg template, scriptlet
template) live under `tools/packages/cpanel/`.

`.github/scripts/release-assets.py` adds the 20 names to the expected x86_64
assets (EL 8, 9, 10 rpms and the deb per minor, x86_64/amd64 only).

## Tests

`scripts/test-packages` gets cPanel scenarios (x86_64 only; skipped on aarch64),
each in a clean container with cPanel's public EA4 repository, covering every PHP
minor and every system once:

| Scenario | Image | PHP |
| --- | --- | --- |
| `cpanel-el8-php8.2` | `almalinux:8` | `ea-php82` |
| `cpanel-el9-php8.4` | `almalinux:9` | `ea-php84` |
| `cpanel-el10-php8.6` | `almalinux:10` | `ea-php86` |
| `cpanel-ubuntu2204-php8.3` | `ubuntu:22.04` | `ea-php83` |
| `cpanel-ubuntu2404-php8.5` | `ubuntu:24.04` | `ea-php85` |

Repository setup:

- EL: `/etc/yum.repos.d/EA4.repo` with
  `baseurl=http://httpupdate.cpanel.net/cpanelsync/repos/<CentOS/8|CentOS/9|Almalinux/10>/EA4/`,
  `gpgkey=https://securedownloads.cpanel.net/cPanelPublicPkgKey.asc`.
- EL10: cPanel's key has SHA-1 self-signatures, which EL10's rpm-sequoia rejects;
  the EL10 scenario sets `gpgcheck=0` for the EA4 repo only (test-only workaround,
  documented in the script).
- Ubuntu: key `cPanelPublicPkgKey.asc` (fingerprint
  `7FB6 9A06 2F33 4F25 10CC DFFE 2378 B595 7E93 1C7C`) as a keyring, source
  `deb [signed-by=…] mirror://httpupdate.cpanel.net/ea4-u22-mirrorlist ./`
  (`u24` for 24.04).
- Ubuntu: cPanel's ea-php debs don't declare `libxml2`; install it with the PHP.
- Ubuntu 24.04: ea-php aborts with a "buffer overflow detected" under podman's
  default open-file limit; run that container with `--ulimit nofile=1024:1024`.

`tools/packages/check-install` learns the cPanel layout: PHP is
`/opt/cpanel/ea-phpNN/root/usr/bin/php`, the PHP package is `ea-phpNN-php-cli`, and
the leftover check covers the cPanel paths and the CageFS cfg. It keeps the same
flow: install, `php -m` shows plutobook, render the test page, `pdftotext` finds
the text, HTTP client line, remove, plutobook gone, no leftovers.

Script coverage: in one rpm and one deb scenario, check-install puts logging stubs
at `/usr/sbin/cagefsctl` (with a `/etc/cagefs/cagefs.mp` mount line),
`/usr/local/cpanel/scripts/restartsrv_apache_php_fpm` and
`/usr/local/cpanel/scripts/restartsrv_httpd`, installs **two** cPanel packages (two
ea-php versions) in one `dnf`/`apt` command, and asserts: each of the three steps
ran exactly once, in that order, after both packages were installed; removing one
package runs only `cagefsctl`, once; a failing stub does not fail the install.
Without stubs (the other scenarios) the scripts must be silent no-ops. The
scenario needs both packages' ea-php versions installed, so it installs two
`ea-phpNN-php-cli` packages.

## Verification on a real cPanel server (manual, before merging)

A merge to master with a `feat:`/`fix:` commit starts a release and publishes the
docs site, so this runs on the pull request's `linux-x86_64` CI artifact (version
`0.0.0-ci`) before merging.

The builder runs it over the existing SSH access to oblak.test (maintainer's
go-ahead, 2026-11-27). On oblak.test (CloudLinux 9.9, cPanel 138, ea-php82–85
under mod_lsapi with CageFS), remove PIE's earlier `80-plutobook.ini` and module,
install the el9 packages for two ea-php versions in one `dnf` command, then
confirm without manual steps: the extension loads for CLI and for a website user
inside CageFS; a web-rendered PDF has DejaVu text; running lsphp backends picked
up the new ini; removing one package reverses it for that version, with no
warning about a missing module inside the cage. Leave the server as found
afterwards (PIE's install restored or the packages left installed, whichever the
maintainer prefers), and remove any probe files.

## Docs

- New page `docs/install/cpanel.md` (sidebar after `install/rhel`), a row in the
  platform table in `docs/install/index.md`, and the matching section in
  `INSTALL.md`. Plain language, per the README style.
- Content: who it is for; download and install for one PHP version on EL
  (`el=$(rpm -E %rhel)`, signature check as on the RHEL page) and on Ubuntu,
  and how to repeat for each ea-php; what the package does automatically (FPM and
  Apache restart, CageFS refresh); checking with
  `/opt/cpanel/ea-phpNN/root/usr/bin/php --ri plutobook`; file table; removal.
- Notes: settings go in your own ini or the MultiPHP INI Editor, not
  `20-plutobook.ini`; removing an ea-php version through EasyApache also removes
  its package; before ELevate remove the packages and afterwards install the ones
  for the new system version; LiteSpeed Web Server users restart it themselves;
  if PIE installed the extension before, delete PIE's `80-plutobook.ini`.
- `docs/install/rhel.md` and the Debian/Ubuntu page point cPanel servers to the new
  page.

## Approaches considered

- **Extend `scripts/build-packages` (chosen).** Reuses nFPM setup, license staging
  and version checks; release, signing and checksums pick the files up unchanged.
- **Separate cPanel build and test scripts.** Easier to read alone, but duplicates
  staging and checks and adds CI steps for no gain.
- **Register files with EA4's universal hooks instead of package scripts.** Would
  run once per transaction like cPanel's, but the hook file goes away with the
  last package, so a final removal would never restart anything.

## Left to the builder

- How the scriptlet is templated (heredoc in the builder like the general deb's,
  or a file under `tools/packages/cpanel/` with the version substituted).
- Exact DejaVu directories per system, confirmed from the font packages.
- How check-install is split or parameterised for the cPanel layout.
- Wording of scriptlet messages.
