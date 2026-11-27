# cPanel EasyApache 4 Packages Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build, test, release and document `oblak-ea-phpNN-plutobook` rpm (EL8/9/10) and deb (Ubuntu 22.04/24.04) packages that install the portable module into cPanel's ea-php82–86.

**Architecture:** `scripts/build-packages` gains cPanel functions next to the general deb/rpm ones (x86_64 only), reusing nFPM, license staging and `module_for`. Install/remove scripts share one shell library (`tools/packages/cpanel/refresh.sh`) that mirrors cPanel's own EA4 hooks, once per transaction. `scripts/test-packages` runs the packages in clean containers against cPanel's public EA4 repositories; release, signing and checksums pick the files up unchanged.

**Tech Stack:** bash/POSIX sh, nFPM 2.49.0, rpm/dpkg (triggers, `%posttrans`), podman, Python 3 (`release-assets.py`), Docusaurus Markdown.

**Spec:** `design/2026-11-27-cpanel-ea4-packages.md` (read it first; this plan argues from it).

## Global Constraints

- Package name `oblak-ea-phpNN-plutobook`; files `oblak-ea-phpNN-plutobook-VERSION-1.elN.x86_64.rpm` (N = 8, 9, 10) and `oblak-ea-phpNN-plutobook_VERSION-1_amd64.deb`. rpm `release: 1.elN`, deb release `"1"`.
- Built only when the host is x86_64 (`arch=amd64`); aarch64 builds and tests skip everything cPanel.
- Module: EL8 rpm ← `dist/el8/phpX.Y`, everything else ← `dist/linux/phpX.Y`, via the existing `module_for`.
- Module path `/opt/cpanel/ea-phpNN/root/usr/lib64/php/modules/plutobook.so` (rpm 0755, deb 0644); ini `/opt/cpanel/ea-phpNN/root/etc/php.d/20-plutobook.ini` containing `extension=plutobook.so` (rpm `config|noreplace`; deb plain file, never a conffile); CageFS file `/etc/cagefs/conf.d/oblak-ea-phpNN-plutobook.cfg`.
- rpm depends: `ea-phpNN-php(api) = API-64` (API from the module's `phpapi`), `fontconfig`, `dejavu-sans-fonts`, plus EL9/10 `libcurl.so.4()(64bit)` or EL8 `libgnutls.so.30()(64bit)` and `libnettle.so.6()(64bit)`.
- deb depends: `ea-phpNN-php-common`, `libc6 (>= 2.28)`, `libstdc++6 (>= 8)`, `libgcc-s1 | libgcc1`, `libcurl4 (>= 7.68.0) | libcurl4t64 (>= 7.68.0)`, `fontconfig-config`, `fonts-dejavu-core`.
- Script steps, in order, each skipped when its tool is missing, none able to fail the package operation: (1) `/usr/sbin/cagefsctl --update --force-update` only if `/etc/cagefs/cagefs.mp` has a line matching `^[[:space:]]*[!@#]?/[[:alnum:]_]`; (2) `/usr/local/cpanel/scripts/restartsrv_apache_php_fpm`; (3) `/usr/local/cpanel/scripts/restartsrv_httpd`.
- Install/upgrade: all three steps once per transaction after it completes. Final removal: step 1 only.
- Every line the scripts print starts with `PlutoBook: `. With no cPanel/CloudLinux tools present they print nothing.
- Git: the project's CLAUDE.md reserves commits and pushes for explicit maintainer approval. Tasks end with a checkpoint (`git add` + `git diff --cached --stat`), not a commit; Task 6 asks for approval.
- Docs: plain, human language (see the `docs-style` memory in `bd memories docs`); every install page states the commands always fetch the latest release.

## Review Focus

1. **PIE was used before** (oblak.test has PIE's `80-plutobook.ini` in every ea-php): installing the package must say so, because PHP otherwise warns "Module 'plutobook' is already loaded" on every start. Test: Task 3 Step 1, `duplicate ini` assertion.
2. **EasyApache removes an ea-php version** and dnf/apt removes our package as a dependent: only the CageFS step runs, once, and the removal succeeds. Test: Task 3 Step 1, `removed with ea-php` assertion.
3. **Reinstall or upgrade to the next release** (same name): the refresh runs once, the old package's removal script does nothing, no flag file is left. Test: Task 3 Step 1, `reinstall` assertion.
4. **CloudLinux tools present but CageFS has no mounts** (`cagefs.mp` empty or comments only): `cagefsctl` is not run, restarts still happen. Test: Task 3 Step 1, `no mounts` assertion.
5. **deb `apt purge` after `apt remove`**, and plain `apt remove`: nothing of ours is left that makes PHP load the module. Test: Task 2 Step 1 (check-install removes with `purge`) and Task 3 Step 1 (`apt-get remove` without purge, then `php -m`).

---

### Task 1: Build the cPanel packages

**Files:**
- Modify: `scripts/build-packages` (header comment; new `cpanel_deb`, `cpanel_rpm`, `cpanel_cagefs_cfg`, `cpanel_hooks`; main loop)
- Create: `tools/packages/cpanel/20-plutobook.ini`
- Create: `tools/packages/cpanel/refresh.sh` (library only in this task: functions defined, see Interfaces; Task 3 tests it)
- Modify: `.github/scripts/release-assets.py` (extract `linux_assets`, add cPanel names)

**Interfaces:**
- Produces (build output in `build/portable/packages/`): the 20 file names in Global Constraints.
- Produces (`tools/packages/cpanel/refresh.sh`, POSIX sh, no shebang, prepended to every hook script by `cpanel_hooks`):
  - `PLUTOBOOK_FLAG=/run/oblak-ea-plutobook.refresh`, `PLUTOBOOK_TRIGGER=oblak-ea-plutobook-refresh`
  - `plutobook_cagefs_update` — step 1 (guarded)
  - `plutobook_restart_services` — steps 2 and 3
  - `plutobook_refresh_due` — creates the flag; if that fails, runs `plutobook_cagefs_update; plutobook_restart_services` at once
  - `plutobook_refresh_if_due` — if the flag exists: remove it, run steps 1–3
  - `plutobook_check_duplicate NN` — prints `PlutoBook: warning: <file> also loads plutobook; remove it` for every file in `/opt/cpanel/ea-phpNN/root/etc/php.d/*.ini` other than `20-plutobook.ini` with a line matching `^[[:space:]]*extension[[:space:]]*=[[:space:]]*"?plutobook(\.so)?"?[[:space:]]*$`
  - Failures print `PlutoBook: warning: <command> failed; run it by hand`.
- Produces (`cpanel_hooks DIR NN`, bash function in `build-packages`): writes `DIR/post`, `DIR/posttrans`, `DIR/postun` (rpm) and `DIR/postinst`, `DIR/postrm` (deb), each `#!/bin/sh` + `refresh.sh` + one action block + `exit 0`:
  - rpm `post`: `plutobook_check_duplicate NN; plutobook_refresh_due`
  - rpm `posttrans`: `plutobook_refresh_if_due`
  - rpm `postun`: `[ "$1" = 0 ] && plutobook_cagefs_update`
  - deb `postinst`: `configure)` → `plutobook_check_duplicate NN; plutobook_refresh_due; dpkg-trigger --no-await "$PLUTOBOOK_TRIGGER" || plutobook_refresh_if_due`; `triggered)` → `plutobook_refresh_if_due`
  - deb `postrm`: `remove)` → `plutobook_cagefs_update`
- Produces (`.github/scripts/release-assets.py`): `linux_assets(version: str) -> set[str]`, used by `collect`.
- nFPM keys: rpm `scripts.postinstall`, `scripts.postremove`, `rpm.scripts.posttrans`; deb `scripts.postinstall`, `scripts.postremove`, `deb.triggers.interest_noawait: [oblak-ea-plutobook-refresh]`.

- [ ] **Step 1: Stage modules for local builds**

Local builds need `build/portable/dist/` (ignored by git). Fill it from the 1.1.0 release instead of the multi-hour stack build, in a fresh empty download directory under `$CLAUDE_JOB_DIR/tmp/rel-1.1.0/`:
- `gh release download 1.1.0 --repo oblakhost/php-plutobook --pattern 'php_plutobook-1.1.0_php8.?-x86_64-linux-glibc-nts.zip' --pattern 'php-plutobook-1.1.0-1.php8?.el8.x86_64.rpm' --pattern SHA256SUMS`; check with `sha256sum --ignore-missing -c SHA256SUMS`.
- For X.Y in 8.2–8.6: `dist/linux/phpX.Y/plutobook.so` from the zip, `dist/el8/phpX.Y/plutobook.so` from the el8 rpm (`rpm2cpio | cpio -idm`), `phpapi` in both = 20220829, 20230831, 20240924, 20250925, 20261112.
- `dist/licenses/` = the el8 rpm's `/usr/share/licenses/php-plutobook/bundled/` contents.

Run: `PLUTOBOOK_RELEASE_VERSION=1.1.0 scripts/build-packages`
Expected: succeeds and lists today's 20 x86_64 files (5 debs, 15 rpms); no `oblak-ea-*` yet.

- [ ] **Step 2: Write the asset-name check (fails)**

Run:
```bash
python3 -B - <<'PY'
import importlib.util, pathlib, sys
spec = importlib.util.spec_from_file_location("ra", ".github/scripts/release-assets.py")
ra = importlib.util.module_from_spec(spec); spec.loader.exec_module(ra)
want = {n for n in ra.linux_assets("1.1.0") if n.startswith("oblak-ea-")}
have = {p.name for p in pathlib.Path("build/portable/packages").glob("oblak-ea-*")}
assert len(want) == 20 and want == have, (sorted(want ^ have))
assert "oblak-ea-php83-plutobook-1.1.0-1.el9.x86_64.rpm" in want
assert "oblak-ea-php86-plutobook_1.1.0-1_amd64.deb" in want
assert not any("aarch64" in n or "arm64" in n for n in want)
assert len(ra.linux_assets("1.1.0")) == 71
print("asset names OK")
PY
```
Expected: FAIL (`linux_assets` does not exist).

- [ ] **Step 3: Extract `linux_assets(version)` and add the cPanel names**

Move the expected-name loop out of `collect` unchanged (51 names), then add per PHP minor, x86_64 only: one `oblak-ea-phpNN-plutobook_{version}-1_amd64.deb` and `oblak-ea-phpNN-plutobook-{version}-1.el{8,9,10}.x86_64.rpm`. `collect` calls `check_names(linux, linux_assets(version))`.

- [ ] **Step 4: Implement the build**

- `tools/packages/cpanel/20-plutobook.ini`: the same two lines as `tools/packages/rpm/40-plutobook.ini` (`; Enable the PlutoBook HTML-to-PDF extension`, `extension=plutobook.so`).
- `refresh.sh`: the functions in Interfaces. No `set -e`; every command guarded.
- `cpanel_cagefs_cfg FILE NN FONT_PATHS` writes `[oblak-ea-phpNN-plutobook]`, `comment=DejaVu fonts for the PlutoBook PHP extension (ea-phpNN)`, `paths=<FONT_PATHS>`. Font paths per system are the font package's directories and fontconfig snippets (no docs/licenses), taken from `rpm -ql dejavu-sans-fonts` in `almalinux:8`, `:9`, `:10` and `dpkg -L fonts-dejavu-core` in `ubuntu:22.04`/`:24.04`, directories with a trailing `/`. Already known: EL9 `/usr/share/fonts/dejavu-sans-fonts/, /etc/fonts/conf.d/20-unhint-small-dejavu-sans.conf, /etc/fonts/conf.d/57-dejavu-sans-fonts.conf, /usr/share/fontconfig/conf.avail/20-unhint-small-dejavu-sans.conf, /usr/share/fontconfig/conf.avail/57-dejavu-sans-fonts.conf`. If 22.04 and 24.04 differ, the deb lists the union.
- `cpanel_rpm EL MINOR` and `cpanel_deb MINOR` follow `rpm`/`deb`: name, release, depends, contents (module, ini, cfg, license files under `oblak-ea-phpNN-plutobook` names, as the general packages lay them out), scripts from `cpanel_hooks`, deb triggers. Descriptions: summary `PlutoBook HTML-to-PDF extension for cPanel ea-phpNN`; body says it is for cPanel EasyApache 4's ea-phpNN and that installing restarts PHP-FPM and Apache and refreshes CloudLinux CageFS.
- Main loop: after the existing calls, `if [[ $arch == amd64 ]]; then cpanel_deb "$minor"; cpanel_rpm 9 "$minor"; cpanel_rpm 10 "$minor"; cpanel_rpm 8 "$minor"; fi`. Update the header comment's file list.

- [ ] **Step 5: Run the asset-name check and inspect the packages**

Run: `PLUTOBOOK_RELEASE_VERSION=1.1.0 scripts/build-packages`, then the Step 2 script. Expected: `asset names OK`.

Run and compare:
- `rpm -qp --requires build/portable/packages/oblak-ea-php84-plutobook-1.1.0-1.el9.x86_64.rpm` → includes `ea-php84-php(api) = 20240924-64`, `fontconfig`, `dejavu-sans-fonts`, `libcurl.so.4()(64bit)`; the el8 file has `libgnutls.so.30()(64bit)` and `libnettle.so.6()(64bit)` and no libcurl.
- `rpm -qp --qf '[%{FILEMODES:perms} %{FILEFLAGS:fflags} %{FILENAMES}\n]' …el9…rpm` → `-rwxr-xr-x  …/modules/plutobook.so`, `cn` on `…/php.d/20-plutobook.ini`, the cfg present.
- `rpm -qp --scripts …el9…rpm` → `postinstall`, `postuninstall`, `posttrans` scriptlets present.
- `dpkg-deb -f …php85…deb Depends` → exactly the Global Constraints deb list.
- `dpkg-deb --ctrl-tarfile …deb | tar -t` → `./postinst`, `./postrm`, `./triggers`, and **no** `./conffiles`; `dpkg-deb --info …deb triggers` → `interest-noawait oblak-ea-plutobook-refresh`.
- `bash -n scripts/build-packages`; `sh -n` on each generated hook in `build/packages/cpanel-*/`.

- [ ] **Step 6: Checkpoint**

`git add scripts/build-packages tools/packages/cpanel .github/scripts/release-assets.py && git diff --cached --stat`

---

### Task 2: Install tests in clean containers

**Files:**
- Modify: `scripts/test-packages` (EA4 setup helpers; scenario format; cPanel scenarios)
- Modify: `tools/packages/check-install` (cPanel layout)

**Interfaces:**
- Consumes: Task 1 package names.
- Produces: scenario format `name|image|globs|setup[|checker]` — `globs` is space-separated (each match mounted under `/pkg/`), `checker` defaults to `check-install`; the whole `tools/packages/` directory is mounted read-only at `/tools`, run as `sh /tools/$checker`.
- Produces (bash helpers in `test-packages`): `ea4_el PATH GPGCHECK` (writes `/etc/yum.repos.d/EA4.repo`: `[EA4]`, `baseurl=http://httpupdate.cpanel.net/cpanelsync/repos/PATH/EA4/`, `gpgcheck=GPGCHECK`, `gpgkey=https://securedownloads.cpanel.net/cPanelPublicPkgKey.asc`), `ea4_ubuntu u22|u24` (installs `ca-certificates curl`, key to `/etc/apt/keyrings/cpanel-ea4.asc`, source `deb [signed-by=/etc/apt/keyrings/cpanel-ea4.asc] mirror://httpupdate.cpanel.net/ea4-u22-mirrorlist ./`).

- [ ] **Step 1: Add the cPanel scenarios (fail)**

Inside `if [[ $rpm == x86_64 ]]`, append:

| name | image | globs | setup |
| --- | --- | --- | --- |
| `cpanel-el8-php8.2` | `almalinux:8` | `oblak-ea-php82-plutobook-*.el8.x86_64.rpm` | `ea4_el CentOS/8 1` |
| `cpanel-el9-php8.4` | `almalinux:9` | `oblak-ea-php84-plutobook-*.el9.x86_64.rpm` | `ea4_el CentOS/9 1` |
| `cpanel-el10-php8.6` | `almalinux:10` | `oblak-ea-php86-plutobook-*.el10.x86_64.rpm` | `ea4_el Almalinux/10 0` |
| `cpanel-ubuntu2204-php8.3` | `ubuntu:22.04` | `oblak-ea-php83-plutobook_*_amd64.deb` | `ea4_ubuntu u22` |
| `cpanel-ubuntu2404-php8.5` | `ubuntu:24.04` | `oblak-ea-php85-plutobook_*_amd64.deb` | `ea4_ubuntu u24; ulimit -n 1024` |

Comments in the script record why: EL10's rpm-sequoia rejects cPanel's SHA-1-signed key, so the test repo has `gpgcheck=0`; cPanel's Ubuntu 24.04 ea-php aborts ("buffer overflow detected") under podman's default open-file limit, and `ulimit` in setup carries into the check because setup runs in the same shell.

Run: `scripts/test-packages cpanel-el9-php8.4`
Expected: FAIL (check-install installs `php-cli`, not ea-php).

- [ ] **Step 2: Teach check-install the cPanel layout**

- Name from the package itself: deb `dpkg-deb -f "$pkg" Package`, rpm `rpm -qp --qf '%{NAME}' "$pkg"`.
- If the name matches `oblak-ea-php*-plutobook`: `nn` from the name, `php=/opt/cpanel/ea-php$nn/root/usr/bin/php`, PHP packages `ea-php$nn-php-cli` (deb: plus `libxml2`, which cPanel's debs forget). Otherwise `php=php` and today's packages.
- All `php` invocations use `"$php"`.
- cPanel mode only: capture the package install's output and fail if any line starts with `PlutoBook: ` (no tools present, so the scripts must stay silent).
- Leftover list adds `/opt/cpanel/ea-php*/root/usr/lib64/php/modules/plutobook.so`, `/opt/cpanel/ea-php*/root/etc/php.d/20-plutobook.ini`, `/etc/cagefs/conf.d/oblak-ea-php*-plutobook.cfg`, `/usr/share/licenses/oblak-ea-php*-plutobook`, `/usr/share/doc/oblak-ea-php*-plutobook`, `/run/oblak-ea-plutobook.refresh`.
- `test-packages`: scenario format and `/tools` mount from Interfaces.

- [ ] **Step 3: Run every scenario**

Run: `scripts/test-packages`
Expected: all 13 existing and 5 cPanel scenarios print `PHP 8.x.y, 1.1.0, Zdravo, свете … removed cleanly`; exit 0. While here, record the actual font paths for Task 1 Step 4 if any were guessed, and rebuild if they changed.

- [ ] **Step 4: Checkpoint**

`git add scripts/test-packages tools/packages/check-install && git diff --cached --stat`

---

### Task 3: Install and remove scripts behave once per transaction

**Files:**
- Create: `tools/packages/check-cpanel-scripts`
- Modify: `scripts/test-packages` (two scenarios)
- Modify: `tools/packages/cpanel/refresh.sh`, `scripts/build-packages` (only if the test exposes a defect)

**Interfaces:**
- Consumes: Task 1 hooks and function names; Task 2 scenario format.

- [ ] **Step 1: Write the stub test**

`tools/packages/check-cpanel-scripts` (POSIX sh, `set -eu`), run with two packages in `/pkg/` (versions A and B):
- Stubs at `/usr/sbin/cagefsctl`, `/usr/local/cpanel/scripts/restartsrv_apache_php_fpm`, `/usr/local/cpanel/scripts/restartsrv_httpd`: append `"<basename> <args> modules=<count of /opt/cpanel/ea-php*/root/usr/lib64/php/modules/plutobook.so>"` to `/tmp/calls`; `exit 1` if `/tmp/stub-fail` exists. `/etc/cagefs/cagefs.mp` contains `/opt`.
- `expect_calls LINE...` compares `/tmp/calls` with the given lines exactly, then empties it.
- Assertions, each a named block printing its name on failure; every block starts with an empty `/tmp/calls`:
  1. `one refresh`: install `ea-phpA-php-cli ea-phpB-php-cli` (deb: `libxml2`) first, then both packages in **one** install command → `cagefsctl --update --force-update modules=2`, `restartsrv_apache_php_fpm modules=2`, `restartsrv_httpd modules=2`; both PHPs list plutobook; no `/run/oblak-ea-plutobook.refresh`.
  2. `reinstall`: `dnf -y reinstall /pkg/A` / `apt-get install --reinstall /pkg/A` → the same three lines, `modules=2`, nothing else.
  3. `remove`: remove package A → only `cagefsctl --update --force-update modules=1`; on deb use `apt-get remove` (no purge) and check `ea-phpA`'s `php -m` has no plutobook and no startup warning on stderr.
  4. `duplicate ini`: write `extension=plutobook.so` to `/opt/cpanel/ea-phpA/root/etc/php.d/80-plutobook.ini`, install A → output contains `PlutoBook: warning: /opt/cpanel/ea-phpA/root/etc/php.d/80-plutobook.ini also loads plutobook; remove it`; delete that file.
  5. `failing tools`: `touch /tmp/stub-fail`; remove and install A → both exit 0, A loads plutobook, output contains `PlutoBook: warning:`; `rm /tmp/stub-fail`.
  6. `no mounts`: `cagefs.mp` becomes `# none`; reinstall A → `restartsrv_apache_php_fpm modules=2`, `restartsrv_httpd modules=2`, no `cagefsctl` line.
  7. `removed with ea-php`: restore `/opt` in `cagefs.mp`; `dnf -y remove ea-phpA-php-common` / `apt-get -y remove ea-phpA-php-common` → our A package is gone too, calls are exactly `cagefsctl --update --force-update modules=1`.
- Ends with `echo scripts OK`.

Scenarios (x86_64 only), checker `check-cpanel-scripts`:
- `cpanel-el9-scripts` | `almalinux:9` | `oblak-ea-php82-plutobook-*.el9.x86_64.rpm oblak-ea-php83-plutobook-*.el9.x86_64.rpm` | `ea4_el CentOS/9 1`
- `cpanel-ubuntu2204-scripts` | `ubuntu:22.04` | `oblak-ea-php84-plutobook_*_amd64.deb oblak-ea-php86-plutobook_*_amd64.deb` | `ea4_ubuntu u22`

- [ ] **Step 2: Run them**

Run: `scripts/test-packages cpanel-el9-scripts cpanel-ubuntu2204-scripts`
Expected: both end with `scripts OK`. If a block fails, fix `refresh.sh`/`cpanel_hooks` (not the test) and rebuild. Specific risk: if dpkg never calls `postinst triggered` for the trigger a package activated itself, `one refresh` shows no calls on deb; then activate the trigger declaratively instead (nFPM `deb.triggers.activate_noawait: [oblak-ea-plutobook-refresh]`, dropping the `dpkg-trigger` call) and re-run.

- [ ] **Step 3: Full regression**

Run: `scripts/test-packages`
Expected: all 20 scenarios pass.

- [ ] **Step 4: Checkpoint**

`git add tools/packages scripts && git diff --cached --stat`

---

### Task 4: Documentation

**Files:**
- Create: `docs/install/cpanel.md`
- Modify: `docs/install/index.md` (platform table row), `website/sidebars.js` (`'install/cpanel'` after `'install/rhel'`), `INSTALL.md` (new section after "RHEL, AlmaLinux, Rocky Linux and CloudLinux", tested-combinations rows, table-of-contents link), `README.md` (platform table row linking `INSTALL.md#cpanel-servers`), `docs/install/rhel.md` and `docs/install/debian-ubuntu.md` (one line pointing cPanel servers to the new page), `CONTRIBUTING.md` (only if it lists `build-packages` outputs)

- [ ] **Step 1: Write the page**

Sections, plain language, x86_64 only: who it's for (cPanel servers with EasyApache 4 PHP, AlmaLinux/Rocky/CloudLinux 8–10, Ubuntu 22.04/24.04); install; what happens automatically (PHP-FPM and Apache restart, CageFS refresh on CloudLinux); check (`/opt/cpanel/ea-php84/root/usr/bin/php --ri plutobook`); installed files table; settings (own ini or MultiPHP INI Editor, not `20-plutobook.ini`); removing; notes: removing an ea-php through EasyApache removes its package too; ELevate (remove the packages first, install the ones for the new version after); LiteSpeed Web Server (restart it yourself); after PIE (delete PIE's `80-plutobook.ini`, the package warns about it).

Install commands (they install for every ea-php 8.2–8.6 on the server):

```sh
el=$(rpm -E %rhel)
version=$(basename "$(curl -fsSLo /dev/null -w '%{url_effective}' https://github.com/oblakhost/php-plutobook/releases/latest)")
base=https://github.com/oblakhost/php-plutobook/releases/download/$version
sudo rpm --import "$base/php-plutobook-signing-key.asc"
for php in $(ls /opt/cpanel | grep -E '^ea-php8[2-6]$'); do
  curl -fLO "$base/oblak-$php-plutobook-$version-1.el$el.x86_64.rpm"
done
rpm -K oblak-ea-php*-plutobook-*.rpm   # each reports "digests signatures OK"
sudo dnf install ./oblak-ea-php*-plutobook-*.rpm
```

```sh
version=$(basename "$(curl -fsSLo /dev/null -w '%{url_effective}' https://github.com/oblakhost/php-plutobook/releases/latest)")
base=https://github.com/oblakhost/php-plutobook/releases/download/$version
for php in $(ls /opt/cpanel | grep -E '^ea-php8[2-6]$'); do
  curl -fLO "$base/oblak-$php-plutobook_${version}-1_amd64.deb"
done
sudo apt install ./oblak-ea-php*-plutobook_*_amd64.deb
```

Index row: `| ![](../assets/icons/linux.svg) cPanel (EasyApache 4) | AlmaLinux, Rocky Linux, CloudLinux 8, 9, 10; Ubuntu 22.04, 24.04 | EasyApache 4 (ea-php82 to ea-php86) | [cPanel package](cpanel.md) |`. INSTALL.md's section mirrors the page; its tested-combinations table gains the five cPanel scenarios (system, ea-php version, "cPanel EasyApache 4").

- [ ] **Step 2: Check the site builds and links resolve**

Run: `npm --prefix website ci && npm --prefix website run build` (use the project's Node per `.node-version`; if the toolchain is missing, follow CONTRIBUTING.md's docs section).
Expected: build succeeds with no broken-link warnings for `install/cpanel`.

- [ ] **Step 3: Checkpoint**

`git add docs/install website/sidebars.js INSTALL.md README.md CONTRIBUTING.md && git diff --cached --stat`

---

### Task 5: Real-server check on oblak.test

Maintainer's go-ahead: 2026-11-27 ("we have a ssh setup you can use that machine"). Host `oblak.test` (CloudLinux 9.9, cPanel 138, ea-php82–85 under mod_lsapi with CageFS, site `npr.rs` → ea-php84, IP 116.202.98.212). Leave the server with our packages installed for ea-php82–85 in place of PIE's install.

**Files:** none in the repo. Results go into the bead notes (`bd update pdf-ext-zcs.6 --append-notes`).

- [ ] **Step 1: Record the starting state and return the cage to its original fonts**

Run on the server: list `/opt/cpanel/ea-php8*/root/etc/php.d/*plutobook*`, `cagefsctl --list-rpm | grep -i dejavu`. Then remove PIE's install (`80-plutobook.ini` and `plutobook.so` in each ea-php82–85) and undo the earlier manual font step: `cagefsctl --delrpm dejavu-sans-fonts && cagefsctl --update --force-update`.
Expected: `/usr/share/cagefs-skeleton/usr/share/fonts/` no longer has `dejavu-sans-fonts`.

- [ ] **Step 2: Install the four el9 packages in one transaction**

Copy `oblak-ea-php8{2,3,4,5}-plutobook-1.1.0-1.el9.x86_64.rpm` from the local build (Task 1, 1.1.0 modules) to `/root/`; `dnf -y install /root/oblak-ea-php8*-plutobook-*.rpm 2>&1 | tee /root/plutobook-install.log`.
Expected: the log shows one `PlutoBook:` CageFS line and one each for PHP-FPM and Apache (not four of each); `/run/oblak-ea-plutobook.refresh` absent; each `/opt/cpanel/ea-php8N/root/usr/bin/php --ri plutobook` shows version 1.1.0; `ls /usr/share/cagefs-skeleton/usr/share/fonts/` has `dejavu-sans-fonts`.

- [ ] **Step 3: Check a website inside CageFS**

Put a probe at `/home/npr/public_html/zz-plutobook-probe.php` (owner npr) that loads `<h1>Zdravo, свете</h1>` with `loadHtml()`, writes it with `writeToFile()` to a file under `sys_get_temp_dir()`, sends it with `readfile()` and deletes it; fetch it with `curl -sk --resolve npr.rs:443:116.202.98.212 https://npr.rs/zz-plutobook-probe.php -o probe.pdf`.
Expected locally: `pdftotext probe.pdf -` prints `Zdravo, свете`; `pdffonts probe.pdf` lists a DejaVu font embedded. The lsphp group serving it started after the install (its start time from `ps -o lstart` is later than the dnf log).

- [ ] **Step 4: Remove one package, then restore it**

`dnf -y remove oblak-ea-php85-plutobook` → log shows only the CageFS line; as npr inside the cage (`cagefs_enter_user npr /opt/cpanel/ea-php85/root/usr/bin/php -m`, or `su -s /bin/bash npr -c …` if that is unavailable) plutobook is absent and stderr has no "Unable to load dynamic library". Reinstall it with `dnf -y install /root/oblak-ea-php85-plutobook-*.rpm`.

- [ ] **Step 5: Clean up and record**

Remove the probe and `/root/*.rpm`. Append the results (commands, outputs that matter, anything that differed from the spec) to the bead. If anything failed, fix it in Tasks 1–3, rebuild, re-run the affected scenarios and repeat from Step 2.

---

### Task 6: Hand-off

- [ ] **Step 1: Final verification**

Run: `bash -n scripts/build-packages scripts/test-packages && sh -n tools/packages/check-install tools/packages/check-cpanel-scripts`, the Task 1 Step 2 asset check, `scripts/test-packages` (all 20), and `git status`.
Expected: all pass; only the files listed in Tasks 1–4 and `design/` changed.

- [ ] **Step 2: Ask for git approval**

Report what changed and the verification evidence, and ask the maintainer to approve committing (conventional commits: `feat(packages): build cPanel EasyApache 4 packages`, `test(packages): …`, `docs: …`, `docs(design): …`), pushing the branch and opening a draft PR. The PR's CI then builds with fresh modules on both architectures; merging releases the packages.

- [ ] **Step 3: Beads**

`bd update pdf-ext-zcs.6 --append-notes "<summary, PR link>"`; close the bead only after the release containing the packages is published and its assets are checked (as was done for zcs.16).
