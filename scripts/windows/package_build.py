"""Package and test the private Windows development runtime produced by runner.py."""

import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import tarfile
import tempfile
import zipfile


LAUNCHER = r"""@echo off
setlocal DisableDelayedExpansion
set "FONTCONFIG_FILE=%~dp0etc\fonts\fonts.conf"
set "FONTCONFIG_PATH=%~dp0etc\fonts"
set "OPENSSL_CONF=%~dp0etc\openssl.cnf"
"%~dp0php.exe" -n -d "extension_dir=%~dp0." -d extension=php_openssl.dll -d extension=php_plutobook.dll %*
exit /b %errorlevel%
"""

FONTS_CONF = """<?xml version="1.0"?>
<fontconfig>
  <dir prefix="relative">../../share/fonts/dejavu</dir>
  <cachedir>LOCAL_APPDATA_FONTCONFIG_CACHE</cachedir>
</fontconfig>
"""

PROBE = r"""<?php
if (PHP_VERSION !== $argv[1] || PHP_OS_FAMILY !== 'Windows' || PHP_INT_SIZE !== 8 || PHP_ZTS || PHP_DEBUG ||
    !extension_loaded('openssl') || !extension_loaded('zlib') || !extension_loaded('plutobook')) {
    exit(1);
}
$doc = new PlutoBook\Document();
$doc->loadHtml('<p style="font-family: DejaVu Sans">Regular <b>Bold</b></p>');
$doc->writeToFile('smoke.pdf');
"""


def add_notices(bundle, run, source, deps, archives):
    for name in ("LICENSE", "THIRD_PARTY.md"):
        bundle.write(run / name, name)
    if source:
        for name in ("LICENSE", "README.REDIST.BINS", "ext/standard/libavifinfo/LICENSE",
                     "ext/standard/libavifinfo/PATENTS"):
            bundle.write(source / name, "licenses/php/" + name)
        # These PHP hash sources carry their binary redistribution notices in the opening comment.
        for name in ("ext/hash/xxhash/xxhash.h", "ext/hash/hash_sha_ni.c", "ext/hash/hash_sha_sse2.c"):
            file = source / name
            if file.exists():
                header = file.read_bytes().split(b"*/", 1)[0] + b"*/\n"
                bundle.writestr("licenses/php/" + name + ".notice", header)
    bundle.write(deps / "curl-src/COPYING", "licenses/libcurl/COPYING")
    with tarfile.open(archives["fonts"]) as fonts:
        for name in ("LICENSE", "AUTHORS"):
            member = next(e for e in fonts.getmembers() if e.name.endswith("/" + name))
            bundle.writestr("licenses/dejavu/" + name, fonts.extractfile(member).read())
    with zipfile.ZipFile(archives["patched_renderer"]) as renderer:
        for member in renderer.infolist():
            name = member.filename
            if member.is_dir():
                continue
            if name == "sources.sha256":
                bundle.writestr("renderer-sources.sha256", renderer.read(member))
            elif name.startswith(("licenses/", "patches/")):
                if "\\" in name or ":" in name or ".." in name.split("/"):
                    raise ValueError(f"Invalid renderer archive path: {name}")
                section, relative = name.split("/", 1)
                bundle.writestr(f"{section}/renderer/{relative}", renderer.read(member))
        if not any(name.startswith("licenses/") and not name.endswith("/") for name in renderer.namelist()):
            raise ValueError("Renderer archive is missing its bundled component licenses")
        renderer.getinfo("sources.sha256")
    if not source:
        bundle.write(deps / "zlib-src/LICENSE", "licenses/zlib/LICENSE")
    for component in ("tls", "zlib") if source else ():
        with zipfile.ZipFile(archives[component]) as archive:
            for name in archive.namelist():
                if not name.endswith("/") and name.startswith(("share/licenses/", "share/sbom/")):
                    bundle.writestr(name.removeprefix("share/"), archive.read(name))
    # Include the exact renderer source URL alongside the binary archive URL.
    names = {p.name for p in archives.values()}
    lines = []
    for line in (run / "tools/sources.sha256").read_text().splitlines():
        if line and not line.startswith("#"):
            url = line.split()[1]
            if url.rsplit("/", 1)[-1] in names or "/plutoprint/plutobook/tar.gz/" in url:
                lines.append(line)
    bundle.writestr("sources.sha256", "\n".join(lines) + "\n")


def runtime_env():
    env = {k: os.environ[k] for k in ("SystemRoot", "ComSpec", "TEMP", "TMP", "LOCALAPPDATA")
           if k in os.environ}
    env["PATH"] = str(Path(env["SystemRoot"]) / "System32")
    return env


def verify_fonts(caller):
    pdf = (caller / "smoke.pdf").read_bytes()
    embedded_faces = set(re.findall(rb"/FontName\s+/[A-Z]{6}\+([^\s/<>]+)", pdf))
    if not pdf.startswith(b"%PDF-") or not {b"DejaVuSans", b"DejaVuSans-Bold"} <= embedded_faces:
        raise ValueError("Extracted Windows build did not render a PDF with both packaged fonts")


def verify_package(archive, work, version):
    package = work / "unpacked build"
    caller = work / "separate working directory"
    caller.mkdir()
    with zipfile.ZipFile(archive) as bundle:
        bundle.extractall(package)
    (caller / "probe.php").write_text(PROBE, encoding="utf-8")
    env = runtime_env()
    subprocess.run([env["ComSpec"], "/d", "/c", "call", package / "php-plutobook.cmd",
                    "probe.php", version], check=True, cwd=caller, env=env)
    verify_fonts(caller)


def pie_basename(release, version, thread, compiler):
    minor = ".".join(version.split(".")[:2])
    return f"php_plutobook-{release}-{minor}-{thread}-{compiler}-x86_64"


def package_release(out, php, module, deps, archives, release, version, thread, compiler):
    from runtime_crt import copy_app_crt

    name = pie_basename(release, version, thread, compiler)
    result = out / (name + ".zip")
    with tempfile.TemporaryDirectory(dir=out) as scratch:
        work = Path(scratch)
        archive = work / result.name
        with zipfile.ZipFile(archive, "w", zipfile.ZIP_DEFLATED) as bundle:
            bundle.write(module, name + ".dll")
            bundle.write(module.parent / "libplutobook-0.dll", "libplutobook-0.dll")
            for face in ("DejaVuSans.ttf", "DejaVuSans-Bold.ttf"):
                bundle.write(deps / "share/fonts/dejavu" / face, "fonts/" + face)
            bundle.writestr("fonts.conf", FONTS_CONF.replace("../../share/fonts/dejavu", "fonts"))
            add_notices(bundle, out.parent, None, deps, archives)

        # Reproduce PIE's WindowsInstall layout under a clean PHP binary and no SDK PATH.
        runtime = work / "installed PHP"
        ext = runtime / "ext"
        ext.mkdir(parents=True)
        core = "php8ts.dll" if thread == "ts" else "php8.dll"
        for file in (php, php.parent / core):
            shutil.copyfile(file, runtime / file.name)
        copy_app_crt(php.parent, runtime)
        with zipfile.ZipFile(archive) as bundle:
            dlls = {item for item in bundle.namelist() if item.lower().endswith(".dll")}
            if dlls != {name + ".dll", "libplutobook-0.dll"}:
                raise ValueError(f"Unexpected DLL layout in {archive.name}: {dlls}")
            for item in bundle.namelist():
                if item == name + ".dll":
                    target = ext / "php_plutobook.dll"
                elif item == "libplutobook-0.dll":
                    target = runtime / item
                else:
                    target = runtime / "extras/plutobook" / item
                target.parent.mkdir(parents=True, exist_ok=True)
                with bundle.open(item) as src, target.open("wb") as dest:
                    shutil.copyfileobj(src, dest)
        caller = work / "separate working directory"
        caller.mkdir()
        env = runtime_env()
        cli = [runtime / php.name, "-n", "-d", f"extension_dir={ext}", "-d", "extension=plutobook"]
        probe = ("exit(PHP_VERSION === $argv[1] && phpversion('plutobook') === $argv[2] "
                 "&& (bool)PHP_ZTS === ($argv[3] === 'ts') && !PHP_DEBUG && PHP_INT_SIZE === 8 ? 0 : 1);")
        subprocess.run([*cli, "-r", probe, version, release, thread], cwd=caller, env=env, check=True)
        info = subprocess.check_output([*cli, "--ri", "plutobook"], cwd=caller, env=env, text=True)
        if not re.search(r"^HTTP client => .*Schannel", info, re.M | re.I):
            raise ValueError("Installed module did not report the Schannel HTTP client")
        subprocess.run([sys.executable, out.parent / "tests/integration/windows-fonts.py",
                        runtime / php.name, ext / "php_plutobook.dll"], cwd=caller, env=env, check=True)
        archive.replace(result)
    print(f"Verified installed PIE archive: {result.name}")
    return result


def package_build(out, php, module, deps, source, archives):
    run = out.parent
    version = archives["php"].name.removeprefix("php-").removesuffix(".tar.xz")
    result = out / f"php-plutobook-php-{version}-nts-win-x64.zip"
    with tempfile.TemporaryDirectory(dir=out) as scratch:
        work = Path(scratch)
        archive = work / result.name
        with zipfile.ZipFile(archive, "w", zipfile.ZIP_DEFLATED) as bundle:
            binaries = [php, php.parent / "php8.dll", php.parent / "php_openssl.dll", module,
                        module.parent / "libplutobook-0.dll"]
            binaries += [deps / "bin" / name for name in (
                "libcurl.dll", "libssl-3-x64.dll", "libcrypto-3-x64.dll")]
            for binary in binaries:
                bundle.write(binary, binary.name)
            for face in ("DejaVuSans.ttf", "DejaVuSans-Bold.ttf"):
                name = "share/fonts/dejavu/" + face
                bundle.write(deps / name, name)
            bundle.write(deps / "openssl.cnf", "etc/openssl.cnf")
            bundle.writestr("etc/fonts/fonts.conf", FONTS_CONF)
            bundle.writestr("php-plutobook.cmd", LAUNCHER.replace("\n", "\r\n"))
            bundle.writestr("README.txt", (
                f"Private development build for PHP {version} NTS on Windows x64.\r\n"
                "Requires the Microsoft Visual C++ x64 v14 runtime.\r\n"
                "Extract all files together and run php-plutobook.cmd your-script.php.\r\n"
                "The launcher loads PlutoBook and OpenSSL and selects the bundled DejaVu fonts.\r\n"
                "Build-input URLs and SHA-256 checksums are in sources.sha256 and renderer-sources.sha256.\r\n"
            ))
            add_notices(bundle, run, source, deps, archives)
        verify_package(archive, work, version)
        archive.replace(result)
    print(f"Verified extracted Windows development build: {result.name}")
    return result
