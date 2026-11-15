"""Windows launcher integration checks: local HTTP packages and disposable installs only."""

import contextlib
import ctypes
from ctypes import wintypes
import functools
import http.server
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import threading
import unittest
import zipfile

sys.stdout.reconfigure(encoding="utf-8", errors="replace")
sys.stderr.reconfigure(encoding="utf-8", errors="replace")
LAUNCHER = Path(sys.argv.pop(1)).resolve()
REQUIRED = ("fallout2-ce.exe", "ce.dat", "scene-exporter.exe")
ZIP = "fallout2-ce-3d-win64.zip"
ART = "user-assets/fullscreen-20261028/wallpapers-selected/hires/menu/art__intrface__hr_mainmenu__d0_f0_21-9.png"


class QuietHandler(http.server.SimpleHTTPRequestHandler):
    def log_message(self, *_args):
        pass


@contextlib.contextmanager
def read_lock(path):
    # Allow backup reads, but deny writes and renames just like a running exe.
    kernel = ctypes.WinDLL("kernel32", use_last_error=True)
    kernel.CreateFileW.argtypes = (wintypes.LPCWSTR, wintypes.DWORD, wintypes.DWORD,
                                  wintypes.LPVOID, wintypes.DWORD, wintypes.DWORD, wintypes.HANDLE)
    kernel.CreateFileW.restype = wintypes.HANDLE
    kernel.CloseHandle.argtypes = (wintypes.HANDLE,)
    handle = kernel.CreateFileW(str(path), 0x80000000, 1, None, 3, 0, None)
    if handle == wintypes.HANDLE(-1).value:
        raise ctypes.WinError(ctypes.get_last_error())
    try:
        yield
    finally:
        kernel.CloseHandle(handle)


@unittest.skipUnless(os.name == "nt", "the launcher uses Windows system tools")
class InstallTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(prefix="f2r-install-test-")
        self.addCleanup(self.temp.cleanup)
        self.base = Path(self.temp.name)
        self.root = self.base / "game"
        self.root.mkdir()
        for name in ("master.dat", "critter.dat", *REQUIRED, "version.txt", "fallout2.cfg"):
            (self.root / name).write_bytes(b"old-build")
        self.payload = {name: b"new-build" for name in REQUIRED}
        # A tiny existing console test stands in for the exporter; the game never runs.
        self.payload["scene-exporter.exe"] = LAUNCHER.with_name("launcher_routes_tests.exe").read_bytes()
        self.payload.update({"version.txt": b"new-build", ART: b"art", "fallout2.cfg": b"default-config"})
        self.release = self.base / "release"
        self.release.mkdir()
        (self.release / "version.txt").write_text("new-build", encoding="ascii")
        self.package()
        self.server = http.server.ThreadingHTTPServer(("127.0.0.1", 0),
            functools.partial(QuietHandler, directory=str(self.release)))
        thread = threading.Thread(target=self.server.serve_forever, daemon=True)
        thread.start()
        self.addCleanup(self.server.server_close)
        self.addCleanup(thread.join)
        self.addCleanup(self.server.shutdown)
        self.url = f"http://127.0.0.1:{self.server.server_port}"

    def package(self):
        with zipfile.ZipFile(self.release / ZIP, "w", zipfile.ZIP_DEFLATED) as package:
            for name, content in self.payload.items():
                package.writestr(name, content)

    def run_launcher(self, language="en"):
        result = subprocess.run([str(LAUNCHER), "--game-root", str(self.root),
            "--update-url", self.url, "--mirror-url", self.url, "--ui-language", language,
            "--no-start"], capture_output=True, timeout=90)
        output = result.stdout.decode("utf-8", errors="replace") + result.stderr.decode("utf-8", errors="replace")
        print(f"\n{self.id()}: exit {result.returncode}\n{output}", flush=True)
        return result.returncode, output

    def assert_failed(self, code, output, filename):
        self.assertNotEqual(code, 0, output)
        self.assertIn(filename, output)
        self.assertEqual((self.root / "version.txt").read_bytes(), b"old-build")
        self.assertNotIn("done (--no-start)", output)

    def test_locked_exe_and_retry(self):
        with read_lock(self.root / REQUIRED[0]):
            code, output = self.run_launcher()
        self.assert_failed(code, output, REQUIRED[0])
        self.assertEqual((self.root / REQUIRED[0]).read_bytes(), b"old-build")
        code, output = self.run_launcher()
        self.assertEqual(code, 0, output)
        self.assertEqual((self.root / REQUIRED[0]).read_bytes(), b"new-build")
        self.assertEqual((self.root / "version.txt").read_bytes(), b"new-build")
        self.assertEqual((self.root / "fallout2.cfg").read_bytes(), b"old-build")

    def test_corrupt_zip_tries_fallback(self):
        (self.release / ZIP).write_bytes(b"this is not a zip")
        code, output = self.run_launcher("ru")
        self.assert_failed(code, output, "Expand-Archive")
        self.assertIn("скачанный файл не распаковался", output)
        self.assertNotIn("installing into the game folder:", output)

    def test_missing_required_files_change_nothing(self):
        for missing in REQUIRED:
            with self.subTest(missing=missing):
                saved = self.payload.pop(missing)
                self.package()
                self.payload[missing] = saved
                before = {p.relative_to(self.root): p.read_bytes() for p in self.root.rglob("*") if p.is_file()}
                code, output = self.run_launcher()
                self.assert_failed(code, output, missing)
                after = {p.relative_to(self.root): p.read_bytes() for p in self.root.rglob("*") if p.is_file()}
                self.assertEqual(before, after)

    def test_required_file_is_a_directory(self):
        self.payload.pop("ce.dat")
        self.payload["ce.dat/"] = b""
        self.package()
        code, output = self.run_launcher()
        self.assert_failed(code, output, "ce.dat")
        self.assertEqual((self.root / REQUIRED[0]).read_bytes(), b"old-build")
        self.assertFalse((self.root / "f2reimagined-backups").exists())

    def test_missing_optional_files_are_skipped(self):
        self.payload.pop(ART)
        self.payload.pop("fallout2.cfg")
        self.package()
        (self.root / "fallout2.cfg").unlink()
        code, output = self.run_launcher()
        self.assertEqual(code, 0, output)
        self.assertEqual((self.root / "version.txt").read_bytes(), b"new-build")

    def test_default_config_copy_failure(self):
        (self.root / "fallout2.cfg").unlink()
        self.payload.pop("fallout2.cfg")
        self.payload["fallout2.cfg/blocked"] = b"not a config file"
        self.package()
        code, output = self.run_launcher()
        self.assert_failed(code, output, "fallout2.cfg")

    def test_present_optional_tree_failure(self):
        (self.root / "user-assets").write_bytes(b"blocks the destination directory")
        code, output = self.run_launcher()
        self.assert_failed(code, output, "user-assets")

    def test_present_optional_launcher_failure(self):
        name = "Fallout 2 Reimagined.exe"
        (self.root / name).write_bytes(b"old-launcher")
        self.payload["fallout2-ce-3d-launcher.exe"] = b"new-launcher"
        self.package()
        with read_lock(self.root / name):
            code, output = self.run_launcher()
        self.assert_failed(code, output, name)
        self.assertEqual((self.root / name).read_bytes(), b"old-launcher")

    def test_locked_version_keeps_old_stamp(self):
        with read_lock(self.root / "version.txt"):
            code, output = self.run_launcher()
        self.assert_failed(code, output, "version.txt")


if __name__ == "__main__":
    unittest.main(verbosity=2)
