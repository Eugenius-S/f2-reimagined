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
class LauncherFixture(unittest.TestCase):
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



class InstallTests(LauncherFixture):
    def assert_failed(self, code, output, filename):
        self.assertNotEqual(code, 0, output)
        self.assertIn(filename, output)
        self.assertEqual((self.root / "version.txt").read_bytes(), b"old-build")
        self.assertNotIn("done (--no-start)", output)

    def test_zip_version_wins_over_remote_version(self):
        (self.release / "version.txt").write_text("remote-build-A", encoding="ascii")
        self.payload["version.txt"] = b"package-build-B\r\n"
        self.package()
        code, output = self.run_launcher()
        self.assertEqual(code, 0, output)
        self.assertEqual((self.root / REQUIRED[0]).read_bytes(), b"new-build")
        self.assertEqual((self.root / "version.txt").read_bytes(), b"package-build-B")
        self.assertIn("the game folder holds build package-build-B", output)

    def test_zip_version_matches_installed_keeps_previous_backup(self):
        self.payload["version.txt"] = b"old-build"
        previous = self.root / "f2reimagined-backups" / "previous-build"
        previous.mkdir(parents=True)
        (previous / "version.txt").write_bytes(b"previous-build")
        self.package()
        code, output = self.run_launcher()
        self.assertEqual(code, 0, output)
        self.assertEqual((self.root / "version.txt").read_bytes(), b"old-build")
        # The wide-art refill at the same version journals for its recovery:
        # its fresh snapshot of the current state joins the folder, and the
        # older rollback point stays (307, 313).
        self.assertEqual({p.name for p in previous.parent.iterdir()}, {"refill-snapshot", "previous-build"})
        self.assertIn("backed up build refill-snapshot to f2reimagined-backups", output)
        self.assertIn("user-assets: installed", output)

    def test_refill_without_a_recovery_point_changes_nothing(self):
        # A plain file where the backups folder belongs: no backup of this
        # build stands and no manual snapshot can be made, so the same-version
        # refill must refuse before its first write (311).
        self.payload["version.txt"] = b"old-build"
        self.package()
        (self.root / "f2reimagined-backups").write_bytes(b"not a folder")
        before = {p.relative_to(self.root): p.read_bytes() for p in self.root.rglob("*") if p.is_file()}
        code, output = self.run_launcher()
        self.assertNotEqual(code, 0, output)
        self.assertIn("the wide-art refill found no complete backup of this build", output)
        self.assertNotIn("installing into the game folder:", output)
        self.assertNotIn("done (--no-start)", output)
        after = {p.relative_to(self.root): p.read_bytes() for p in self.root.rglob("*") if p.is_file()}
        self.assertEqual(before, after)

    def test_failed_refill_rolls_back_from_its_own_snapshot(self):
        # The refill snapshots the build as "refill-snapshot", replaces the
        # build files, then its stamp write meets a held version.txt.  The
        # in-place restore meets the same hold, so the marker names that
        # snapshot; once the hold is gone, --rollback restores from it and
        # the player's files stand untouched (311, 313).
        self.payload["version.txt"] = b"old-build"
        self.package()
        save = self.root / "data" / "savegame" / "SLOT01" / "SAVE.DAT"
        save.parent.mkdir(parents=True)
        save.write_bytes(b"player-save")
        backups = self.root / "f2reimagined-backups"
        with read_lock(self.root / "version.txt"):
            code, output = self.run_launcher()
        self.assert_failed(code, output, "version.txt")
        self.assertIn("backed up build refill-snapshot to f2reimagined-backups", output)
        self.assertIn("installing into the game folder:", output)
        self.assertIn("the restore could not finish", output)
        self.assertEqual((backups / "update-in-progress.txt").read_text(encoding="ascii").strip(), "refill-snapshot")
        self.assertTrue((backups / "refill-snapshot" / "f2r-backup-receipt.txt").is_file())

        result = subprocess.run([str(LAUNCHER), "--game-root", str(self.root), "--rollback"],
                                capture_output=True, timeout=90)
        output = result.stdout.decode("utf-8", errors="replace") + result.stderr.decode("utf-8", errors="replace")
        print(f"\n{self.id()} rollback: exit {result.returncode}\n{output}", flush=True)
        self.assertEqual(result.returncode, 0, output)
        self.assertIn("restoring the saved build from f2reimagined-backups/refill-snapshot", output)
        self.assertIn("the saved build refill-snapshot stands", output)
        for name in REQUIRED:
            self.assertEqual((self.root / name).read_bytes(), b"old-build", name)
        self.assertEqual((self.root / "version.txt").read_bytes(), b"old-build")
        self.assertFalse((self.root / ART).exists())
        self.assertFalse((backups / "update-in-progress.txt").exists())
        self.assertTrue((backups / "refill-snapshot").is_dir())
        self.assertEqual(save.read_bytes(), b"player-save")
        for name in ("fallout2.cfg", "master.dat", "critter.dat"):
            self.assertEqual((self.root / name).read_bytes(), b"old-build", name)

    def test_failed_refill_never_restores_an_older_manual_backup(self):
        # The real path to an old manual backup: a stamp-less install
        # (bytes old-build) updates to new-build and keeps "manual".  A later
        # new-build refill fails on a held version.txt; its rollback must
        # bring back the current new-build bytes, not the older manual
        # backup, and that backup stays as it was (313).
        (self.root / "version.txt").unlink()
        code, output = self.run_launcher()
        self.assertEqual(code, 0, output)
        backups = self.root / "f2reimagined-backups"
        manual = backups / "manual"
        self.assertTrue((manual / "f2r-backup-receipt.txt").is_file())
        manualBefore = {p.relative_to(manual): p.read_bytes() for p in manual.rglob("*") if p.is_file()}
        current = {p.relative_to(self.root): p.read_bytes()
                   for p in self.root.rglob("*") if p.is_file() and backups not in p.parents}
        # The release announces another build, so the launcher downloads; the
        # package inside still says new-build, which makes it a refill.
        (self.release / "version.txt").write_text("remote-other", encoding="ascii")
        with read_lock(self.root / "version.txt"):
            code, output = self.run_launcher()
        self.assertNotEqual(code, 0, output)
        self.assertIn("updating: new-build -> remote-other", output)
        self.assertIn("installing into the game folder:", output)
        self.assertIn("the restore could not finish", output)
        self.assertNotEqual((backups / "update-in-progress.txt").read_text(encoding="ascii").strip(), "manual")

        result = subprocess.run([str(LAUNCHER), "--game-root", str(self.root), "--rollback"],
                                capture_output=True, timeout=90)
        output = result.stdout.decode("utf-8", errors="replace") + result.stderr.decode("utf-8", errors="replace")
        print(f"\n{self.id()} rollback: exit {result.returncode}\n{output}", flush=True)
        self.assertEqual(result.returncode, 0, output)
        after = {p.relative_to(self.root): p.read_bytes()
                 for p in self.root.rglob("*") if p.is_file() and backups not in p.parents}
        self.assertEqual(after, current)
        self.assertEqual({p.relative_to(manual): p.read_bytes() for p in manual.rglob("*") if p.is_file()},
                         manualBefore)
        self.assertFalse((backups / "update-in-progress.txt").exists())

    def test_invalid_zip_version_changes_nothing(self):
        for invalid in (None, b"", b" \t\r\n", b"first\nsecond", b"first\n\n",
                        b"\nsecond", b"first\rsecond", b"bad\x00version", b".", b".."):
            with self.subTest(version=invalid):
                self.payload.pop("version.txt", None)
                if invalid is not None:
                    self.payload["version.txt"] = invalid
                self.package()
                before = {p.relative_to(self.root): p.read_bytes() for p in self.root.rglob("*") if p.is_file()}
                code, output = self.run_launcher("ru" if invalid == b"" else "en")
                self.assert_failed(code, output, "version.txt")
                if invalid == b"":
                    self.assertIn("в скачанном пакете некорректный version.txt; ничего не изменено", output)
                after = {p.relative_to(self.root): p.read_bytes() for p in self.root.rglob("*") if p.is_file()}
                self.assertEqual(before, after)
                self.assertFalse((self.root / "f2reimagined-backups").exists())

    def test_zip_version_is_a_directory(self):
        self.payload.pop("version.txt")
        self.payload["version.txt/"] = b""
        self.package()
        code, output = self.run_launcher()
        self.assert_failed(code, output, "version.txt")
        self.assertEqual((self.root / REQUIRED[0]).read_bytes(), b"old-build")
        self.assertFalse((self.root / "f2reimagined-backups").exists())

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
