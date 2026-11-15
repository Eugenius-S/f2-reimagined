"""RP operations use only synthetic games, local packages and real filesystem failures."""
import hashlib
import json
import os
import time
import ctypes
from ctypes import wintypes
from pathlib import Path
import subprocess
import sys
import zipfile

from launcher_install_test import LauncherFixture, LAUNCHER, read_lock, ART


def snapshot(root):
    return {p.relative_to(root).as_posix(): hashlib.sha256(p.read_bytes()).hexdigest()
            for p in root.rglob("*") if p.is_file()}


class NativeUi:
    """Inspect only this test process' HWNDs, without activating a window."""
    def __init__(self):
        self.user = ctypes.WinDLL("user32", use_last_error=True)
        self.gdi = ctypes.WinDLL("gdi32", use_last_error=True)
        for name, args, result in (
            ("GetDlgItem", [wintypes.HWND, ctypes.c_int], wintypes.HWND),
            ("SendMessageW", [wintypes.HWND, wintypes.UINT, wintypes.WPARAM, wintypes.LPARAM], ctypes.c_ssize_t),
            ("PostMessageW", [wintypes.HWND, wintypes.UINT, wintypes.WPARAM, wintypes.LPARAM], wintypes.BOOL),
            ("IsWindowEnabled", [wintypes.HWND], wintypes.BOOL),
            ("GetWindowThreadProcessId", [wintypes.HWND, ctypes.POINTER(wintypes.DWORD)], wintypes.DWORD),
            ("GetClientRect", [wintypes.HWND, ctypes.POINTER(wintypes.RECT)], wintypes.BOOL),
            ("PrintWindow", [wintypes.HWND, wintypes.HDC, wintypes.UINT], wintypes.BOOL),
        ):
            function = getattr(self.user, name)
            function.argtypes, function.restype = args, result
        self.callback_type = ctypes.WINFUNCTYPE(wintypes.BOOL, wintypes.HWND, wintypes.LPARAM)
        self.user.EnumWindows.argtypes = [self.callback_type, wintypes.LPARAM]
        self.gdi.CreateCompatibleDC.argtypes = [wintypes.HDC]
        self.gdi.CreateCompatibleDC.restype = wintypes.HDC
        self.gdi.CreateDIBSection.argtypes = [wintypes.HDC, ctypes.c_void_p, wintypes.UINT,
            ctypes.POINTER(ctypes.c_void_p), wintypes.HANDLE, wintypes.DWORD]
        self.gdi.CreateDIBSection.restype = wintypes.HBITMAP
        self.gdi.SelectObject.argtypes = [wintypes.HDC, wintypes.HANDLE]
        self.gdi.SelectObject.restype = wintypes.HANDLE
        self.gdi.DeleteObject.argtypes = [wintypes.HANDLE]
        self.gdi.DeleteDC.argtypes = [wintypes.HDC]

    def window(self, pid):
        found = []
        @self.callback_type
        def visit(hwnd, _):
            process = wintypes.DWORD()
            self.user.GetWindowThreadProcessId(hwnd, ctypes.byref(process))
            if process.value == pid and self.user.GetDlgItem(hwnd, 1005):
                found.append(hwnd)
            return True
        self.user.EnumWindows(visit, 0)
        return found[0] if found else None

    def pixels(self, hwnd):
        import struct
        rect = wintypes.RECT()
        if not self.user.GetClientRect(hwnd, ctypes.byref(rect)):
            raise ctypes.WinError(ctypes.get_last_error())
        width, height = rect.right, rect.bottom
        header = struct.pack("<IiiHHIIiiII", 40, width, -height, 1, 32, 0, width * height * 4, 0, 0, 0, 0)
        info = ctypes.create_string_buffer(header)
        bits = ctypes.c_void_p()
        dc = self.gdi.CreateCompatibleDC(None)
        bitmap = self.gdi.CreateDIBSection(dc, info, 0, ctypes.byref(bits), None, 0)
        if not bitmap:
            self.gdi.DeleteDC(dc)
            raise ctypes.WinError(ctypes.get_last_error())
        previous = self.gdi.SelectObject(dc, bitmap)
        try:
            if not self.user.PrintWindow(hwnd, dc, 1):
                raise RuntimeError("PrintWindow failed on the scratch launcher")
            return width, height, ctypes.string_at(bits, width * height * 4)
        finally:
            self.gdi.SelectObject(dc, previous)
            self.gdi.DeleteObject(bitmap)
            self.gdi.DeleteDC(dc)

    def checked(self, hwnd):
        width, _, pixels = self.pixels(hwnd)
        # Owner-drawn checkbox uses gold fill when ticked, dark fill when clear.
        return pixels[(324 * width + 30) * 4:(324 * width + 30) * 4 + 3] == bytes((91, 190, 236))


class RpInstallTests(LauncherFixture):
    # Inherited update tests live in their own CTest target.
    def setUp(self):
        super().setUp()
        (self.root / "version.txt").write_bytes(b"new-build")
        art = self.root / ART
        art.parent.mkdir(parents=True, exist_ok=True)
        art.write_bytes(b"art")
        (self.root / "scene-exporter.exe").write_bytes(self.payload["scene-exporter.exe"])
        for name in ("patch000.dat", "f2_res.dat", "data/maps/old.map",
                     "data/savegame/slot01/save.dat", "ddraw.ini"):
            path = self.root / name
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_bytes(b"original-" + name.encode())
        self.rp = {"a-added.txt": b"RP added", "ddraw.ini": b"RP settings",
                   "mods/rpu.dat": b"RP archive", "gl_ammomod.int": b"RP script"}
        self.rp_package()

    def rp_package(self):
        with zipfile.ZipFile(self.release / "rp.zip", "w", zipfile.ZIP_DEFLATED) as package:
            for name, content in self.rp.items():
                package.writestr(name, content)

    def rp_run(self, *args):
        result = subprocess.run([str(LAUNCHER), "--game-root", str(self.root),
            "--update-url", self.url, "--mirror-url", self.url, "--no-start",
            "--rp-zip-url", self.url + "/rp.zip", *args], capture_output=True, timeout=90)
        output = (result.stdout + result.stderr).decode("utf-8", errors="replace")
        print(f"\n{self.id()}: exit {result.returncode}\n{output}", flush=True)
        return result.returncode, output

    def test_copy_failure_restores_originals(self):
        # The RP copy fails after moving originals and copying earlier package files.
        (self.root / "zz-blocked").write_bytes(b"player file blocks RP parent")
        self.rp["zz-blocked/file.txt"] = b"cannot copy here"
        self.rp_package()
        before = snapshot(self.root)
        code, output = self.rp_run("--restoration-project")
        self.assertNotEqual(code, 0, output)
        self.assertEqual(before, snapshot(self.root))

    def install_rp(self):
        code, output = self.rp_run("--restoration-project")
        self.assertEqual(code, 0, output)
        self.assertTrue((self.root / "mods/rpu.dat").is_file())

    def test_locked_overwrite_rolls_back_and_retry(self):
        before = snapshot(self.root)
        with read_lock(self.root / "ddraw.ini"):
            code, output = self.rp_run("--restoration-project")
        self.assertNotEqual(code, 0, output)
        self.assertIn("copy failed", output)
        self.assertEqual(before, snapshot(self.root))
        self.install_rp()
        code, output = self.rp_run("--remove-restoration-project")
        self.assertEqual(code, 0, output)
        self.assertEqual(before, snapshot(self.root))

    def test_locked_original_rolls_back(self):
        before = snapshot(self.root)
        with read_lock(self.root / "patch000.dat"):
            code, output = self.rp_run("--restoration-project")
        self.assertNotEqual(code, 0, output)
        self.assertEqual(before, snapshot(self.root))

    def test_roundtrip_preserves_saves_even_in_package(self):
        self.rp["data/savegame/slot01/save.dat"] = b"must never replace a save"
        self.rp["data/savegame/slot02/save.dat"] = b"must never add a save"
        self.rp_package()
        before = snapshot(self.root)
        self.install_rp()
        self.assertEqual((self.root / "data/savegame/slot01/save.dat").read_bytes(),
                         b"original-data/savegame/slot01/save.dat")
        self.assertFalse((self.root / "data/savegame/slot02/save.dat").exists())
        code, output = self.rp_run("--remove-restoration-project")
        self.assertEqual(code, 0, output)
        self.assertEqual(before, snapshot(self.root))
        self.assertFalse((self.root / "backup/rpu").exists())

    def test_fresh_install_then_full_uninstall(self):
        import shutil
        for name in ("fallout2-ce.exe", "ce.dat", "scene-exporter.exe", "version.txt"):
            (self.root / name).unlink()
        shutil.rmtree(self.root / "user-assets")
        before = snapshot(self.root)
        self.install_rp()
        code, output = self.rp_run("--uninstall")
        self.assertEqual(code, 0, output)
        self.assertEqual(before, snapshot(self.root))

    def test_manual_rp_with_or_without_legacy_backup_is_untouched(self):
        (self.root / "mods").mkdir()
        (self.root / "mods/rpu.dat").write_bytes(b"manual RP")
        for legacy_backup in (False, True):
            with self.subTest(legacy_backup=legacy_backup):
                if legacy_backup:
                    (self.root / "backup/rpu").mkdir(parents=True)
                    (self.root / "backup/rpu/patch000.dat").write_bytes(b"manual backup")
                before = snapshot(self.root)
                code, output = self.rp_run("--remove-restoration-project")
                self.assertEqual(code, 0, output)
                self.assertIn("manual install was left untouched", output)
                self.assertEqual(before, snapshot(self.root))

    def test_modified_rp_blocks_full_uninstall_before_ce_changes(self):
        self.install_rp()
        (self.root / "ddraw.ini").write_bytes(b"player changed RP config")
        before = snapshot(self.root)
        code, output = self.rp_run("--uninstall")
        self.assertNotEqual(code, 0, output)
        self.assertIn("changed by the player", output)
        self.assertEqual(before, snapshot(self.root))
        self.assertNotIn("the mod is removed", output)

    def test_extra_mod_files_and_shared_order_stay(self):
        (self.root / "mods").mkdir()
        original = b"original.dat"  # No trailing newline is intentional.
        (self.root / "mods/mods_order.txt").write_bytes(original)
        self.rp["mods_order.txt"] = b"rpu.dat\r\n"
        self.rp_package()
        self.install_rp()
        (self.root / "mods/mods_order.txt").write_bytes(b"rpu.dat\r\nrpu_russian.dat\r\nother.dat\r\n")
        (self.root / "mods/rpu_russian.dat").write_bytes(b"translation")
        (self.root / "mods/other.dat").write_bytes(b"another mod")
        (self.root / "data/new-player-file.txt").write_bytes(b"player addition")
        (self.root / "fallout2.cfg").write_bytes(b"[system]\r\nlanguage=russian\r\n")
        code, output = self.rp_run("--remove-restoration-project")
        self.assertEqual(code, 0, output)
        self.assertEqual((self.root / "mods/mods_order.txt").read_bytes(),
                         original + b"\r\nrpu_russian.dat\r\nother.dat\r\n")
        self.assertEqual((self.root / "mods/rpu_russian.dat").read_bytes(), b"translation")
        self.assertEqual((self.root / "mods/other.dat").read_bytes(), b"another mod")
        self.assertEqual((self.root / "data/new-player-file.txt").read_bytes(), b"player addition")
        self.assertIn(b"language=russian", (self.root / "fallout2.cfg").read_bytes())

    def test_invalid_manifest_is_rejected_before_any_changes(self):
        self.install_rp()
        manifest = self.root / "backup/rpu/f2r-manifest.txt"
        valid = manifest.read_text()
        digest = hashlib.sha256(b"RP added").hexdigest()
        for path in ("../outside", "C:/outside", "/outside", "data/savegame/slot01/save.dat",
                     "a-added.txt", "a-added.txt/child", "mods/../outside", "mods/trailing."):
            with self.subTest(path=path):
                manifest.write_text(valid + f'F - {digest} "{path}"\n', newline="\n")
                before = snapshot(self.root)
                code, output = self.rp_run("--uninstall")
                self.assertNotEqual(code, 0, output)
                self.assertIn("invalid installer manifest", output)
                self.assertEqual(before, snapshot(self.root))
        manifest.write_text(valid, newline="\n")

    def test_duplicate_package_targets_are_rejected_before_mutation(self):
        self.rp["mods_order.txt"] = b"first"
        self.rp["mods/mods_order.txt"] = b"second"
        self.rp_package()
        before = snapshot(self.root)
        code, output = self.rp_run("--restoration-project")
        self.assertNotEqual(code, 0, output)
        self.assertIn("duplicate target", output)
        self.assertEqual(before, snapshot(self.root))

    def test_data_is_a_file_is_rejected_without_mutation(self):
        import shutil
        shutil.rmtree(self.root / "data")
        (self.root / "data").write_bytes(b"player data file")
        before = snapshot(self.root)
        code, output = self.rp_run("--restoration-project")
        self.assertNotEqual(code, 0, output)
        self.assertEqual(before, snapshot(self.root))

    def test_junction_target_is_rejected_before_removal(self):
        import _winapi
        self.install_rp()
        outside = self.base / "outside-mods"
        (self.root / "mods").rename(outside)
        _winapi.CreateJunction(str(outside), str(self.root / "mods"))
        before = snapshot(self.root)
        outside_before = snapshot(outside)
        code, output = self.rp_run("--uninstall")
        self.assertNotEqual(code, 0, output)
        self.assertIn("invalid installer manifest", output)
        self.assertEqual(before, snapshot(self.root))
        self.assertEqual(outside_before, snapshot(outside))

    def test_bad_backup_is_kept_without_any_removal(self):
        self.install_rp()
        (self.root / "backup/rpu/originals/patch000.dat").write_bytes(b"damaged backup")
        before = snapshot(self.root)
        code, output = self.rp_run("--uninstall")
        self.assertNotEqual(code, 0, output)
        self.assertIn("invalid or missing backup", output)
        self.assertEqual(before, snapshot(self.root))

    def test_interrupted_pending_install_recovers_before_rp_marker_skip(self):
        before = snapshot(self.root)
        self.install_rp()
        manifest = self.root / "backup/rpu/f2r-manifest.txt"
        manifest.write_text(manifest.read_text().replace("F2R-RP-1 installed", "F2R-RP-1 pending"), newline="\n")
        # The marker was already copied, but the installed receipt was never committed.
        code, output = self.rp_run("--restoration-project")
        self.assertEqual(code, 0, output)
        self.assertIn("original files restored", output)
        code, output = self.rp_run("--remove-restoration-project")
        self.assertEqual(code, 0, output)
        self.assertEqual(before, snapshot(self.root))

    def test_pending_unknown_contents_are_left_untouched(self):
        self.install_rp()
        manifest = self.root / "backup/rpu/f2r-manifest.txt"
        manifest.write_text(manifest.read_text().replace("F2R-RP-1 installed", "F2R-RP-1 pending"), newline="\n")
        (self.root / "a-added.txt").write_bytes(b"could be a player edit after an interrupted install")
        before = snapshot(self.root)
        for option in ("--remove-restoration-project", "--restoration-project"):
            code, output = self.rp_run(option)
            self.assertNotEqual(code, 0, output)
            self.assertIn("changed by the player", output)
            self.assertEqual(before, snapshot(self.root))

    def test_locked_shared_order_keeps_added_lines_and_retries(self):
        self.rp["mods_order.txt"] = b"rpu.dat\r\n"
        self.rp_package()
        self.install_rp()
        order = self.root / "mods/mods_order.txt"
        desired = b"rpu_russian.dat\r\nother.dat\r\n"
        order.write_bytes(b"rpu.dat\r\n" + desired)
        with read_lock(order):
            code, output = self.rp_run("--remove-restoration-project")
        self.assertNotEqual(code, 0, output)
        self.assertIn("cannot restore mods/mods_order.txt", output)
        self.assertEqual(order.read_bytes(), b"rpu.dat\r\n" + desired)
        self.assertTrue((self.root / "backup/rpu/f2r-manifest.txt").exists())
        code, output = self.rp_run("--remove-restoration-project")
        self.assertEqual(code, 0, output)
        self.assertEqual(order.read_bytes(), desired)

    def test_locked_backup_cleanup_receipt_retries_without_changing_restored_game(self):
        before = snapshot(self.root)
        self.install_rp()
        foreign = self.root / "backup/rpu/player-empty-folder"
        foreign.mkdir()
        backup = self.root / "backup/rpu/originals/patch000.dat"
        with read_lock(backup):
            code, output = self.rp_run("--remove-restoration-project")
        self.assertNotEqual(code, 0, output)
        self.assertIn("restore receipt was kept", output)
        manifest = self.root / "backup/rpu/f2r-manifest.txt"
        self.assertTrue(manifest.read_text().startswith("F2R-RP-1 restored"))
        # Recovery must clean only backup files, even if the player edited the restored game.
        (self.root / "patch000.dat").write_bytes(b"player edit after restore")
        before["patch000.dat"] = hashlib.sha256(b"player edit after restore").hexdigest()
        code, output = self.rp_run("--remove-restoration-project")
        self.assertEqual(code, 0, output)
        self.assertEqual(before, snapshot(self.root))
        self.assertTrue(foreign.is_dir())
        self.assertFalse(manifest.exists())

    def test_created_directory_replaced_by_player_file_is_kept(self):
        self.install_rp()
        (self.root / "data/scripts/gl_ammomod.int").unlink()
        (self.root / "data/scripts").rmdir()
        (self.root / "data/scripts").write_bytes(b"player replaced this directory")
        before = snapshot(self.root)
        code, output = self.rp_run("--uninstall")
        self.assertNotEqual(code, 0, output)
        self.assertEqual(before, snapshot(self.root))

    def test_window_checkbox_removes_only_rp(self):
        import configparser
        self.install_rp()
        settings = self.base / "ui.ini"
        language = os.environ.get("F2R_RP_UI_LANGUAGE", "en")
        settings.write_text(f"[launcher]\nauto_updates=0\nlanguage={language}\n", encoding="utf-8")
        (self.release / "info.json").write_text(json.dumps({"tag_name": "new-build", "body": "Scratch fixture"}))
        startup = subprocess.STARTUPINFO()
        startup.dwFlags = subprocess.STARTF_USESHOWWINDOW
        startup.wShowWindow = 4  # SW_SHOWNOACTIVATE
        env = os.environ.copy()
        env.update(APPDATA=str(self.base / "roaming"), LOCALAPPDATA=str(self.base / "local"))
        process = subprocess.Popen([str(LAUNCHER), "--ui", "--no-activate", "--game-root", str(self.root),
            "--settings-file", str(settings), "--update-url", self.url,
            "--release-info-url", self.url + "/info.json"], env=env,
            startupinfo=startup, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        ui = NativeUi()
        window = None
        try:
            deadline = time.monotonic() + 15
            while time.monotonic() < deadline:
                window = ui.window(process.pid)
                if window:
                    break
                time.sleep(0.05)
            self.assertIsNotNone(window, "scratch launcher did not create its window")
            checkbox = ui.user.GetDlgItem(window, 1005)
            self.assertTrue(ui.user.IsWindowEnabled(checkbox))
            self.assertTrue(ui.checked(window), "installed RP must paint a ticked checkbox")
            before_ce = (self.root / "fallout2-ce.exe").read_bytes()
            ui.user.SendMessageW(checkbox, 0xF5, 0, 0)  # BM_CLICK, no focus change.
            deadline = time.monotonic() + 15
            while time.monotonic() < deadline:
                if not (self.root / "mods/rpu.dat").exists() and ui.user.IsWindowEnabled(checkbox):
                    break
                time.sleep(0.05)
            self.assertFalse((self.root / "mods/rpu.dat").exists())
            self.assertTrue(ui.user.IsWindowEnabled(checkbox))
            self.assertFalse(ui.checked(window), "successful removal must paint an unticked checkbox")
            self.assertEqual((self.root / "fallout2-ce.exe").read_bytes(), before_ce)
            config = configparser.ConfigParser()
            config.read(settings, encoding="utf-16" if settings.read_bytes().startswith(b"\xff\xfe") else "utf-8")
            self.assertEqual(config["launcher"]["rp_wish"], "0")
            evidence = os.environ.get("F2R_RP_UI_EVIDENCE")
            if evidence:
                from PIL import Image
                width, height, data = ui.pixels(window)
                # Crop excludes private scratch paths and the raw log.
                image = Image.frombytes("RGB", (width, height), data, "raw", "BGRX")
                directory = Path(evidence)
                directory.mkdir(parents=True, exist_ok=True)
                image.crop((18, 280, 782, 532)).save(directory / f"rp-removed-{language}.jpg", quality=90)
        finally:
            if window:
                ui.user.PostMessageW(window, 0x10, 0, 0)  # WM_CLOSE to our own test window only.
            try:
                process.wait(timeout=10)
            except subprocess.TimeoutExpired:
                process.kill()
                process.wait()


if __name__ == "__main__":
    import unittest
    unittest.main(verbosity=2)
