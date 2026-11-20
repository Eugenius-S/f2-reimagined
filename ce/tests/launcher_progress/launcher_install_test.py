"""Windows launcher integration checks: local HTTP packages and disposable installs only."""

import contextlib
import ctypes
from ctypes import wintypes
import functools
import hashlib
import http.server
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import threading
import time
import unittest
import zipfile

sys.stdout.reconfigure(encoding="utf-8", errors="replace")
sys.stderr.reconfigure(encoding="utf-8", errors="replace")
LAUNCHER = Path(sys.argv.pop(1)).resolve()
REQUIRED = ("fallout2-ce.exe", "ce.dat", "scene-exporter.exe")
ZIP = "fallout2-ce-3d-win64.zip"
ART = "user-assets/fullscreen-20261028/wallpapers-selected/hires/menu/art__intrface__hr_mainmenu__d0_f0_21-9.png"
PUBLISHED_BOOTSTRAP = os.environ.get("F2R_PUBLISHED_BOOTSTRAP_FIXTURE")
PUBLISHED_BOOTSTRAP_HASH = "9aa8c65b3b8054fa168f3d599bb2f3b5c92550bba4ac8644e9c7be4694bc0b4d"


class QuietHandler(http.server.SimpleHTTPRequestHandler):
    def log_message(self, *_args):
        pass

    def do_GET(self):
        if self.path == "/held/version.txt":
            self.server.image_ready.set()
            self.server.release_image.wait(30)
            self.path = "/version.txt"
        with contextlib.suppress(ConnectionResetError, ConnectionAbortedError):
            super().do_GET()


def hash_during_removal(path):
    import msvcrt
    kernel = ctypes.WinDLL("kernel32", use_last_error=True)
    kernel.CreateFileW.argtypes = (wintypes.LPCWSTR, wintypes.DWORD, wintypes.DWORD,
                                  wintypes.LPVOID, wintypes.DWORD, wintypes.DWORD, wintypes.HANDLE)
    kernel.CreateFileW.restype = wintypes.HANDLE
    # The helper holds DELETE access, so the reader must share delete too.
    handle = kernel.CreateFileW(str(path), 0x80000000, 7, None, 3, 0, None)
    if handle == wintypes.HANDLE(-1).value:
        raise ctypes.WinError(ctypes.get_last_error())
    with os.fdopen(msvcrt.open_osfhandle(handle, os.O_RDONLY | os.O_BINARY), "rb") as stream:
        return hashlib.sha256(stream.read()).hexdigest()


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
        self.payload["fallout2-ce-3d-launcher.exe"] = LAUNCHER.read_bytes()
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
        launcher = self.payload.get("fallout2-ce-3d-launcher.exe", LAUNCHER.read_bytes())
        launcher_hash = hashlib.sha256(launcher).hexdigest()
        (self.release / "launcher-sha256.txt").write_text(launcher_hash, encoding="ascii")
        with zipfile.ZipFile(self.release / ZIP, "w", zipfile.ZIP_DEFLATED) as package:
            for name, content in self.payload.items():
                package.writestr(name, content)
            package.writestr("launcher-sha256.txt", launcher_hash)

    def run_launcher(self, language="en", launcher=LAUNCHER, extra_args=(), env=None):
        result = subprocess.run([str(launcher), "--game-root", str(self.root),
            "--update-url", self.url, "--mirror-url", self.url, "--ui-language", language,
            "--no-start", *extra_args], capture_output=True, timeout=90, env=env,
            creationflags=subprocess.CREATE_NO_WINDOW)
        output = result.stdout.decode("utf-8", errors="replace") + result.stderr.decode("utf-8", errors="replace")
        print(f"\n{self.id()}: exit {result.returncode}\n{output}", flush=True)
        return result.returncode, output



class InstallTests(LauncherFixture):
    def tombstones(self):
        return set(self.root.glob("f2r*.tmp"))

    def wait_for_tombstones(self, expected):
        deadline = time.monotonic() + 10
        while self.tombstones() != expected and time.monotonic() < deadline:
            time.sleep(0.1)
        self.assertEqual(self.tombstones(), expected)

    @contextlib.contextmanager
    def held_image(self, launcher):
        self.server.image_ready = ready = threading.Event()
        self.server.release_image = release = threading.Event()
        with subprocess.Popen([str(launcher), "--check-release", "--game-root", str(self.root),
                "--update-url", self.url + "/held", "--mirror-url", self.url + "/held", "--release-info-url", ""],
                creationflags=subprocess.CREATE_NO_WINDOW, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL) as holder:
            try:
                self.assertTrue(ready.wait(8))
                yield holder
            finally:
                holder.terminate()
                release.set()

    def assert_failed(self, code, output, filename):
        self.assertNotEqual(code, 0, output)
        self.assertIn(filename, output)
        self.assertEqual((self.root / "version.txt").read_bytes(), b"old-build")
        self.assertNotIn("done (--no-start)", output)

    def test_launcher_can_update_itself_and_start_the_installed_version(self):
        launcher = self.root / "Fallout 2 Reimagined.exe"
        launcher.write_bytes(LAUNCHER.read_bytes())
        self.published_launcher = LAUNCHER.read_bytes() + b"F2R launcher release identity: new-build"
        self.payload["fallout2-ce-3d-launcher.exe"] = self.published_launcher
        self.package()

        code, output = self.run_launcher(launcher=launcher)
        self.assertEqual(code, 0, output)
        self.assertEqual((self.root / "version.txt").read_bytes(), b"new-build")
        self.assertTrue(launcher.is_file())
        self.assertEqual(launcher.read_bytes(), self.published_launcher)

        code, output = self.run_launcher(launcher=launcher)
        self.assertEqual(code, 0, output)
        self.assertIn("installed build: new-build", output)
        self.assertIn("matches the release", output)
        self.wait_for_tombstones(set())
        self.assertFalse((self.root / "Fallout 2 Reimagined.exe.old").exists())

    def test_same_game_version_still_refills_a_stale_launcher(self):
        launcher = self.root / "Fallout 2 Reimagined.exe"
        launcher.write_bytes(LAUNCHER.read_bytes())
        (self.root / "version.txt").write_bytes(b"new-build")
        art = self.root / ART
        art.parent.mkdir(parents=True, exist_ok=True)
        art.write_bytes(b"existing art")
        self.published_launcher = LAUNCHER.read_bytes() + b"F2R launcher release identity: new-build"
        self.payload["version.txt"] = b"new-build"
        self.payload["fallout2-ce-3d-launcher.exe"] = self.published_launcher
        self.package()

        code, output = self.run_launcher()
        self.assertEqual(code, 0, output)
        self.assertEqual(launcher.read_bytes(), self.published_launcher)

        code, output = self.run_launcher(launcher=launcher)
        self.assertEqual(code, 0, output)
        self.assertIn("installed build: new-build", output)
        self.assertEqual(launcher.read_bytes(), self.published_launcher)

    def test_release_check_reports_remote_and_installed_launcher_identity(self):
        launcher = self.root / "Fallout 2 Reimagined.exe"
        launcher.write_bytes(LAUNCHER.read_bytes())
        expected = hashlib.sha256(self.payload["fallout2-ce-3d-launcher.exe"]).hexdigest()
        result = subprocess.run([str(LAUNCHER), "--check-release", "--game-root", str(self.root),
            "--update-url", self.url, "--mirror-url", self.url, "--release-info-url", ""],
            capture_output=True, timeout=30)
        output = result.stdout.decode("utf-8", errors="replace") + result.stderr.decode("utf-8", errors="replace")
        self.assertEqual(result.returncode, 0, output)
        self.assertIn(f"RELEASE_LAUNCHER_SHA256:{expected}", output)
        self.assertIn(f"RELEASE_INSTALLED_LAUNCHER_SHA256:{hashlib.sha256(launcher.read_bytes()).hexdigest()}", output)

    def test_launcher_hash_mismatch_changes_nothing(self):
        (self.release / "launcher-sha256.txt").write_text("0" * 64, encoding="ascii")
        before = {p.relative_to(self.root): p.read_bytes() for p in self.root.rglob("*") if p.is_file()}

        code, output = self.run_launcher()

        self.assertNotEqual(code, 0, output)
        self.assertIn("launcher hash metadata", output)
        self.assertEqual(before, {p.relative_to(self.root): p.read_bytes() for p in self.root.rglob("*") if p.is_file()})
        self.assertFalse((self.root / "f2reimagined-backups").exists())

    def test_missing_launcher_asset_does_not_stamp_success(self):
        self.payload.pop("fallout2-ce-3d-launcher.exe", None)
        self.package()

        code, output = self.run_launcher()

        self.assert_failed(code, output, "fallout2-ce-3d-launcher.exe")
        self.assertFalse((self.root / "f2reimagined-backups").exists())

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
        self.assertEqual(list(previous.parent.iterdir()), [previous])

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

    def test_legacy_release_without_hash_sidecar_still_updates(self):
        (self.release / "launcher-sha256.txt").unlink()
        with zipfile.ZipFile(self.release / ZIP, "w", zipfile.ZIP_DEFLATED) as package:
            for name, content in self.payload.items():
                package.writestr(name, content)
        code, output = self.run_launcher()
        self.assertEqual(code, 0, output)
        self.assertEqual((self.root / "version.txt").read_bytes(), b"new-build")
        self.assertEqual((self.root / "Fallout 2 Reimagined.exe").read_bytes(), self.payload["fallout2-ce-3d-launcher.exe"])

    def test_malformed_package_hash_sidecar_changes_nothing(self):
        self.package()
        with zipfile.ZipFile(self.release / ZIP, "a", zipfile.ZIP_DEFLATED) as package:
            package.writestr("launcher-sha256.txt", "not-a-digest")
        before = {p.relative_to(self.root): p.read_bytes() for p in self.root.rglob("*") if p.is_file()}
        code, output = self.run_launcher()
        self.assert_failed(code, output, "hash metadata")
        self.assertEqual(before, {p.relative_to(self.root): p.read_bytes() for p in self.root.rglob("*") if p.is_file()})

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

    def test_locked_launcher_preserves_build_then_updates_on_retry(self):
        launcher = self.root / "Fallout 2 Reimagined.exe"
        launcher.write_bytes(LAUNCHER.read_bytes() + b"previous generation")
        (self.root / "other-mod.dat").write_bytes(b"keep")
        sentinel = launcher.with_suffix(".exe.old")
        sentinel.write_bytes(b"pre-existing file, not owned by this launcher")
        snapshot = lambda: {p.relative_to(self.root): hashlib.sha256(p.read_bytes()).hexdigest()
                            for p in self.root.rglob("*") if p.is_file()}
        before = snapshot()
        with read_lock(launcher):
            code, output = self.run_launcher()
        self.assert_failed(code, output, launcher.name)
        self.assertEqual(before, snapshot())
        code, output = self.run_launcher()
        self.assertEqual(code, 0, output)
        self.assertEqual(launcher.read_bytes(), self.payload["fallout2-ce-3d-launcher.exe"])
        self.assertEqual((self.root / "other-mod.dat").read_bytes(), b"keep")
        self.assertEqual(hashlib.sha256(sentinel.read_bytes()).hexdigest(), before[sentinel.relative_to(self.root)])

    def test_two_live_launcher_images_can_be_replaced(self):
        launcher = self.root / "Fallout 2 Reimagined.exe"
        previous = LAUNCHER.read_bytes() + b"previous generation"
        launcher.write_bytes(previous)
        # Keeps a second copy of the installed image mapped while its worker
        # runs, like the GUI does, but never creates a window.
        with self.held_image(launcher) as holder:
            code, output = self.run_launcher(launcher=launcher)
            self.assertEqual(code, 0, output)
            self.assertIsNone(holder.poll(), "old image must still be running during replacement")
            self.assertEqual(launcher.read_bytes(), self.payload["fallout2-ce-3d-launcher.exe"])
            self.assertEqual([hash_during_removal(p) for p in self.tombstones()], [hashlib.sha256(previous).hexdigest()])
        self.wait_for_tombstones(set())

    @unittest.skipUnless(PUBLISHED_BOOTSTRAP, "exact published bootstrap fixture not provided")
    def test_published_launcher_can_move_to_fixed_package_in_console_mode(self):
        launcher = self.root / "Fallout 2 Reimagined.exe"
        legacy = Path(PUBLISHED_BOOTSTRAP).read_bytes()
        self.assertEqual(hashlib.sha256(legacy).hexdigest(), PUBLISHED_BOOTSTRAP_HASH)
        launcher.write_bytes(legacy)
        code, output = self.run_launcher(launcher=launcher)
        self.assertEqual(code, 0, output)
        self.assertEqual(launcher.read_bytes(), self.payload["fallout2-ce-3d-launcher.exe"])
        self.assertEqual((self.root / "version.txt").read_bytes(), b"new-build")
        code, output = self.run_launcher(launcher=launcher)
        self.assertEqual(code, 0, output)
        self.assertIn("matches the release: nothing to pull", output)

    def test_uninstall_leaves_unidentified_legacy_orphan_untouched(self):
        orphan = self.root / "Fallout 2 Reimagined.exe.old"
        orphan.write_bytes(LAUNCHER.read_bytes())
        keep = self.root / "other.old"
        keep.write_bytes(b"not a launcher artifact")
        env = dict(os.environ, APPDATA=str(self.base / "appdata"))
        with read_lock(orphan):
            code, output = self.run_launcher(extra_args=("--uninstall",), env=env)
        self.assertEqual(code, 0, output)
        self.assertTrue(orphan.is_file())
        code, output = self.run_launcher(extra_args=("--uninstall",), env=env)
        self.assertEqual(code, 0, output)
        self.assertEqual(orphan.read_bytes(), LAUNCHER.read_bytes())
        self.assertEqual(keep.read_bytes(), b"not a launcher artifact")

    def test_reinstall_and_uninstall_while_first_cleanup_is_pending(self):
        launcher = self.root / "Fallout 2 Reimagined.exe"
        launcher.write_bytes(LAUNCHER.read_bytes() + b"old generation")
        previous = hashlib.sha256(launcher.read_bytes()).hexdigest()
        env = dict(os.environ, APPDATA=str(self.base / "appdata"))
        with self.held_image(launcher) as first:
            code, output = self.run_launcher(extra_args=("--uninstall",), env=env)
            self.assertEqual(code, 0, output)
            self.assertIn("launcher removal is pending", output)
            self.assertEqual([hash_during_removal(p) for p in self.tombstones()], [previous])
            code, output = self.run_launcher()
            self.assertEqual(code, 0, output)
            replacement = hashlib.sha256(launcher.read_bytes()).hexdigest()
            self.assertEqual(replacement, hashlib.sha256(self.payload["fallout2-ce-3d-launcher.exe"]).hexdigest())
            with self.held_image(launcher):
                code, output = self.run_launcher(extra_args=("--uninstall",), env=env)
                self.assertEqual(code, 0, output)
                artifacts = {hash_during_removal(p): p for p in self.tombstones()}
                self.assertEqual(set(artifacts), {previous, replacement})
                first.terminate()
                first.wait(timeout=10)
                self.wait_for_tombstones({artifacts[replacement]})
                self.assertEqual(hash_during_removal(artifacts[replacement]), replacement)
        self.wait_for_tombstones(set())

    def test_uninstall_removes_only_mod_files_settings_and_delayed_launcher(self):
        launcher = self.root / "Fallout 2 Reimagined.exe"
        launcher.write_bytes(LAUNCHER.read_bytes())
        originals = {
            "master.dat": b"original master",
            "critter.dat": b"original critters",
            "fallout2.exe": b"original executable",
            "fallout2.cfg": b"[system]\nlanguage=english\n",
        }
        for name, content in originals.items():
            (self.root / name).write_bytes(content)
        save = self.root / "data" / "savegame" / "slot01.sav"
        save.parent.mkdir(parents=True)
        save.write_bytes(b"player save")
        (self.root / "mods").mkdir()
        (self.root / "mods" / "other-mod.dat").write_bytes(b"another mod")
        (self.root / "mod-cache").mkdir()
        (self.root / "mod-cache" / "scene.bin").write_bytes(b"mod cache")
        (self.root / "f2reimagined-backups" / "old-build").mkdir(parents=True)
        (self.root / "f2reimagined-backups" / "old-build" / "version.txt").write_bytes(b"old")
        art = self.root / ART
        art.parent.mkdir(parents=True)
        art.write_bytes(b"mod menu art")
        custom_art = self.root / "user-assets" / "player-custom" / "keep.png"
        custom_art.parent.mkdir(parents=True)
        custom_art.write_bytes(b"player art")
        appdata = self.base / "appdata"
        settings = appdata / "Fallout2Reimagined" / "launcher.ini"
        settings.parent.mkdir(parents=True)
        settings.write_text(f"[launcher]\nauto_updates=1\ngame_root={self.root}\n", encoding="utf-8")
        keep_settings = settings.with_name("other.ini")
        keep_settings.write_text("keep", encoding="utf-8")
        env = os.environ.copy()
        env["APPDATA"] = str(appdata)

        code, output = self.run_launcher(launcher=launcher, extra_args=("--uninstall",), env=env)
        self.assertEqual(code, 0, output)
        self.wait_for_tombstones(set())
        self.assertFalse(launcher.exists())
        self.assertFalse((self.root / "Fallout 2 Reimagined.exe.old").exists())
        for name in ("fallout2-ce.exe", "ce.dat", "scene-exporter.exe", "version.txt", "mod-cache", "f2reimagined-backups"):
            self.assertFalse((self.root / name).exists(), name)
        self.assertFalse((self.root / ART).exists())
        self.assertFalse(settings.exists())
        self.assertTrue(keep_settings.exists())
        self.assertEqual(save.read_bytes(), b"player save")
        self.assertEqual((self.root / "mods" / "other-mod.dat").read_bytes(), b"another mod")
        self.assertEqual(custom_art.read_bytes(), b"player art")
        for name, content in originals.items():
            self.assertEqual((self.root / name).read_bytes(), content, name)

    def test_uninstall_root_a_preserves_settings_configured_for_root_b(self):
        launcher = self.root / "Fallout 2 Reimagined.exe"
        launcher.write_bytes(LAUNCHER.read_bytes())
        root_b = self.base / "game-b"
        root_b.mkdir()
        settings = self.base / "appdata" / "Fallout2Reimagined" / "launcher.ini"
        settings.parent.mkdir(parents=True)
        settings.write_text(f"[launcher]\ngame_root={root_b}\nauto_updates=1\n", encoding="utf-8")
        env = os.environ.copy()
        env["APPDATA"] = str(settings.parents[1])
        code, output = self.run_launcher(launcher=launcher, extra_args=("--uninstall",), env=env)
        self.assertEqual(code, 0, output)
        self.assertTrue(settings.exists())
        self.assertIn(str(root_b), settings.read_text(encoding="utf-8"))

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
