import argparse
from datetime import datetime, timezone
import ftplib
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys
from urllib.parse import unquote, urlparse
import uuid
import zipfile

ROOT = Path(__file__).resolve().parent
PRODUCT = "ELECTROMAGNETIC_TOOLS_3.0"
APP = PRODUCT + ".exe"
AGENT = "EmUpdate.exe"
DEFAULT_FTP = "ftp://89.189.156.184/VORONEJ_FTP/UPDATE/ELECTROMAGNETIC_TOOLS_3.0/"
ALLOWED = (APP, AGENT, APP + ".config", AGENT + ".config")


def digest(path):
    sha = hashlib.sha256()
    with Path(path).open("rb") as stream:
        for chunk in iter(lambda: stream.read(65536), b""):
            sha.update(chunk)
    return sha.hexdigest()


def package_release(source, output, build, notes):
    source, output = Path(source), Path(output)
    if build <= 0 or build > 9223372036854775807 or len(notes) > 16000:
        raise ValueError("Invalid build number or release notes")
    for name in (APP, AGENT):
        if not (source / name).is_file():
            raise ValueError(f"Required file not found: {name}")
    version = subprocess.check_output([str(source / AGENT), "--info", str(source / APP)], timeout=30).decode("utf-8-sig").strip()
    if not output.parent.is_dir():
        raise ValueError("Output parent directory must exist")
    output.mkdir(exist_ok=False)
    package = output / f"release-{build}-{uuid.uuid4().hex[:12]}.zip"
    files = []

    def add_payload(archive, name, data):
        archive.writestr(name, data)
        files.append({"path": name, "size": len(data), "sha256": hashlib.sha256(data).hexdigest()})

    with zipfile.ZipFile(package, "x", zipfile.ZIP_DEFLATED) as archive:
        for name in ALLOWED:
            path = source / name
            if path.is_file():
                if path.stat().st_size > 268435456:
                    raise ValueError(f"Payload file is too large: {name}")
                add_payload(archive, name, path.read_bytes())
        scripts_dir = source / "scripts"
        if scripts_dir.is_dir():
            for path in sorted(scripts_dir.rglob("*")):
                if "__pycache__" in path.parts:
                    continue
                name = "scripts/" + path.relative_to(scripts_dir).as_posix()
                if not (path.is_file() and not path.is_symlink() and path.suffix == ".py"):
                    raise ValueError(f"В scripts допускаются только файлы *.py без ссылок: {name}")
                if path.stat().st_size > 268435456:
                    raise ValueError(f"Payload file is too large: {name}")
                add_payload(archive, name, path.read_bytes())
    if package.stat().st_size > 268435456 or sum(file["size"] for file in files) > 536870912:
        raise ValueError("Package exceeds updater size limits")
    manifest = {
        "schema": 1, "product": PRODUCT, "platform": "x64", "authentication": "none",
        "build": build, "version": version,
        "released_utc": datetime.now(timezone.utc).isoformat(), "notes": notes,
        "package": package.name, "size": package.stat().st_size, "sha256": digest(package),
        "files": files,
        "organization": "НПФ «АМК Горизонт»",
        "author": "Разработал зам начальника ОП Смирнов С. Г.",
    }
    (output / "manifest.json").write_text(json.dumps(manifest, ensure_ascii=False, indent=2), encoding="utf-8")
    return manifest


def upload_release(output, manifest, uri, ftp_factory=ftplib.FTP, manifest_name="manifest.json"):
    if manifest_name not in ("manifest.json", "python-runtime.json"):
        raise ValueError("Unsupported publication channel")
    component = "python-runtime" if manifest_name == "python-runtime.json" else "application"
    if manifest.get("product") != PRODUCT or manifest.get("component", "application") != component:
        raise ValueError("Manifest component does not match the publication channel")
    parsed = urlparse(uri)
    if parsed.scheme != "ftp" or not parsed.hostname or parsed.username or parsed.password or parsed.query or parsed.fragment:
        raise ValueError("Use an FTP directory URI without credentials")
    username = os.environ.get("EM_FTP_USER")
    password = os.environ.get("EM_FTP_PASSWORD")
    if not username or not password:
        raise ValueError("Set EM_FTP_USER and EM_FTP_PASSWORD locally before publishing")
    temporary = ".upload-" + uuid.uuid4().hex
    with ftp_factory() as ftp:
        ftp.connect(parsed.hostname, parsed.port or 21, timeout=20)
        ftp.login(username, password)
        ftp.set_pasv(True)
        ftp.cwd(unquote(parsed.path))
        current = bytearray()
        def receive(chunk):
            current.extend(chunk)
            if len(current) > (16777216 if manifest_name == "python-runtime.json" else 1048576):
                raise ValueError("Remote manifest is too large")
        try:
            ftp.retrbinary("RETR " + manifest_name, receive)
        except ftplib.error_perm as error:
            if not str(error).startswith("550"):
                raise
        if current:
            previous = json.loads(current.decode("utf-8-sig"))
            if (previous.get("product") != manifest["product"]
                    or previous.get("component", "application") != manifest.get("component", "application")
                    or not isinstance(previous.get("build"), int) or previous["build"] >= manifest["build"]):
                raise ValueError("Remote product/build does not allow this publication")
        with (Path(output) / manifest["package"]).open("rb") as stream:
            ftp.storbinary("STOR " + temporary + ".zip", stream)
        ftp.rename(temporary + ".zip", manifest["package"])
        with (Path(output) / manifest_name).open("rb") as stream:
            ftp.storbinary("STOR " + temporary + ".json", stream)
        ftp.rename(temporary + ".json", manifest_name)


def main():
    parser = argparse.ArgumentParser(description="Подготовка релиза. Организация: НПФ «АМК Горизонт», разработал зам начальника ОП Смирнов С. Г.")
    parser.add_argument("--build", type=int, required=True, help="Монотонно возрастающий номер сборки")
    parser.add_argument("--output", type=Path, required=True, help="Новая папка локального релиза")
    parser.add_argument("--desc", required=True, help="Описание изменений")
    parser.add_argument("--source", type=Path, default=ROOT / "x64" / "Release")
    parser.add_argument("--skip-checks", action="store_true", help="Использовать ранее проверенную сборку")
    group = parser.add_mutually_exclusive_group()
    group.add_argument("--publish", action="store_true", help="Явно разрешить публикацию на FTP")
    group.add_argument("--no-upload", action="store_true", help="Только локальный пакет (это поведение по умолчанию)")
    parser.add_argument("--ftp-uri", default=DEFAULT_FTP)
    args = parser.parse_args()
    try:
        if not args.skip_checks:
            if args.source.resolve() != (ROOT / "x64" / "Release").resolve():
                raise ValueError("A custom --source requires explicit --skip-checks after verifying that build")
            subprocess.run([sys.executable, str(ROOT / "verify.py"), "--config", "Release", "--csv-check", "--update-check", "--python-check"], cwd=ROOT, check=True)
        manifest = package_release(args.source.resolve(), args.output.resolve(), args.build, args.desc)
        print(f"[PACKAGE SUCCESS] build={manifest['build']}, version={manifest['version']}, files={len(manifest['files'])}")
        if args.publish:
            try:
                upload_release(args.output, manifest, args.ftp_uri)
            except Exception:
                print("[PUBLISH FAILED] Проверьте доступ, существование отдельного FTP-каталога и возможность переименования. Локальный пакет сохранен.", file=sys.stderr)
                return 1
            print("[PUBLISH SUCCESS] Пакет опубликован, manifest.json обновлен последним")
        else:
            print("[LOCAL ONLY] FTP не использовался")
        return 0
    except Exception as error:
        print(f"[RELEASE FAILED] {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    sys.exit(main())
