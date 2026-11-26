"""Check the Windows renderer's imports and package its C ABI, sources and licenses."""
from pathlib import Path
import re
import subprocess
import sys
import zipfile


source = Path("/work/plutobook")
dll = Path("/work/libplutobook-0.dll")
table = subprocess.check_output(["x86_64-w64-mingw32-objdump", "-p", dll], text=True)
imports = sorted(set(re.findall(r"DLL Name: (\S+)", table, re.I)))
system_dlls = {"advapi32.dll", "gdi32.dll", "kernel32.dll", "msimg32.dll", "msvcrt.dll",
               "ole32.dll", "user32.dll"}
unexpected = [name for name in imports if name.lower() not in system_dlls]
if not imports or unexpected:
    sys.exit(f"Unexpected renderer DLL imports: {imports}")
print("Renderer DLL imports: " + ", ".join(imports))

files = {
    "bin/libplutobook-0.dll": dll,
    "include/plutobook.h": source / "include/plutobook.h",
}
for child in (source / "subprojects").iterdir():
    if not child.is_dir() or child.name in {"packagecache", "packagefiles"}:
        continue
    found = [f for pattern in ("LICENSE*", "LICENCE*", "COPYING*", "PATENTS", "README.ijg",
                                "docs/FTL.TXT", "docs/GPLv2.TXT") for f in child.glob(pattern) if f.is_file()]
    if not found:
        sys.exit(f"No redistribution notices found in {child}")
    for notice in found:
        files["licenses/" + child.name + "/" + notice.name] = notice
for notice in [source / "CREDITS", source / "LICENSE", *(source / "licenses").rglob("*")]:
    if notice.is_file():
        files["licenses/plutobook/" + str(notice.relative_to(source))] = notice
for pattern in (Path("/src/tools/patches"), source / "subprojects/packagefiles"):
    for patch in pattern.glob("*.patch"):
        files["patches/" + patch.name] = patch
for pkg in ("gcc-14-base", "gcc-mingw-w64-base", "mingw-w64-common"):
    files["licenses/" + pkg + "/copyright"] = Path("/usr/share/doc") / pkg / "copyright"
files["licenses/gcc-14-base/GPL-3"] = Path("/usr/share/common-licenses/GPL-3")
inputs = set(sys.argv[1:])
manifest = [line for line in Path("/src/tools/sources.sha256").read_text().splitlines()
            if line and not line.startswith("#") and line.split()[-1].rsplit("/", 1)[-1] in inputs]
if len(manifest) != len(inputs):
    sys.exit("Missing pinned renderer source")

out = Path("/work/renderer.zip")
with zipfile.ZipFile(out.with_suffix(".tmp"), "w") as bundle:
    for name in sorted([*files, "sources.sha256"]):
        entry = zipfile.ZipInfo(name, (1980, 1, 1, 0, 0, 0))
        entry.compress_type = zipfile.ZIP_DEFLATED
        data = ("\n".join(manifest) + "\n").encode() if name == "sources.sha256" else files[name].read_bytes()
        bundle.writestr(entry, data)
out.with_suffix(".tmp").replace(out)
