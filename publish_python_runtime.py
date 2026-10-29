import argparse
from datetime import datetime, timezone
import hashlib
import json
import os
import shutil
import tempfile
from pathlib import Path
import stat
import subprocess
import sys
import uuid
import zipfile

from publish_release import ROOT, PRODUCT, DEFAULT_FTP, digest, upload_release

PACKAGES = ("numpy", "scipy", "pandas", "matplotlib", "tcl")


def probe_runtime(agent, runtime):
    result = subprocess.run([str(agent), "--python-probe", str(runtime)], stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=90)
    if result.returncode:
        raise ValueError("Portable Python validation failed: " + result.stdout.decode("utf-8-sig", errors="replace")[-1400:])
    return json.loads(result.stdout.decode("utf-8-sig"))


def runtime_paths(runtime):
    runtime = Path(runtime)
    for root, directories, files in os.walk(runtime, followlinks=False):
        root = Path(root)
        for name in list(directories):
            path = root / name
            if path.is_symlink() or getattr(path.lstat(), "st_file_attributes", 0) & 0x400:
                raise ValueError("Links are not allowed in runtime: " + str(path.relative_to(runtime)))
        directories[:] = sorted(name for name in directories if name not in ("__pycache__", "tests", "test", ".git"))
        for name in sorted(files):
            path = root / name
            if path.is_symlink() or getattr(path.lstat(), "st_file_attributes", 0) & 0x400:
                raise ValueError("Links are not allowed in runtime: " + str(path.relative_to(runtime)))
            if name.endswith((".pyc", ".pyo", ".chm")) or name == "runtime-release.json":
                continue
            if not stat.S_ISREG(path.stat().st_mode):
                raise ValueError("Non-regular runtime file")
            yield path


def package_runtime(runtime, output, build, notes, agent):
    runtime, output = Path(runtime).resolve(), Path(output).resolve()
    if build <= 0 or build > 9223372036854775807 or len(notes) > 16000:
        raise ValueError("Invalid runtime build or notes")
    if runtime == output or runtime in output.parents:
        raise ValueError("Output must be outside the source runtime")
    if not output.parent.is_dir():
        raise ValueError("Output parent directory must exist")
    probe = probe_runtime(agent, runtime)
    if probe.get("bits") != 64 or probe.get("portable") is not True or not all(name in probe for name in PACKAGES):
        raise ValueError("Incomplete or non-portable Python environment")
    output.mkdir(exist_ok=False)
    package = output / f"python-runtime-{build}-{uuid.uuid4().hex[:12]}.zip"
    files = []
    total = 0
    with zipfile.ZipFile(package, "x", zipfile.ZIP_DEFLATED, compresslevel=6) as archive:
        for path in runtime_paths(runtime):
            relative = path.relative_to(runtime).as_posix()
            if path.stat().st_size > 268435456:
                raise ValueError("Runtime file exceeds size limit")
            checksum = hashlib.sha256()
            length = 0
            with path.open("rb") as source, archive.open(relative, "w") as destination:
                for chunk in iter(lambda: source.read(65536), b""):
                    length += len(chunk)
                    if length > 268435456:
                        raise ValueError("Runtime file exceeds size limit")
                    destination.write(chunk)
                    checksum.update(chunk)
            files.append({"path": relative, "size": length, "sha256": checksum.hexdigest()})
            total += length
            if len(files) > 20000 or total > 1073741824:
                raise ValueError("Runtime exceeds file count or expanded-size limit")
    if package.stat().st_size > 268435456:
        raise ValueError("Runtime ZIP exceeds 256 MiB")
    manifest = {
        "schema": 1, "product": PRODUCT, "platform": "x64", "authentication": "none", "component": "python-runtime",
        "build": build, "version": probe["python"], "released_utc": datetime.now(timezone.utc).isoformat(),
        "notes": notes, "package": package.name, "size": package.stat().st_size, "sha256": digest(package),
        "files": files, "packages": {name: str(probe[name]) for name in PACKAGES},
        "organization": "НПФ «АМК Горизонт»", "author": "Разработал зам начальника ОП Смирнов С. Г.",
    }
    content = json.dumps(manifest, ensure_ascii=False, indent=2)
    if len(content.encode("utf-8")) > 16777216:
        raise ValueError("Runtime manifest exceeds 16 MiB")
    manifest_path = output / "python-runtime.json"
    manifest_path.write_text(content, encoding="utf-8")
    with tempfile.TemporaryDirectory(prefix="em-runtime-package-check-") as target:
        result = subprocess.run([str(agent), "--runtime-prepare", str(manifest_path), str(package), target], stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=240)
        text = result.stdout.decode("utf-8-sig", errors="replace").strip()
        if result.returncode:
            raise ValueError("Runtime package rejected by the client: " + text[-1400:])
        stage = Path(text.splitlines()[-1])
        if stage.parent.resolve() != Path(tempfile.gettempdir()).resolve() or not stage.name.startswith("EmToolsPython-"):
            raise ValueError("Unexpected runtime validation directory")
        shutil.rmtree(stage)
    return manifest


def main():
    parser = argparse.ArgumentParser(description="Отдельная поставка portable Python. Организация: НПФ «АМК Горизонт», разработал зам начальника ОП Смирнов С. Г.")
    parser.add_argument("--runtime-dir", type=Path, required=True, help="Готовая portable-среда с Python/NumPy/SciPy/pandas/Matplotlib/Tkinter")
    parser.add_argument("--build", type=int, required=True)
    parser.add_argument("--output", type=Path, required=True, help="Новая папка пакета вне исходной среды")
    parser.add_argument("--desc", required=True)
    parser.add_argument("--agent", type=Path, default=ROOT / "x64" / "Release" / "EmUpdate.exe")
    parser.add_argument("--ftp-uri", default=DEFAULT_FTP)
    group = parser.add_mutually_exclusive_group()
    group.add_argument("--publish", action="store_true")
    group.add_argument("--no-upload", action="store_true")
    args = parser.parse_args()
    try:
        manifest = package_runtime(args.runtime_dir, args.output, args.build, args.desc, args.agent.resolve())
        print(f"[RUNTIME PACKAGE SUCCESS] Python {manifest['version']}, build={manifest['build']}, files={len(manifest['files'])}, zip={manifest['size']} bytes")
        if args.publish:
            try:
                upload_release(args.output, manifest, args.ftp_uri, manifest_name="python-runtime.json")
            except Exception:
                print("[RUNTIME PUBLISH FAILED] Проверьте доступ/канал FTP; локальный пакет сохранен", file=sys.stderr)
                return 1
            print("[RUNTIME PUBLISH SUCCESS] python-runtime.json опубликован последним; manifest.json программы не изменялся")
        else:
            print("[LOCAL ONLY] FTP не использовался; исходная среда не изменялась")
        return 0
    except Exception as error:
        print("[RUNTIME FAILED] " + str(error), file=sys.stderr)
        return 1


if __name__ == "__main__":
    sys.exit(main())
