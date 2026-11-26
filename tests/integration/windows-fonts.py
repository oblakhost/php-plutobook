"""Check the installed PIE fonts and Fontconfig overrides with an unwrapped php.exe."""
import ctypes
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import tempfile
import xml.etree.ElementTree as ET


php, extension = (Path(arg).resolve(strict=True) for arg in sys.argv[1:])
packaged = php.parent / "extras" / "plutobook"
probe = r"""
$d = new PlutoBook\Document();
$d->loadHtml('<p style="font-family:DejaVu Sans">Regular <b>Bold</b></p>');
echo $d->toPdf();
"""
env = {k: v for k, v in os.environ.items() if k.upper() not in ("FONTCONFIG_FILE", "FONTCONFIG_PATH")}


def check_fonts(cwd, overrides, expected, executable=php):
    result = subprocess.run([executable, "-n", "-d", f"extension={extension}", "-r", probe],
                            cwd=cwd, env={**env, **overrides}, check=True, stdout=subprocess.PIPE)
    faces = set(re.findall(rb"/FontName\s+/[A-Z]{6}\+([^\s/<>]+)", result.stdout))
    if faces != expected:
        raise ValueError(f"{overrides or 'Packaged configuration'}: expected {expected}, got {faces}")


with tempfile.TemporaryDirectory() as tmp:
    work = Path(tmp)
    check_fonts(work, {}, {b"DejaVuSans", b"DejaVuSans-Bold"})
    # PHP reads the extension's CLI argument in its system code page, so load the
    # original DLL while running PHP and finding extras under a Unicode install path.
    unicode_install = work / "PHP Ž日"
    shutil.copytree(php.parent, unicode_install)
    short = ctypes.create_unicode_buffer(32768)
    length = ctypes.windll.kernel32.GetShortPathNameW(str(unicode_install), short, len(short))
    config_path = short.value if 0 < length < len(short) else str(unicode_install)
    if config_path.encode("mbcs", errors="replace").decode("mbcs") == config_path:
        check_fonts(work, {}, {b"DejaVuSans", b"DejaVuSans-Bold"}, unicode_install / php.name)
    else:
        print("Skipped Unicode installation path: no 8.3 alias in the system code page")
    # In a separate PHP process, an explicit configuration exposes just the bold face.
    fonts = work / "fonts"
    fonts.mkdir()
    bold, = packaged.rglob("DejaVuSans-Bold.ttf")
    shutil.copy(bold, fonts)
    config = ET.Element("fontconfig")
    ET.SubElement(config, "dir").text = fonts.as_posix()
    ET.SubElement(config, "cachedir").text = (work / "cache").as_posix()
    custom = work / "fonts.conf"
    ET.ElementTree(config).write(custom, encoding="utf-8")
    for override in ({"FONTCONFIG_FILE": str(custom)}, {"FONTCONFIG_PATH": str(work)}):
        check_fonts(work, override, {b"DejaVuSans-Bold"})
print("Verified packaged DejaVu fonts and both Fontconfig overrides")
