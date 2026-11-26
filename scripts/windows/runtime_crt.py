"""Use a private Visual C++ redistributable with the test PHP executable."""

import filecmp
import os
from pathlib import Path
import re
import shutil
import subprocess


def stage_private_crt(private_tools, directory):
    if private_tools is None:
        return
    toolset = tuple(map(int, private_tools.name.split(".")[:2]))
    system_dll = Path(os.environ["SystemRoot"]) / "System32/vcruntime140.dll"
    if system_dll.is_file():
        info = subprocess.check_output(
            ["powershell.exe", "-NoProfile", "-NonInteractive", "-Command",
             "(Get-Item -LiteralPath $env:SystemRoot/System32/vcruntime140.dll).VersionInfo.FileVersionRaw.ToString()"],
            text=True,
        ).strip()
        if tuple(map(int, info.split(".")[:2])) >= toolset:
            return
    redist = private_tools.parents[2] / "Redist/MSVC"
    versions = sorted((p for p in redist.glob("*") if re.fullmatch(r"\d+(\.\d+)+", p.name)),
                      key=lambda p: tuple(map(int, p.name.split("."))), reverse=True)
    for version in versions:
        if tuple(map(int, version.name.split(".")[:2])) != toolset:
            continue
        crt = next(version.glob("x64/Microsoft.VC*.CRT/vcruntime140.dll"), None)
        if crt is not None:
            copy_app_crt(crt.parent, directory)
            return
    raise RuntimeError(f"Missing matching private x64 runtime under {redist}")


def copy_app_crt(source, target):
    # Only the isolated PHP test installation gets the app-local redistributable.
    for pattern in ("vcruntime140*.dll", "msvcp140*.dll", "concrt140.dll", "vccorlib140.dll"):
        for dll in source.glob(pattern):
            destination = target / dll.name
            # Another run may be using the same cached PHP and holding its DLLs open.
            if destination.is_file() and filecmp.cmp(dll, destination, shallow=False):
                continue
            shutil.copyfile(dll, destination)
