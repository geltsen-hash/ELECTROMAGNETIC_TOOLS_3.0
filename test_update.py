import argparse
import copy
import ctypes
import ftplib
import hashlib
import json
import os
from pathlib import Path
import shutil
import socket
import socketserver
import subprocess
import threading
import time
import tempfile
import zipfile
from unittest.mock import patch

from publish_release import APP, AGENT, ROOT, package_release, upload_release, digest


def main():
    parser = argparse.ArgumentParser(description="Local updater regression tests; no external FTP access")
    parser.add_argument("--config", choices=("Release", "Debug"), default="Release")
    args = parser.parse_args()
    binaries = ROOT / "x64" / args.config
    agent = binaries / AGENT
    checks = 0
    stages = []
    with tempfile.TemporaryDirectory(prefix="em-update-tests-") as directory:
        root = Path(directory)
        source = root / "new"
        source.mkdir()
        for name in (APP, AGENT):
            shutil.copyfile(binaries / name, source / name)
        (source / "scripts").mkdir()
        (source / "scripts" / "check_script.py").write_bytes(b"# test analysis script")
        with (source / APP).open("ab") as stream:
            stream.write(b"EM-UPDATE-TEST-TRAILER")
        manifest = package_release(source, root / "release", 2, "Тест обновления; без FTP")
        assert any(f["path"] == "scripts/check_script.py" for f in manifest["files"])
        release = root / "release"

        def invoke(*options, success=True):
            result = subprocess.run([str(agent), *map(str, options)], stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=90)
            text = result.stdout.decode("utf-8-sig", errors="replace").strip()
            assert (result.returncode == 0) == success, text
            return text

        def target(name, state=True, app=True):
            path = root / name
            path.mkdir()
            for item in (APP, AGENT):
                if item != APP or app:
                    shutil.copyfile(binaries / item, path / item)
            for name, content in (("station_config.json", b"operator settings"), ("metrology.bin", b"calibration"), ("data.csv", b"user data")):
                (path / name).write_bytes(content)
            if state:
                previous = copy.deepcopy(manifest)
                previous["build"] = 1
                (path / "release.json").write_text(json.dumps(previous), encoding="utf-8")
            return path

        def user_files_unchanged(path):
            assert (path / "station_config.json").read_bytes() == b"operator settings"
            assert (path / "metrology.bin").read_bytes() == b"calibration"
            assert (path / "data.csv").read_bytes() == b"user data"

        def prepare(path, manifest_path=None, zip_path=None, success=True):
            text = invoke("--prepare", manifest_path or release / "manifest.json", zip_path or release / manifest["package"], path, success=success)
            if success:
                stage = Path(text.splitlines()[-1])
                stages.append(stage)
                return stage
            return None

        try:
            installation = target("portable папка")
            stage = prepare(installation)
            assert digest(stage / APP) == digest(source / APP)
            assert not (installation / ".em-update-backup").exists()
            checks += 1
            invoke("--install-local", release / "manifest.json", release / manifest["package"], installation)
            assert digest(installation / APP) == digest(source / APP)
            assert (installation / "scripts" / "check_script.py").read_bytes() == b"# test analysis script"
            assert json.loads((installation / "release.json").read_text())["build"] == 2
            user_files_unchanged(installation)
            backup = next((installation / ".em-update-backup").iterdir())
            assert digest(backup / APP) == digest(binaries / APP)
            assert json.loads((backup / "transaction.json").read_text())["status"] == "installed"
            checks += 1
            prepare(installation, success=False)
            checks += 1
            fresh = target("no-release-state", state=False)
            invoke("--install-local", release / "manifest.json", release / manifest["package"], fresh)
            assert json.loads((fresh / "release.json").read_text())["build"] == 2
            checks += 1

            original = target("validation")
            before = (original / APP).read_bytes()
            variants = []
            for field, value in [("product", "SONDE_TEST"), ("platform", "x86"), ("schema", 2), ("authentication", "signed"),
                                 ("sha256", "0" * 64), ("size", manifest["size"] + 1), ("build", 1), ("build", 0),
                                 ("version", "invalid"), ("package", "../release.zip"), ("package", "release-evil.zip?x=1")]:
                changed = copy.deepcopy(manifest)
                changed[field] = value
                variants.append(changed)
            for bad_name in ("../outside.exe", "..\\outside.exe", "C:\\outside.exe", "station_config.json", "metrology.bin", "EmUpdate.exe:stream", "emupdate.exe",
                             "scripts/evil.exe", "scripts/sub/x.py", "scripts/../x.py"):
                changed = copy.deepcopy(manifest)
                changed["files"][0]["path"] = bad_name
                variants.append(changed)
            changed = copy.deepcopy(manifest)
            changed["files"].append(changed["files"][0])
            variants.append(changed)
            changed = copy.deepcopy(manifest)
            changed["files"] = changed["files"][:1]
            variants.append(changed)
            for i, changed in enumerate(variants):
                path = root / f"bad-manifest-{i}.json"
                path.write_text(json.dumps(changed), encoding="utf-8")
                prepare(original, path, success=False)
                assert (original / APP).read_bytes() == before
                checks += 1

            for i, mode in enumerate(("traversal", "duplicate", "extra", "symlink", "corrupt-payload", "missing")):
                changed = copy.deepcopy(manifest)
                archive_path = root / f"bad-{i}.zip"
                with zipfile.ZipFile(archive_path, "w") as archive:
                    for entry in changed["files"]:
                        if mode == "missing" and entry["path"] == AGENT:
                            continue
                        name = entry["path"]
                        content = (source / name).read_bytes()
                        if mode == "corrupt-payload" and name == APP:
                            content = b"x" + content[1:]
                        if mode == "symlink" and name == APP:
                            info = zipfile.ZipInfo(name)
                            info.external_attr = (0o120777 << 16)
                            archive.writestr(info, content)
                        else:
                            archive.writestr(name, content)
                    if mode == "traversal":
                        archive.writestr("../outside.exe", b"bad")
                    if mode == "duplicate":
                        with patch("warnings.warn"):
                            archive.writestr(APP, (source / APP).read_bytes())
                    if mode == "extra":
                        archive.writestr("station_config.json", b"bad")
                changed["size"] = archive_path.stat().st_size
                changed["sha256"] = digest(archive_path)
                path = root / f"bad-zip-{i}.json"
                path.write_text(json.dumps(changed), encoding="utf-8")
                prepare(original, path, archive_path, success=False)
                assert (original / APP).read_bytes() == before
                checks += 1

            stage = prepare(original)
            with (stage / APP).open("ab") as stream:
                stream.write(b"changed after verification")
            job = root / "tampered-job.json"
            job.write_text(json.dumps({"stage": str(stage), "target": str(original), "restart": False}), encoding="utf-8")
            invoke("--apply-job", job, "--quiet", success=False)
            assert (original / APP).read_bytes() == before
            checks += 1

            kernel32 = ctypes.WinDLL("kernel32", use_last_error=True)
            kernel32.CreateFileW.argtypes = [ctypes.c_wchar_p, ctypes.c_uint32, ctypes.c_uint32, ctypes.c_void_p, ctypes.c_uint32, ctypes.c_uint32, ctypes.c_void_p]
            kernel32.CreateFileW.restype = ctypes.c_void_p
            kernel32.CloseHandle.argtypes = [ctypes.c_void_p]
            for n, has_app in enumerate((True, False)):
                locked = target(f"rollback-{n}", app=has_app)
                state_before = (locked / "release.json").read_bytes()
                handle = kernel32.CreateFileW(str(locked / AGENT), 0x80000000, 1, None, 3, 0, None)
                assert handle not in (None, ctypes.c_void_p(-1).value)
                try:
                    invoke("--install-local", release / "manifest.json", release / manifest["package"], locked, success=False)
                finally:
                    kernel32.CloseHandle(handle)
                assert (locked / "release.json").read_bytes() == state_before
                if has_app:
                    assert (locked / APP).read_bytes() == before
                else:
                    assert not (locked / APP).exists()
                user_files_unchanged(locked)
                journal = next((locked / ".em-update-backup").glob("*/transaction.json"))
                assert json.loads(journal.read_text())["status"] == "rolled_back"
                checks += 1

            waiting = target("wait-for-parent")
            stage = prepare(waiting)
            parent = subprocess.Popen([str(waiting / APP)], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
            kernel32.GetProcessTimes.argtypes = [ctypes.c_void_p, ctypes.POINTER(ctypes.c_uint64), ctypes.POINTER(ctypes.c_uint64), ctypes.POINTER(ctypes.c_uint64), ctypes.POINTER(ctypes.c_uint64)]
            created, exited, kernel, user = (ctypes.c_uint64() for _ in range(4))
            assert kernel32.GetProcessTimes(int(parent._handle), ctypes.byref(created), ctypes.byref(exited), ctypes.byref(kernel), ctypes.byref(user))
            job = root / "waiting-job.json"
            job.write_text(json.dumps({"stage": str(stage), "target": str(waiting), "parent_pid": parent.pid,
                                       "parent_start_ticks": created.value + 504911232000000000, "restart": False}), encoding="utf-8")
            worker = subprocess.Popen([str(agent), "--apply-job", str(job), "--quiet"], stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
            try:
                time.sleep(1)
                assert parent.poll() is None, "Test application exited unexpectedly"
                assert worker.poll() is None, "Worker did not wait for the application"
                assert (waiting / APP).read_bytes() == before
                parent.terminate()
                parent.wait(timeout=10)
                output = worker.communicate(timeout=30)[0]
                assert worker.returncode == 0, output.decode("utf-8-sig", errors="replace")
                assert digest(waiting / APP) == digest(source / APP)
            finally:
                if parent.poll() is None:
                    parent.terminate()
                    parent.wait(timeout=10)
                if worker.poll() is None:
                    worker.terminate()
                    worker.wait(timeout=10)
            checks += 1

            stage = prepare(original)
            job = root / "wrong-parent-job.json"
            job.write_text(json.dumps({"stage": str(stage), "target": str(original), "parent_pid": os.getpid(), "parent_start_ticks": 0, "restart": False}), encoding="utf-8")
            invoke("--apply-job", job, "--quiet", success=False)
            assert (original / APP).read_bytes() == before
            checks += 1

            recovering = target("recovery")
            backup = recovering / ".em-update-backup" / "interrupted"
            backup.mkdir(parents=True)
            shutil.copyfile(recovering / APP, backup / APP)
            journal = {"target": str(recovering), "status": "applying", "files": [{"path": APP, "existed": True, "sha256": digest(backup / APP)}], "applied": [], "pending": APP}
            (backup / "transaction.json").write_text(json.dumps(journal), encoding="utf-8")
            shutil.copyfile(source / APP, recovering / APP)
            prepare(recovering, success=False)
            checks += 1
            invoke("--recover", backup)
            assert (recovering / APP).read_bytes() == before
            checks += 1

            remote_files = {"manifest.json": (release / "manifest.json").read_bytes(), manifest["package"]: (release / manifest["package"]).read_bytes()}
            class DownloadFTP(socketserver.StreamRequestHandler):
                def handle(self):
                    passive = None
                    def reply(line):
                        self.wfile.write((line + "\r\n").encode("ascii"))
                        self.wfile.flush()
                    reply("220 local test FTP")
                    try:
                        while True:
                            line = self.rfile.readline().decode("utf-8").strip()
                            if not line:
                                break
                            command, _, argument = line.partition(" ")
                            if command == "USER": reply("331 Password required")
                            elif command == "PASS": reply("230 Logged in")
                            elif command == "PWD": reply('257 "/"')
                            elif command in ("TYPE", "OPTS", "CWD"): reply("200 OK")
                            elif command in ("PASV", "EPSV"):
                                if passive: passive.close()
                                passive = socket.socket()
                                passive.bind(("127.0.0.1", 0))
                                passive.listen(1)
                                passive.settimeout(10)
                                port = passive.getsockname()[1]
                                reply(f"227 Entering Passive Mode (127,0,0,1,{port // 256},{port % 256})" if command == "PASV" else f"229 Entering Extended Passive Mode (|||{port}|)")
                            elif command == "SIZE":
                                payload = remote_files.get(argument.rsplit("/", 1)[-1])
                                reply(f"213 {len(payload)}" if payload is not None else "550 Missing")
                            elif command == "RETR":
                                payload = remote_files.get(argument.rsplit("/", 1)[-1])
                                if payload is None:
                                    reply("550 Missing")
                                else:
                                    reply(f"150 Opening data connection ({len(payload)} bytes)")
                                    connection, _ = passive.accept()
                                    with connection:
                                        connection.sendall(payload)
                                    passive.close()
                                    passive = None
                                    reply("226 Transfer complete")
                            elif command == "QUIT":
                                reply("221 Bye")
                                break
                            else: reply("200 OK")
                    finally:
                        if passive: passive.close()
            class LocalFTP(socketserver.ThreadingTCPServer):
                daemon_threads = True
                allow_reuse_address = True
            with LocalFTP(("127.0.0.1", 0), DownloadFTP) as server:
                thread = threading.Thread(target=server.serve_forever, daemon=True)
                thread.start()
                settings = root / "local-ftp.json"
                settings.write_text(json.dumps({"base_uri": f"ftp://127.0.0.1:{server.server_address[1]}/test/", "username": "test", "protected_password": ""}), encoding="utf-8")
                try:
                    probe_source = root / "ProxyRegression.cs"
                    probe_source.write_text('''using System;
using System.Net;
using System.Reflection;
using EmTools.Updates;
sealed class ForcedProxy : IWebProxy {
    public ICredentials Credentials { get; set; }
    public bool IsBypassed(Uri destination) { return false; }
    public Uri GetProxy(Uri destination) { return new Uri("http://127.0.0.1:1/"); }
}
static class ProxyRegression {
    static int Main(string[] args) {
        WebRequest.DefaultWebProxy = new ForcedProxy();
        var settings = new UpdateSettings { base_uri = args[0], username = "test" };
        try {
            var request = (FtpWebRequest)typeof(UpdateController).GetMethod("CreateDownloadRequest", BindingFlags.NonPublic | BindingFlags.Static)
                .Invoke(null, new object[] { settings, "manifest.json" });
            var explicitProxy = typeof(FtpWebRequest).GetField("m_ProxyUserSet", BindingFlags.NonPublic | BindingFlags.Instance);
            if (request.Proxy != null || explicitProxy == null || !(bool)explicitProxy.GetValue(request)) {
                Console.Error.WriteLine("[PROXY REGRESSION FAILED] FTP still uses the default proxy");
                return 1;
            }
            typeof(UpdateController).GetMethod("Download", BindingFlags.NonPublic | BindingFlags.Static)
                .Invoke(null, new object[] { settings, "manifest.json", args[1], 1048576L, -1L });
            return 0;
        } catch (TargetInvocationException ex) {
            Console.Error.WriteLine("[PROXY REGRESSION FAILED] " + ex.InnerException.GetType().Name);
            return 1;
        }
    }
}
''', encoding="utf-8")
                    shutil.copyfile(agent, root / AGENT)
                    probe = root / "ProxyRegression.exe"
                    compiler = Path(os.environ["SystemRoot"]) / "Microsoft.NET" / "Framework64" / "v4.0.30319" / "csc.exe"
                    compiled = subprocess.run([str(compiler), "/nologo", "/target:exe", "/r:" + str(root / AGENT), "/out:" + str(probe), str(probe_source)], stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=30)
                    assert compiled.returncode == 0, compiled.stdout.decode(errors="replace")
                    destination = root / "direct-ftp-with-forced-proxy.json"
                    result = subprocess.run([str(probe), f"ftp://127.0.0.1:{server.server_address[1]}/test/", str(destination)], stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=30)
                    assert result.returncode == 0, result.stdout.decode(errors="replace")
                    assert destination.read_bytes() == remote_files["manifest.json"]
                    checks += 1
                    for i, (name, limit, expected, success) in enumerate((("manifest.json", 1048576, -1, True), (manifest["package"], 268435456, manifest["size"], True),
                                                                                    ("manifest.json", 10, -1, False), ("manifest.json", 1048576, 1, False), ("missing.zip", 1048576, -1, False))):
                        destination = root / f"download-{i}"
                        invoke("--download", settings, name, destination, limit, expected, success=success)
                        assert destination.read_bytes() == remote_files[name] if success else not destination.exists()
                        checks += 1
                finally:
                    server.shutdown()
                    thread.join(timeout=5)

            events = []
            class FakeFTP:
                def __enter__(self): return self
                def __exit__(self, *args): pass
                def connect(self, *args, **kwargs): events.append("connect")
                def login(self, *args): events.append("login")
                def set_pasv(self, *args): pass
                def cwd(self, *args): events.append("cwd")
                def retrbinary(self, *args): raise ftplib.error_perm("550 not found")
                def storbinary(self, command, stream):
                    assert stream.read()
                    events.append(command)
                def rename(self, source, destination): events.append("RENAME " + destination)
            with patch.dict(os.environ, {"EM_FTP_USER": "test", "EM_FTP_PASSWORD": "test"}):
                upload_release(release, manifest, "ftp://127.0.0.1/test/", FakeFTP)
                assert events[-1] == "RENAME manifest.json"
                assert events.index("RENAME " + manifest["package"]) < len(events) - 2
                checks += 1
                class SameBuildFTP(FakeFTP):
                    def retrbinary(self, command, callback): callback(json.dumps(manifest).encode("utf-8"))
                events.clear()
                try:
                    upload_release(release, manifest, "ftp://127.0.0.1/test/", SameBuildFTP)
                    raise AssertionError("Same remote build was accepted")
                except ValueError:
                    assert not any(event.startswith("STOR") for event in events)
                    checks += 1
                class FailedFTP(FakeFTP):
                    def storbinary(self, command, stream): raise OSError("simulated transfer failure")
                events.clear()
                try:
                    upload_release(release, manifest, "ftp://127.0.0.1/test/", FailedFTP)
                    raise AssertionError("Failure was ignored")
                except OSError:
                    assert "RENAME manifest.json" not in events
                    checks += 1
        finally:
            for stage in stages:
                if stage.exists(): shutil.rmtree(stage)
    print(f"[UPDATE TEST SUCCESS] {args.config}: {checks} checks, no external FTP access")


if __name__ == "__main__":
    main()
