"""Expand the pinned PHP releases into Windows jobs and check the files sent for signing."""

import json
from pathlib import Path
import re
import shutil
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts/windows"))
from runner import compiler_for


def php_versions():
    pins = (ROOT / "tools/sources.sha256").read_text().splitlines()
    versions = [m[1] for pin in pins if (m := re.search(r"/php-(\d+\.\d+\.\d+)\.tar\.xz$", pin))]
    minors = {version.rsplit(".", 1)[0] for version in versions}
    if not versions or len(minors) != len(versions):
        sys.exit("Expected exactly one PHP source pin per supported minor")
    return versions


def windows_jobs(version):
    jobs = []
    for php in php_versions():
        minor = php.rsplit(".", 1)[0]
        compiler = compiler_for(php)
        # The 2022 image includes the VS2019 v142 toolset and the native VS2022 v143 toolset.
        image = {"vs16": "windows-2022", "vs17": "windows-2022", "vs18": "windows-2025-vs2026"}[compiler]
        for thread in ("nts", "ts"):
            jobs.append({
                "php": php, "thread": thread, "compiler": compiler, "image": image,
                "artifact": f"windows-release-{php}-{thread}",
                "zip": f"php_plutobook-{version}-{minor}-{thread}-{compiler}-x86_64.zip",
            })
    return jobs


def check_names(directory, expected):
    actual = {file.name for file in directory.iterdir()}
    if actual != expected:
        sys.exit(f"{directory}: missing {sorted(expected - actual)}, unexpected {sorted(actual - expected)}")


def linux_assets(version):
    expected = {f"php_plutobook-{version}-mpl-sources.tar.xz"}
    for php in php_versions():
        minor = php.rsplit(".", 1)[0]
        nodot = minor.replace(".", "")
        # Each architecture as PIE, Debian and RPM name it.
        for pie, deb, rpm in (("x86_64", "amd64", "x86_64"), ("arm64", "arm64", "aarch64")):
            expected.update({
                f"php_plutobook-{version}_php{minor}-{pie}-linux-glibc-nts.zip",
                f"php{minor}-plutobook_{version}-1_{deb}.deb",
            })
            for el in (8, 9, 10):
                expected.add(f"php-plutobook-{version}-1.php{nodot}.el{el}.{rpm}.rpm")
        # cPanel EasyApache 4 packages; cPanel runs only on x86_64.
        expected.add(f"oblak-ea-php{nodot}-plutobook_{version}-1_amd64.deb")
        for el in (8, 9, 10):
            expected.add(f"oblak-ea-php{nodot}-plutobook-{version}-1.el{el}.x86_64.rpm")
    return expected


def collect(version):
    linux = ROOT / "build/linux-release"
    windows = ROOT / "build/windows-release"
    releases = ROOT / "artifacts/releases"
    check_names(linux, linux_assets(version))
    assets = list(linux.iterdir())

    jobs = windows_jobs(version)
    check_names(windows, {job["artifact"] for job in jobs})
    for job in jobs:
        directory = windows / job["artifact"]
        check_names(directory, {job["zip"], "cli.pdf"})
        # Poppler writes reports and page images next to the PDF. Keep the downloaded inputs intact.
        inspection = ROOT / "build/windows-invoices" / job["artifact"]
        inspection.mkdir(parents=True, exist_ok=True)
        invoice = inspection / "cli.pdf"
        shutil.copyfile(directory / "cli.pdf", invoice)
        subprocess.run([sys.executable, ROOT / "tests/integration/inspect-invoice.py", invoice], check=True)
        assets.append(directory / job["zip"])

    for file in assets:
        if not file.is_file() or not file.stat().st_size:
            sys.exit(f"Missing or empty release asset: {file}")
    releases.mkdir(parents=True, exist_ok=True)
    for file in assets:
        shutil.copyfile(file, releases / file.name)
    check_names(releases, {file.name for file in assets})
    print(f"Verified {len(assets)} release assets, including {len(jobs)} Windows ZIPs")


if __name__ == "__main__":
    command, version = sys.argv[1:]
    if command == "matrix":
        print("matrix=" + json.dumps({"include": windows_jobs(version)}, separators=(",", ":")))
    elif command == "collect":
        collect(version)
    else:
        sys.exit("Usage: release-assets.py matrix|collect RELEASE_VERSION")
