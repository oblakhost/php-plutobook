"""Build and test in the build account's private pdf-ext directory on Windows."""
import fnmatch
import hashlib
import json
import lzma
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import tarfile
import tempfile
import time
import xml.etree.ElementTree as ET
import zipfile


def compiler_for(version):
    return {"8.2": "vs16", "8.3": "vs16", "8.4": "vs17", "8.5": "vs17",
            "8.6": "vs18"}[".".join(version.split(".")[:2])]


def inputs(version, mode="suite"):
    if not re.fullmatch(r"\d+\.\d+\.\d+", version):
        sys.exit("Expected a pinned PHP release number")
    manifest = {}
    for line in Path("tools/sources.sha256").read_text().splitlines():
        if line and not line.startswith("#"):
            checksum, url = line.split()
            manifest[url] = checksum
    files = {}
    for key, pattern in {
        "php": f"*/php-{version}.tar.xz", "sdk": "*/php/php-sdk-binary-tools/tar.gz/*",
        "renderer": "*/plutobook-*-win-x64.zip", "fonts": "*/dejavu-fonts-ttf-*.tar.bz2",
        "curl": "*/curl-*.tar.gz", "tls": "*/openssl-*-vs17-x64.zip",
        "zlib": "*/zlib-*.tar.xz" if mode == "release" else "*/zlib-*-vs17-x64.zip",
    }.items():
        matches = fnmatch.filter(manifest.keys(), pattern)
        if len(matches) != 1:
            sys.exit(f"Expected one pin for {pattern} in tools/sources.sha256")
        url = matches[0]
        files[key] = (url.rsplit("/", 1)[1], manifest[url])
    return files


def digest(file):
    with file.open("rb") as stream:
        return hashlib.file_digest(stream, "sha256").hexdigest()


def unpack(archive, directory):
    marker = directory / ".unpacked"
    if marker.exists():
        return
    directory.mkdir(parents=True, exist_ok=True)
    cmd = ["tar", "-xf", archive, "--strip-components", "1", "-C", directory]
    if archive.suffix == ".xz":
        # The Windows 10 inbox tar has no xz decoder.
        cmd[2] = "-"
        with lzma.open(archive) as data, subprocess.Popen(cmd, stdin=subprocess.PIPE) as tar:
            shutil.copyfileobj(data, tar.stdin)
            tar.stdin.close()
            if tar.wait():
                sys.exit(f"Could not extract {archive}")
    else:
        subprocess.run(cmd, check=True)
    marker.touch()


def prepare_deps(archives, deps, sdk_task):
    if (deps / ".built").exists():
        return
    deps.mkdir(parents=True, exist_ok=True)
    # PHP's openssl extension serves the local TLS test fixture. The release module uses Schannel.
    with zipfile.ZipFile(archives["tls"]) as archive:
        archive.extractall(deps)
    if archives["zlib"].suffix == ".zip":
        with zipfile.ZipFile(archives["zlib"]) as archive:
            archive.extractall(deps)
    else:
        unpack(archives["zlib"], deps / "zlib-src")
    with zipfile.ZipFile(archives["renderer"]) as archive:
        (deps / "include/plutobook.h").write_bytes(archive.read("plutobook-win-x64/include/plutobook/plutobook.h"))
    # Only the upstream public header is needed to generate our MSVC import library.
    header = (deps / "include/plutobook.h").read_text(encoding="utf-8")
    symbols = re.findall(r"^PLUTOBOOK_API [^\n(]+?\b(plutobook_\w+)\(", header, re.M)
    (deps / "plutobook.def").write_text("LIBRARY libplutobook-0.dll\nEXPORTS\n" + "\n".join(symbols))
    unpack(archives["curl"], deps / "curl-src")
    sdk_task("build-deps.cmd")
    fonts = deps / "share/fonts/dejavu"
    fonts.mkdir(parents=True, exist_ok=True)
    with tarfile.open(archives["fonts"]) as archive:
        for face in ("DejaVuSans.ttf", "DejaVuSans-Bold.ttf"):
            entry = next(e for e in archive if e.name.endswith("/ttf/" + face))
            (fonts / face).write_bytes(archive.extractfile(entry).read())
    config = deps / "etc/fonts/fonts.conf"
    config.parent.mkdir(parents=True, exist_ok=True)
    tree = ET.Element("fontconfig")
    ET.SubElement(tree, "dir").text = fonts.as_posix()
    ET.SubElement(tree, "cachedir").text = (deps / "font-cache").as_posix()
    ET.ElementTree(tree).write(config, encoding="utf-8")
    (deps / ".built").touch()


def stage_renderer(run, cache, deps):
    # SSH uploads on cache misses. CI receives the matching Linux build artifact directly.
    if "PLUTOBOOK_WINDOWS_RENDERER" in os.environ:
        archive = Path(os.environ["PLUTOBOOK_WINDOWS_RENDERER"]).resolve(strict=True)
        checksum = digest(archive)
    else:
        checksum = (run / "renderer.sha256").read_text().strip()
        if not re.fullmatch("[a-f0-9]{64}", checksum):
            sys.exit("Invalid staged renderer SHA-256")
        archive = cache / f"renderer-{checksum}.zip"
        upload = run / "renderer.zip"
        if digest(upload if upload.exists() else archive) != checksum:
            sys.exit("Renderer archive checksum mismatch")
        if upload.exists():
            upload.replace(archive)
    directory = run / "renderer-runtime"
    directory.mkdir(exist_ok=True)
    with zipfile.ZipFile(archive) as bundle:
        header = bundle.read("include/plutobook.h").decode("utf-8").splitlines()
        if header != (deps / "include/plutobook.h").read_text(encoding="utf-8").splitlines():
            sys.exit("Patched renderer's C header differs from the pinned import library header")
        for patch in ("plutobook-0.20.0-static-font-variations.patch", "fontconfig-2.18.3-quiet-cache-dirs.patch"):
            if bundle.read("patches/" + patch) != (run / "tools/patches" / patch).read_bytes():
                sys.exit(f"Renderer was built with a different {patch}")
        with bundle.open("bin/libplutobook-0.dll") as src, (directory / "libplutobook-0.dll").open("wb") as dest:
            shutil.copyfileobj(src, dest)
    print(f"Using source-built Windows renderer {checksum}")
    return archive, directory


def main(version, mode, *args):
    import msvcrt
    from runtime_crt import stage_private_crt

    run = Path.cwd()
    root = Path(os.environ.get("PLUTOBOOK_WINDOWS_ROOT", run.parent.parent)).resolve()
    cache = root / "cache"
    cache.mkdir(parents=True, exist_ok=True)
    release = None
    thread = "nts"
    compiler = "vs17"
    if mode == "release":
        if len(args) != 2 or not re.fullmatch(r"[0-9]+\.[0-9]+\.[0-9]+", args[0]) or args[1] not in {"ts", "nts"}:
            sys.exit("Usage: runner.py PHP_VERSION release RELEASE_VERSION nts|ts")
        release, thread = args
        compiler = compiler_for(version)
    files = inputs(version, mode)
    private_tools = root / "msvc" / compiler / "VC/Tools/MSVC"
    versions = sorted((p for p in private_tools.glob("*") if re.fullmatch(r"\d+(\.\d+)+", p.name)
                       and (p / "bin/Hostx64/x64/cl.exe").is_file()),
                      key=lambda p: tuple(map(int, p.name.split("."))))
    private = versions[-1] if versions else None
    deps_key = hashlib.sha256((compiler + (private.name if private else "") + digest(run / "scripts/windows/build-deps.cmd") +
                              "".join(pin for key, (_, pin) in files.items() if key != "php")).encode()).hexdigest()[:12]
    deps = root / "deps" / deps_key
    recipe = digest(run / "scripts/windows/build-php.cmd")[:8]
    source = root / "php" / f"{version}-{thread}-{recipe}-{files['php'][1][:8]}-{deps_key}"
    build_dir = "Release_TS" if thread == "ts" else "Release"
    php = source / "x64" / build_dir / "php.exe"
    openssl = ["-d", f"extension={php.parent / 'php_openssl.dll'}"]
    sdk = root / "sdk" / files["sdk"][1][:8]
    env = {**os.environ, "PHP_SOURCE": str(source), "RUN": str(run), "DEPS": str(deps),
           "PLUTOBOOK_LINKAGE": "static" if release else "shared",
           "PHP_BUILD_DIR": build_dir,
           "PHP_THREAD_OPTION": "--enable-zts" if thread == "ts" else "--disable-zts",
           "PATH": str(deps / "bin") + os.pathsep + os.environ["PATH"],
           "FONTCONFIG_FILE": str(deps / "etc/fonts/fonts.conf")}

    def sdk_task(script):
        # VS 2022 can host the VS 2019 v142 toolset side by side.
        toolset = ["-s", "14.29"] if compiler == "vs16" and not private else []
        sdk_shell = "vs17" if private else compiler
        task = run / "scripts/windows" / script
        task_env = env
        if private:
            task_env = {**env, "PLUTOBOOK_MSVC": str(private), "PLUTOBOOK_COMPILER": compiler,
                        "PLUTOBOOK_TASK": str(task)}
            task = run / "scripts/windows/build-private.cmd"
        subprocess.run([sdk / f"phpsdk-{sdk_shell}-x64.bat", *toolset, "-t", task], env=task_env, check=True)

    # Closing this handle also releases the build lock after a failed or interrupted build.
    with (cache / "build.lock").open("a+b") as lock:
        print("Preparing cached Windows toolchain...")
        while True:
            try:
                msvcrt.locking(lock.fileno(), msvcrt.LK_NBLCK, 1)
                break
            except PermissionError:
                time.sleep(2)
        archives = {}
        for key, (name, checksum) in files.items():
            archive = run / name if (run / name).exists() else cache / name
            if digest(archive) != checksum:
                sys.exit(f"Checksum mismatch: {archive}")
            if archive.parent == run:
                archive.replace(cache / name)
            archives[key] = cache / name
        unpack(archives["sdk"], sdk)
        prepare_deps(archives, deps, sdk_task)
        archives["patched_renderer"], renderer = stage_renderer(run, cache, deps)
        unpack(archives["php"], source)
        built = (source / ".built").exists() and php.is_file()
        if not built:
            sdk_task("build-php.cmd")
        stage_private_crt(private, php.parent)
        if not built:
            subprocess.run([php, "-n", *openssl, "-r",
                            "exit(extension_loaded('openssl') && extension_loaded('zlib') ? 0 : 1);"],
                           check=True, env=env)
            (source / ".built").touch()

    probe = subprocess.check_output([php, "-n", run / "tests/toolchain/runtime.php"], env=env, text=True)
    print(probe.strip())
    info = json.loads(probe)
    if (info["version"] != version or info["sapi"] != "cli" or info["zts"] != (thread == "ts") or info["debug"]
            or info["ini"] or info["scanned"] or info["architecture"].lower() not in ("amd64", "x86_64")):
        sys.exit("Unexpected Windows PHP runtime")
    build_info = subprocess.check_output([php, "-n", "-i"], env=env, text=True, errors="replace")
    if not re.search(rf"^PHP Extension Build => API\d+,{thread.upper()},{compiler.upper()}\s*$", build_info, re.M):
        sys.exit(f"PHP was not built with the expected {thread} {compiler} ABI")
    env["PHP_DEVEL"] = str(next(php.parent.glob("php-*-devel-*/phpize.bat")).parent)
    out = run / "results"
    out.mkdir(exist_ok=True)
    # Keep the extension beside the verified renderer so Windows loads this exact DLL.
    env["PATH"] = str(renderer) + os.pathsep + env["PATH"]
    module = renderer / "php_plutobook.dll"
    with tempfile.TemporaryDirectory(prefix="ext-", dir=root) as directory:
        ext = Path(directory)
        shutil.copytree(run / "ext", ext, dirs_exist_ok=True, ignore=shutil.ignore_patterns("x64"))
        if release:
            (ext / "release-version.h").write_text(f'#define PHP_PLUTOBOOK_VERSION "{release}"\n', encoding="ascii")
        env["EXT_SOURCE"] = str(ext)
        sdk_task("build-extension.cmd")
        if release:
            linkage = (ext / "linkage.txt").read_text(errors="replace")
            imports = {name.lower() for name in re.findall(r"^\s*([\w.-]+\.dll)\s*$", linkage, re.I | re.M)}
            core = "php8ts.dll" if thread == "ts" else "php8.dll"
            if not {core, "libplutobook-0.dll"} <= imports or any(
                    name.startswith(("libcurl", "libssl", "libcrypto", "zlib")) for name in imports):
                sys.exit(f"Unexpected extension DLL imports: {sorted(imports)}")
            print(f"Verified extension DLL imports: {', '.join(sorted(imports))}")
        shutil.copyfile(ext / "x64" / build_dir / module.name, module)
    shutil.copyfile(module, out / module.name)
    cli = [php, "-n", *openssl, "-d", f"extension={module}"]
    subprocess.run(cli + ["-r", "exit(extension_loaded('plutobook') ? 0 : 1);"], env=env, check=True)
    if mode == "php":
        return subprocess.call([*cli, *args], env=env)
    if mode in ("suite", "release"):
        with (out / "invoice.html").open("wb") as html:
            subprocess.run(cli + ["tests/fixtures/invoice/generate.php"], stdout=html, env=env, check=True)
        subprocess.run(cli + ["tests/integration/invoice.php", out / "invoice.html", out], env=env, check=True)
        started = time.monotonic()
        with (out / "memory.json").open("wb") as memory:
            interval = ["100"] if release else []
            subprocess.run([*cli, "tests/integration/memory.php", out / "invoice.html", *interval],
                           stdout=memory, env=env, check=True)
        print(f"Verified invoice memory run in {time.monotonic() - started:.1f} s")
        args = ["tests/unit"]
    status = subprocess.call(
        [php, "-n", source / "run-tests.php", "-n", "-p", php, *openssl, "-d", f"extension={module}",
         "--show-diff", *args],
        env={**env, "NO_INTERACTION": "1", "REPORT_EXIT_STATUS": "1"},
    )
    if status == 0 and mode == "suite":
        from package_build import package_build
        package_build(out, php, module, deps, source, archives)
    if status == 0 and release:
        from package_build import package_release
        package_release(out, php, module, deps, archives, release, version, thread, compiler)
    return status


if __name__ == "__main__":
    if sys.argv[1] == "--inputs":
        print("\n".join(name for name, checksum in inputs(*sys.argv[2:]).values()))
    else:
        try:
            command = sys.argv[2:] or json.loads(Path("args.json").read_text())
            sys.exit(main(sys.argv[1], *command))
        finally:
            with zipfile.ZipFile("results.zip", "w", zipfile.ZIP_DEFLATED) as bundle:
                for file in Path("results").glob("*"):
                    bundle.write(file, file.name)
                for file in Path("tests").rglob("*.diff"):
                    bundle.write(file)
