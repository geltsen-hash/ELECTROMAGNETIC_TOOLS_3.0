import argparse
import copy
import ftplib
import json
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import zipfile
from unittest.mock import patch

from publish_release import ROOT, APP, digest, upload_release
from publish_python_runtime import package_runtime


def main():
    parser = argparse.ArgumentParser(description="Separate Python Runtime installer regression checks, no external FTP")
    parser.add_argument("--config", choices=("Release", "Debug"), default="Release")
    parser.add_argument("--real-package", type=Path, help="Дополнительно установить реальный пакет в удаляемую тестовую папку и проверить библиотеки")
    args = parser.parse_args()
    agent = ROOT / "x64" / args.config / "EmUpdate.exe"
    checks = 0
    stages = []
    with tempfile.TemporaryDirectory(prefix="em-py-tests-") as folder:
        root = Path(folder)
        source = root / "runtime-source"
        source.mkdir()
        fake_source = root / "FakePython.cs"
        fake_source.write_text('''using System;
using System.IO;
using System.Reflection;
using System.Threading;
class FakePython {
    static int Main(string[] args) {
        if (args.Length > 0 && args[0] == "--hold") { Thread.Sleep(60000); return 0; }
        string root = Path.GetDirectoryName(Assembly.GetExecutingAssembly().Location);
        if (File.Exists(Path.Combine(root,"after-move")) && Path.GetFileName(root)=="python_runtime") return 7;
        Console.WriteLine("{\\"python\\":\\"3.11.9\\",\\"bits\\":64,\\"portable\\":true,\\"numpy\\":\\"1.0\\",\\"scipy\\":\\"1.0\\",\\"pandas\\":\\"1.0\\",\\"matplotlib\\":\\"1.0\\",\\"tcl\\":\\"8.6.14\\"}");
        return 0;
    }
}
''', encoding="utf-8")
        compiler = Path(os.environ["SystemRoot"]) / "Microsoft.NET" / "Framework64" / "v4.0.30319" / "csc.exe"
        result = subprocess.run([str(compiler), "/nologo", "/platform:x64", "/out:" + str(source / "python.exe"), str(fake_source)], stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=30)
        assert result.returncode == 0, result.stdout.decode(errors="replace")
        (source / "python311._pth").write_text(".\nimport site\n")
        (source / "Lib").mkdir()
        (source / "Lib" / "data.txt").write_text("runtime data")
        (source / "empty.txt").write_bytes(b"")
        (source / "tests").mkdir()
        (source / "tests" / "unneeded.py").write_text("not packaged")
        manifest = package_runtime(source, root / "package", 1, "Runtime test", agent)
        package = root / "package"
        assert not any(f["path"].startswith("tests/") for f in manifest["files"])
        checks += 1

        def invoke(*options, success=True):
            result = subprocess.run([str(agent), *map(str, options)], stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=120)
            output = result.stdout.decode("utf-8-sig", errors="replace").strip()
            assert (result.returncode == 0) == success, output
            return output

        def target(name, old=False):
            folder = root / name
            folder.mkdir()
            (folder / APP).write_bytes(b"application unchanged")
            (folder / "release.json").write_bytes(b'{"build":27}')
            (folder / "station_config.json").write_bytes(b"operator settings")
            (folder / "metrology.bin").write_bytes(b"metrology")
            if old:
                (folder / "python_runtime").mkdir()
                (folder / "python_runtime" / "old.txt").write_bytes(b"previous environment")
            return folder

        def unchanged(folder):
            assert (folder / APP).read_bytes() == b"application unchanged"
            assert (folder / "release.json").read_bytes() == b'{"build":27}'
            assert (folder / "station_config.json").read_bytes() == b"operator settings"
            assert (folder / "metrology.bin").read_bytes() == b"metrology"

        def prepare(folder, m=None, z=None, success=True):
            output = invoke("--runtime-prepare", m or package / "python-runtime.json", z or package / manifest["package"], folder, success=success)
            if success:
                path = Path(output.splitlines()[-1])
                stages.append(path)
                return path

        try:
            clean = target("new installation")
            stage = prepare(clean)
            assert (stage / "Lib" / "data.txt").read_text() == "runtime data"
            checks += 1
            invoke("--runtime-install-local", package / "python-runtime.json", package / manifest["package"], clean)
            assert json.loads((clean / "python_runtime" / "runtime-release.json").read_text())["build"] == 1
            unchanged(clean)
            prepare(clean, success=False)
            checks += 2
            invoke("--prepare", package / "python-runtime.json", package / manifest["package"], clean, success=False)
            checks += 1

            old = target("existing runtime", old=True)
            invoke("--runtime-install-local", package / "python-runtime.json", package / manifest["package"], old)
            backup = next((old / ".em-update-backup").iterdir())
            assert (backup / "python_runtime" / "old.txt").read_bytes() == b"previous environment"
            assert not (old / "python_runtime" / "old.txt").exists()
            unchanged(old)
            checks += 1

            blocked = target("validation", old=True)
            variants = []
            for field, value in [("component", "application"), ("product", "SONDE_TEST"), ("platform", "x86"), ("sha256", "0" * 64),
                                 ("package", "../python-runtime.zip"), ("version", "bad"), ("authentication", "signed"), ("packages", {})]:
                changed = copy.deepcopy(manifest)
                changed[field] = value
                variants.append(changed)
            for bad in ("../app.exe", "..\\app.exe", "C:/app.exe", "dir/file:stream", "CON.txt", "a./file.py", "manifest.json", "RUNTIME-RELEASE.JSON"):
                changed = copy.deepcopy(manifest)
                changed["files"][0]["path"] = bad
                variants.append(changed)
            changed = copy.deepcopy(manifest)
            changed["files"].append(changed["files"][0])
            variants.append(changed)
            for i, changed in enumerate(variants):
                path = root / f"invalid-{i}.json"
                path.write_text(json.dumps(changed), encoding="utf-8")
                prepare(blocked, path, success=False)
                unchanged(blocked)
                assert (blocked / "python_runtime" / "old.txt").exists()
                checks += 1

            for i, kind in enumerate(("extra", "traversal", "duplicate", "symlink", "corrupt")):
                changed = copy.deepcopy(manifest)
                archive_path = root / f"bad-archive-{i}.zip"
                with zipfile.ZipFile(archive_path, "w") as archive:
                    for file in manifest["files"]:
                        data = (source / file["path"]).read_bytes()
                        if kind == "corrupt" and file["path"] == "python.exe": data = b"x" + data[1:]
                        if kind == "symlink" and file["path"] == "python.exe":
                            entry = zipfile.ZipInfo(file["path"])
                            entry.external_attr = 0o120777 << 16
                            archive.writestr(entry, data)
                        else: archive.writestr(file["path"], data)
                    if kind == "extra": archive.writestr("not-in-manifest.py", b"bad")
                    if kind == "traversal": archive.writestr("../outside.py", b"bad")
                    if kind == "duplicate":
                        with patch("warnings.warn"): archive.writestr("python.exe", (source / "python.exe").read_bytes())
                changed["sha256"] = digest(archive_path)
                changed["size"] = archive_path.stat().st_size
                path = root / f"bad-archive-{i}.json"
                path.write_text(json.dumps(changed), encoding="utf-8")
                prepare(blocked, path, archive_path, success=False)
                unchanged(blocked)
                checks += 1

            changed = copy.deepcopy(manifest)
            changed["version"] = "3.12.0"
            path = root / "wrong-python-version.json"
            path.write_text(json.dumps(changed), encoding="utf-8")
            invoke("--runtime-install-local", path, package / manifest["package"], blocked, success=False)
            assert (blocked / "python_runtime" / "old.txt").exists()
            unchanged(blocked)
            checks += 1

            (source / "after-move").write_bytes(b"fail only after folder swap")
            bad = package_runtime(source, root / "after-move-package", 2, "Relocation failure", agent)
            for i, had_old in enumerate((False, True)):
                folder = target(f"relocation-rollback-{i}", old=had_old)
                invoke("--runtime-install-local", root / "after-move-package" / "python-runtime.json", root / "after-move-package" / bad["package"], folder, success=False)
                assert (folder / "python_runtime").exists() == had_old
                if had_old: assert (folder / "python_runtime" / "old.txt").read_bytes() == b"previous environment"
                unchanged(folder)
                journal = next((folder / ".em-update-backup").glob("*/transaction.json"))
                assert json.loads(journal.read_text())["status"] == "rolled_back"
                checks += 1

            hold = target("running python")
            shutil.copytree(source, hold / "python_runtime")
            process = subprocess.Popen([str(hold / "python_runtime" / "python.exe"), "--hold"])
            try:
                invoke("--runtime-install-local", package / "python-runtime.json", package / manifest["package"], hold, success=False)
                assert not (hold / ".em-update-backup").exists()
            finally:
                process.terminate()
                process.wait(timeout=10)
            checks += 1

            recovering = target("recovery", old=True)
            backup = recovering / ".em-update-backup" / "interrupted"
            backup.mkdir(parents=True)
            shutil.move(str(recovering / "python_runtime"), str(backup / "python_runtime"))
            shutil.copytree(source, recovering / "python_runtime")
            incoming = recovering / ".py-new-interrupted"
            incoming.mkdir()
            journal = {"component": "python-runtime", "target": str(recovering), "incoming": str(incoming), "had_old": True, "status": "applying"}
            (backup / "transaction.json").write_text(json.dumps(journal), encoding="utf-8")
            prepare(recovering, success=False)
            invoke("--recover", backup)
            assert (recovering / "python_runtime" / "old.txt").read_bytes() == b"previous environment"
            unchanged(recovering)
            checks += 1

            events = []
            class FakeFTP:
                def __enter__(self): return self
                def __exit__(self, *args): pass
                def connect(self, *args, **kwargs): pass
                def login(self, *args): pass
                def set_pasv(self, *args): pass
                def cwd(self, *args): pass
                def retrbinary(self, command, callback):
                    events.append(command)
                    raise ftplib.error_perm("550 absent")
                def storbinary(self, command, stream): assert stream.read(); events.append(command)
                def rename(self, source, destination): events.append("RENAME " + destination)
            with patch.dict(os.environ, {"EM_FTP_USER": "test", "EM_FTP_PASSWORD": "test"}):
                upload_release(package, manifest, "ftp://127.0.0.1/test/", FakeFTP, manifest_name="python-runtime.json")
            assert events[0] == "RETR python-runtime.json"
            assert events[-1] == "RENAME python-runtime.json"
            assert "RENAME manifest.json" not in events
            checks += 1
            events.clear()
            try:
                with patch.dict(os.environ, {"EM_FTP_USER": "test", "EM_FTP_PASSWORD": "test"}):
                    upload_release(package, manifest, "ftp://127.0.0.1/test/", FakeFTP)
                raise AssertionError("Runtime was allowed to overwrite the application channel")
            except ValueError:
                assert not events
                checks += 1
            if args.real_package:
                real_manifest_path = args.real_package.resolve() / "python-runtime.json"
                real_manifest = json.loads(real_manifest_path.read_text(encoding="utf-8"))
                real = target("real")
                invoke("--runtime-install-local", real_manifest_path, real_manifest_path.parent / real_manifest["package"], real)
                probe = json.loads(invoke("--python-probe", real / "python_runtime"))
                assert probe["python"] == real_manifest["version"] and probe["portable"] is True
                assert all(str(probe[name]) == version for name, version in real_manifest["packages"].items())
                unchanged(real)
                print(f"[REAL PYTHON OK] Python {probe['python']}, NumPy/SciPy/pandas/Matplotlib/Tkinter, isolated temporary installation")
                checks += 1
        finally:
            for stage in stages:
                if stage.exists(): shutil.rmtree(stage)
    print(f"[PYTHON RUNTIME TEST SUCCESS] {args.config}: {checks} checks, no external FTP")


if __name__ == "__main__":
    main()
