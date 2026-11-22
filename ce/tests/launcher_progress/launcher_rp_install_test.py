"""RP operations use only synthetic games, local packages and real filesystem failures."""
import hashlib
import http.server
import json
import os
import shutil
import time
import ctypes
from ctypes import wintypes
from pathlib import Path
import subprocess
import sys
import threading
import zipfile
import unittest

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

    def text(self, hwnd):
        length = self.user.SendMessageW(hwnd, 0xE, 0, 0)
        buffer = ctypes.create_unicode_buffer(length + 1)
        self.user.SendMessageW(hwnd, 0xD, len(buffer), ctypes.addressof(buffer))
        return buffer.value

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
            "--rp-zip-url", self.url + "/rp.zip", *args], capture_output=True, timeout=90,
            env={**os.environ, **getattr(self, "launcher_env", {})})
        output = (result.stdout + result.stderr).decode("utf-8", errors="replace")
        print(f"\n{self.id()}: exit {result.returncode}\n{output}", flush=True)
        return result.returncode, output

    @unittest.skipUnless(os.environ.get('F2R_RP_TEXT_PACKS'), 'external pinned RP and text DATs required')
    def test_real_rp_round_trip_retains_text(self):
        packs = Path(os.environ['F2R_RP_TEXT_PACKS'])
        mods = self.root / 'mods'
        mods.mkdir(exist_ok=True)
        for name in ('steam-1c-text.dat', 'shared-legacy-fonts.dat'):
            shutil.copyfile(packs / 'prepared-v4' / name, mods / name)
        shutil.copyfile(packs / 'proof/rpu-2.4.34/mods/rpu_russian.dat', mods / 'rpu_russian.dat')
        (mods / 'f2r-text-choice.txt').write_text('1c\nretired=levy\n')
        (mods / 'player.dat').write_bytes(b'player')
        (mods / 'mods_order.txt').write_bytes(b'steam-1c-text.dat\r\nshared-legacy-fonts.dat\r\nplayer.dat\r\n')
        (self.root / 'fallout2.cfg').write_bytes(b'[system]\r\nlanguage=russian\r\n')
        save = self.root / 'data/savegame/slot01/save.dat'
        before_save = save.read_bytes()
        with zipfile.ZipFile(self.release / 'rp.zip', 'w', zipfile.ZIP_STORED) as package:
            package.write(packs / 'proof/rpu-2.4.34/mods/rpu.dat', 'mods/rpu.dat')
            package.writestr('mods_order.txt', b'rpu.dat\r\n')
        code, output = self.rp_run('--restoration-project')
        self.assertEqual(code, 0, output)
        self.assertEqual(hashlib.sha256((mods / 'steam-1c-text.dat').read_bytes()).hexdigest(),
                         'cecbec9903172351fd4387475cd9f87eab3cd4b2b0836991a5640373ea46de51')
        self.assertEqual((mods / 'f2r-text-choice.txt').read_text().splitlines()[0], '1c')
        order = (mods / 'mods_order.txt').read_text().splitlines()
        self.assertLess(order.index('rpu.dat'), order.index('rpu_russian.dat'))
        self.assertLess(order.index('rpu_russian.dat'), order.index('steam-1c-text.dat'))
        code, output = self.rp_run('--remove-restoration-project')
        self.assertEqual(code, 0, output)
        self.assertEqual(hashlib.sha256((mods / 'steam-1c-text.dat').read_bytes()).hexdigest(),
                         'a89666c64983553b55d042b8a24b46f07ebc8ab782e7e6b1514d902a71d9d5f6')
        self.assertEqual((mods / 'f2r-text-choice.txt').read_text().splitlines()[0], '1c')
        self.assertEqual(save.read_bytes(), before_save)
        self.assertEqual((mods / 'player.dat').read_bytes(), b'player')

    @unittest.skipUnless(os.environ.get("F2R_VOICE_FIXTURE"), "fixture-configured voice launcher required")
    def test_voice_preflight_bad_hash_preserves_original_and_retry(self):
        class BadVoice(http.server.BaseHTTPRequestHandler):
            def do_GET(self):
                self.send_response(200)
                self.end_headers()
                self.wfile.write(b"x" * 22)
            def log_message(self, *args):
                pass
        server = http.server.ThreadingHTTPServer(("127.0.0.1", 18751), BadVoice)
        threading.Thread(target=server.serve_forever, daemon=True).start()
        self.addCleanup(server.server_close)
        self.addCleanup(server.shutdown)
        mods = self.root / "mods"
        mods.mkdir(exist_ok=True)
        voice = mods / "f2r-voice-vanilla-1c.dat"
        voice.write_bytes(b"fixture voice package\n")
        (mods / "f2r-voice-choice.txt").write_text("vanilla-1c\nhash=16b874f8ebf84d286c4684a0a7a78c0cd00c84c35515a0eee9d889746537f2c1\n")
        order = b"f2r-voice-vanilla-1c.dat\r\nplayer.dat\r\n"
        (mods / "mods_order.txt").write_bytes(order)
        for _ in range(2):
            code, output = self.rp_run("--restoration-project", "--voice-translation", "rp-1c", "--text-with-install")
            self.assertNotEqual(code, 0, output)
            self.assertIn("SHA-256", output)
            self.assertEqual(voice.read_bytes(), b"fixture voice package\n")
            self.assertEqual((mods / "mods_order.txt").read_bytes(), order)
            self.assertFalse((mods / "rpu.dat").exists())
            self.assertFalse((self.root / ".f2r-voice-rp-transition").exists())

    @unittest.skipUnless(os.environ.get("F2R_VOICE_FIXTURE") and os.environ.get("F2R_VOICE_REAL_RP"), "fixture launcher and real RP required")
    def test_voice_transition_and_rp_removal_with_real_rp(self):
        class Voice(http.server.BaseHTTPRequestHandler):
            def do_GET(self):
                self.send_response(200)
                self.end_headers()
                self.wfile.write(b"fixture voice package\n")
            def log_message(self, *args):
                pass
        server = http.server.ThreadingHTTPServer(("127.0.0.1", 18751), Voice)
        threading.Thread(target=server.serve_forever, daemon=True).start()
        self.addCleanup(server.server_close)
        self.addCleanup(server.shutdown)
        mods = self.root / "mods"
        mods.mkdir(exist_ok=True)
        old = mods / "f2r-voice-vanilla-1c.dat"
        old.write_bytes(b"fixture voice package\n")
        (mods / "f2r-voice-choice.txt").write_text("vanilla-1c\nhash=16b874f8ebf84d286c4684a0a7a78c0cd00c84c35515a0eee9d889746537f2c1\n")
        (mods / "mods_order.txt").write_bytes(b"f2r-voice-vanilla-1c.dat\r\nplayer.dat\r\n")
        (self.root / "fallout2.cfg").write_bytes(b"[system]\r\nlanguage=english\r\n")
        with zipfile.ZipFile(self.release / "rp.zip", "w", zipfile.ZIP_STORED) as package:
            package.write(os.environ["F2R_VOICE_REAL_RP"], "mods/rpu.dat")
            package.writestr("mods_order.txt", b"rpu.dat\r\n")
        (self.release / "rpu_russian.dat").write_bytes(b"corrupt translation")
        (self.release / "rpu-version.txt").write_text("test")
        code, output = self.rp_run("--restoration-project", "--text-translation", "official", "--voice-translation", "rp-1c", "--text-with-install")
        self.assertNotEqual(code, 0, output)
        self.assertIn("downloaded 19 bytes", output)
        self.assertEqual(old.read_bytes(), b"fixture voice package\n")
        self.assertEqual((mods / "mods_order.txt").read_bytes(), b"f2r-voice-vanilla-1c.dat\r\nplayer.dat\r\n")
        self.assertFalse((mods / "rpu.dat").exists())
        self.assertFalse((mods / "f2r-voice-rp-1c.dat").exists())
        self.assertFalse((self.root / ".f2r-voice-rp-transition").exists())
        code, output = self.rp_run("--restoration-project", "--voice-translation", "rp-1c", "--text-with-install")
        self.assertEqual(code, 0, output)
        self.assertFalse(old.exists())
        self.assertTrue((mods / "f2r-voice-rp-1c.dat").exists())
        self.assertFalse((self.root / ".f2r-voice-rp-transition").exists())
        with (mods / "rpu.dat").open("r+b") as f:
            f.write(b"X")
        code, output = self.rp_run("--remove-restoration-project")
        self.assertNotEqual(code, 0, output)
        self.assertTrue((mods / "f2r-voice-rp-1c.dat").exists())
        self.assertIn(b"f2r-voice-rp-1c.dat", (mods / "mods_order.txt").read_bytes())
        self.assertFalse((self.root / ".f2r-voice-rp-transition").exists())
        import shutil
        shutil.copyfile(os.environ["F2R_VOICE_REAL_RP"], mods / "rpu.dat")
        code, output = self.rp_run("--remove-restoration-project")
        self.assertEqual(code, 0, output)
        order = (mods / "mods_order.txt").read_bytes()
        self.assertNotIn(b"f2r-voice-vanilla-1c.dat", order)
        self.assertNotIn(b"f2r-voice-rp-1c.dat", order)
        self.assertIn(b"player.dat", order)

    @unittest.skipUnless(os.environ.get("F2R_VOICE_KNOWN_DAT"), "verified BGforge DAT required")
    def test_manual_voice_stays_suppressed_through_rp_removal(self):
        import shutil
        mods = self.root / "mods"
        mods.mkdir(exist_ok=True)
        shutil.copyfile(os.environ["F2R_VOICE_KNOWN_DAT"], mods / "upu_russian_sound.dat")
        original = b"# upu_russian_sound.dat\r\nupu_russian_sound.dat\r\nplayer.dat\r\n"
        (mods / "mods_order.txt").write_bytes(original)
        code, output = self.rp_run("--restoration-project")
        self.assertEqual(code, 0, output)
        code, output = self.rp_run("--voice-translation", "original")
        self.assertEqual(code, 0, output)
        code, output = self.rp_run("--remove-restoration-project")
        self.assertEqual(code, 0, output)
        self.assertEqual((mods / "mods_order.txt").read_bytes(), b"# upu_russian_sound.dat\r\nplayer.dat\r\n")
        code, output = self.rp_run("--uninstall")
        self.assertEqual(code, 0, output)
        self.assertEqual((mods / "mods_order.txt").read_bytes(), original)

    @unittest.skipUnless(os.environ.get("F2R_TRANSLATION_FIXTURES"), "external verified translation DATs not supplied")
    def test_unsupported_rp_install_preserves_translation_and_user_mod(self):
        packs = Path(os.environ["F2R_TRANSLATION_FIXTURES"])
        mods = self.root / "mods"
        mods.mkdir(exist_ok=True)
        for name in ("steam-1c-text.dat", "shared-legacy-fonts.dat"):
            (mods / name).write_bytes((packs / name).read_bytes())
        (mods / "f2r-text-choice.txt").write_text("1c\n")
        (mods / "player.dat").write_bytes(b"player mod")
        (mods / "mods_order.txt").write_bytes(b"steam-1c-text.dat\r\nshared-legacy-fonts.dat\r\nplayer.dat\r\n")
        (self.root / "fallout2.cfg").write_bytes(b"[system]\r\nlanguage=russian\r\n")
        self.rp["mods_order.txt"] = b"rpu.dat\r\n"
        self.rp_package()
        before = snapshot(self.root)
        code, output = self.rp_run("--restoration-project")
        self.assertNotEqual(code, 0, output)
        self.assertEqual(snapshot(self.root), before)

    @unittest.skipUnless(os.environ.get("F2R_TRANSLATION_FIXTURES"), "external verified translation DATs not supplied")
    def test_rp_install_rolls_back_if_translation_cannot_be_switched(self):
        packs = Path(os.environ["F2R_TRANSLATION_FIXTURES"])
        mods = self.root / "mods"
        mods.mkdir(exist_ok=True)
        for name in ("steam-1c-text.dat", "shared-legacy-fonts.dat"):
            (mods / name).write_bytes((packs / name).read_bytes())
        (mods / "f2r-text-choice.txt").write_text("1c\n")
        (mods / "mods_order.txt").write_bytes(b"steam-1c-text.dat\r\nshared-legacy-fonts.dat\r\nplayer.dat\r\n")
        (self.root / "fallout2.cfg").write_bytes(b"[system]\r\nlanguage=russian\r\n")
        self.rp["mods_order.txt"] = b"rpu.dat\r\n"
        self.rp_package()
        before = snapshot(self.root)
        with read_lock(self.root / "fallout2.cfg"):
            code, output = self.rp_run("--restoration-project")
        self.assertNotEqual(code, 0, output)
        self.assertEqual(snapshot(self.root), before)

    def test_rp_order_keeps_unrelated_mods_and_official_after_rpu(self):
        mods = self.root / "mods"
        mods.mkdir(exist_ok=True)
        for name in ("rpu_russian.dat", "player.dat"):
            (mods / name).write_bytes(b"user file")
        (mods / "mods_order.txt").write_bytes(b"rpu_russian.dat\r\nplayer.dat\r\n")
        self.rp["mods_order.txt"] = b"rpu.dat\r\n"
        self.rp_package()
        code, output = self.rp_run("--restoration-project")
        self.assertEqual(code, 0, output)
        lines = (mods / "mods_order.txt").read_text().splitlines()
        self.assertLess(lines.index("rpu.dat"), lines.index("rpu_russian.dat"))
        self.assertIn("player.dat", lines)
        code, output = self.rp_run("--remove-restoration-project")
        self.assertEqual(code, 0, output)
        self.assertEqual((mods / "mods_order.txt").read_bytes(), b"rpu_russian.dat\r\nplayer.dat\r\n")

    @unittest.skipUnless(os.environ.get("F2R_TRANSLATION_FIXTURES"), "external verified translation DATs not supplied")
    def test_unsupported_rp_install_preserves_order_without_other_mods(self):
        packs = Path(os.environ["F2R_TRANSLATION_FIXTURES"])
        mods = self.root / "mods"
        mods.mkdir(exist_ok=True)
        for name in ("steam-1c-text.dat", "shared-legacy-fonts.dat"):
            (mods / name).write_bytes((packs / name).read_bytes())
        (mods / "f2r-text-choice.txt").write_text("1c\n")
        (mods / "mods_order.txt").write_bytes(b"steam-1c-text.dat\r\nshared-legacy-fonts.dat\r\n")
        (self.root / "fallout2.cfg").write_bytes(b"[system]\r\nlanguage=russian\r\n")
        self.rp["mods_order.txt"] = b"rpu.dat\r\n"
        self.rp_package()
        before = snapshot(self.root)
        code, output = self.rp_run("--restoration-project")
        self.assertNotEqual(code, 0, output)
        self.assertEqual(snapshot(self.root), before)

    @unittest.skipUnless(os.environ.get("F2R_OFFICIAL_FIXTURE"), "external verified official DAT not supplied")
    def test_official_cli_and_rp_suppression_for_manual_and_owned(self):
        data = Path(os.environ["F2R_OFFICIAL_FIXTURE"]).read_bytes()
        (self.release / "rpu_russian.dat").write_bytes(data)
        (self.release / "rpu-version.txt").write_bytes(b"2.4.34\n")
        mods = self.root / "mods"
        mods.mkdir(exist_ok=True)
        (self.root / "fallout2.cfg").write_bytes(b"[system]\r\nlanguage=english\r\n")
        (mods / "rpu_russian.dat").write_bytes(data)
        (mods / "mods_order.txt").write_bytes(b"rpu_russian.dat\r\nplayer.dat\r\n")
        (mods / "player.dat").write_bytes(b"player mod")
        self.rp["mods_order.txt"] = b"rpu.dat\r\n"
        self.rp_package()
        for manual in (True, False):
            if not manual:
                (mods / "rpu_russian.dat").unlink()
            code, output = self.rp_run("--russian")
            self.assertEqual(code, 0, output)
            code, output = self.rp_run("--restoration-project")
            self.assertEqual(code, 0, output)
            code, output = self.rp_run("--remove-russian")
            self.assertEqual(code, 0, output)
            self.assertEqual((mods / "rpu_russian.dat").exists(), manual)
            if manual:
                self.assertEqual((mods / "rpu_russian.dat").read_bytes(), data)
            code, output = self.rp_run("--remove-restoration-project")
            self.assertEqual(code, 0, output)
            order = (mods / "mods_order.txt").read_bytes()
            self.assertNotIn(b"rpu_russian.dat", order)
            self.assertIn(b"player.dat", order)

    @unittest.skipUnless(os.environ.get("F2R_OFFICIAL_FIXTURE"), "external verified official DAT not supplied")
    def test_reselect_official_clears_rp_suppression(self):
        data = Path(os.environ["F2R_OFFICIAL_FIXTURE"]).read_bytes()
        mods = self.root / "mods"
        mods.mkdir(exist_ok=True)
        (mods / "rpu_russian.dat").write_bytes(data)
        (mods / "mods_order.txt").write_bytes(b"rpu_russian.dat\r\nplayer.dat\r\n")
        (self.root / "fallout2.cfg").write_bytes(b"[system]\r\nlanguage=russian\r\n")
        self.rp["mods_order.txt"] = b"rpu.dat\r\n"
        self.rp_package()
        for args in (("--restoration-project",), ("--remove-russian",), ("--text-translation", "official"),
                     ("--remove-restoration-project",)):
            code, output = self.rp_run(*args)
            self.assertEqual(code, 0, output)
        self.assertIn(b"rpu_russian.dat", (mods / "mods_order.txt").read_bytes())
        self.assertEqual((mods / "rpu_russian.dat").read_bytes(), data)

    def test_rp_preserves_automatically_discovered_mods(self):
        mods = self.root / "mods"
        mods.mkdir(exist_ok=True)
        (mods / "player.dat").write_bytes(b"player mod")
        self.rp["mods_order.txt"] = b"rpu.dat\r\n"
        self.rp_package()
        code, output = self.rp_run("--restoration-project")
        self.assertEqual(code, 0, output)
        self.assertIn(b"player.dat", (mods / "mods_order.txt").read_bytes())
        code, output = self.rp_run("--remove-restoration-project")
        self.assertEqual(code, 0, output)
        self.assertIn(b"player.dat", (mods / "mods_order.txt").read_bytes())

    def test_failed_rp_install_restores_absent_autodiscovered_order(self):
        mods = self.root / "mods"
        mods.mkdir(exist_ok=True)
        (mods / "player.dat").write_bytes(b"player mod")
        (self.root / "zz-blocked").write_bytes(b"not a directory")
        self.rp["mods_order.txt"] = b"rpu.dat\r\n"
        self.rp["zz-blocked/file.txt"] = b"failure"
        self.rp_package()
        before = snapshot(self.root)
        code, output = self.rp_run("--restoration-project")
        self.assertNotEqual(code, 0, output)
        self.assertEqual(snapshot(self.root), before)

    @unittest.skipUnless(os.environ.get("F2R_TRANSLATION_FIXTURES"), "external verified translation DATs not supplied")
    def test_rp_installed_with_interrupted_text_cleanup_recovers_before_play(self):
        packs = Path(os.environ["F2R_TRANSLATION_FIXTURES"])
        code, output = self.rp_run("--restoration-project")
        self.assertEqual(code, 0, output)
        mods = self.root / "mods"
        for name in ("steam-1c-text.dat", "shared-legacy-fonts.dat"):
            (mods / name).write_bytes((packs / name).read_bytes())
        (mods / "f2r-text-choice.txt").write_bytes(b"1c\n")
        (self.root / "fallout2.cfg").write_bytes(b"[system]\r\nlanguage=russian\r\n")
        code, output = self.rp_run("--recover-text")
        self.assertEqual(code, 0, output)
        self.assertFalse((mods / "steam-1c-text.dat").exists())
        self.assertFalse((mods / "shared-legacy-fonts.dat").exists())
        self.assertTrue((mods / "rpu.dat").exists())

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

    def test_console_cannot_launch_with_pending_or_invalid_rp_receipt(self):
        self.install_rp()
        manifest = self.root / "backup/rpu/f2r-manifest.txt"
        valid = manifest.read_bytes()
        rp = self.root / "mods/rpu.dat"
        rp_bytes = rp.read_bytes()
        probe = self.base / "game-probe.log"
        (self.root / "fallout2-ce.exe").write_bytes(LAUNCHER.with_name("launcher_rp_exporter_stub.exe").read_bytes())
        (self.root / "Fallout 2 Reimagined.exe.old").write_bytes(b"keep")
        for state, present in (("installing", False), ("installing", True), ("pending", True),
                               ("invalid", True), ("installed", False), ("installed", "directory")):
            with self.subTest(state=state, rp_present=present):
                if rp.is_dir():
                    rp.rmdir()
                if present == "directory":
                    rp.unlink(missing_ok=True)
                    rp.mkdir()
                elif present:
                    rp.write_bytes(rp_bytes)
                else:
                    rp.unlink(missing_ok=True)
                manifest.write_bytes(b"invalid receipt\n" if state == "invalid" else
                    valid.replace(b"F2R-RP-1 installed", f"F2R-RP-1 {state}".encode()))
                before = snapshot(self.root)
                command = [str(LAUNCHER), "--game-root", str(self.root), "--update-url", self.url,
                           "--mirror-url", self.url]
                result = subprocess.run(command, capture_output=True, timeout=15,
                    env={**os.environ, "F2R_RP_EXPORT_LOG": str(probe)})
                self.assertNotEqual(result.returncode, 0)
                self.assertIn(b"finish Restoration Project recovery", result.stdout)
                self.assertFalse(probe.exists())
                self.assertEqual(snapshot(self.root), before)
                if present == "directory":
                    self.assertTrue(rp.is_dir())
        manifest.unlink()
        manifest.mkdir()
        before = snapshot(self.root)
        result = subprocess.run(command, capture_output=True, timeout=15,
            env={**os.environ, "F2R_RP_EXPORT_LOG": str(probe)})
        self.assertNotEqual(result.returncode, 0)
        self.assertIn(b"finish Restoration Project recovery", result.stdout)
        self.assertFalse(probe.exists())
        self.assertTrue(manifest.is_dir())
        self.assertEqual(snapshot(self.root), before)

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

    def test_changed_backup_after_failed_cleanup_is_kept(self):
        self.install_rp()
        backup = self.root / "backup/rpu/originals/patch000.dat"
        with read_lock(backup):
            code, output = self.rp_run("--remove-restoration-project")
        self.assertNotEqual(code, 0, output)
        backup.write_bytes(b"player changed the remaining backup after restore")
        before = snapshot(self.root)
        code, output = self.rp_run("--remove-restoration-project")
        self.assertNotEqual(code, 0, output)
        self.assertIn("invalid or missing backup", output)
        self.assertEqual(before, snapshot(self.root))

    def test_created_directory_replaced_by_player_file_is_kept(self):
        self.install_rp()
        (self.root / "data/scripts/gl_ammomod.int").unlink()
        (self.root / "data/scripts").rmdir()
        (self.root / "data/scripts").write_bytes(b"player replaced this directory")
        before = snapshot(self.root)
        code, output = self.rp_run("--uninstall")
        self.assertNotEqual(code, 0, output)
        self.assertEqual(before, snapshot(self.root))

    def use_logging_exporter(self, fail=False):
        self.export_log = self.base / "exporter-calls.txt"
        self.export_fail = self.base / "exporter-failure.txt"
        self.export_fail.write_text("1" if fail else "0")
        self.launcher_env = {"F2R_RP_EXPORT_LOG": str(self.export_log), "F2R_RP_EXPORT_FAIL_FILE": str(self.export_fail)}
        (self.root / "scene-exporter.exe").write_bytes(LAUNCHER.with_name("launcher_rp_exporter_stub.exe").read_bytes())
        self.cache = self.root / "mod-cache/scene/fixture-scene.txt"
        self.cache.parent.mkdir(parents=True, exist_ok=True)
        self.cache.write_text("RP")

    def test_install_breaks_hardlink_without_changing_external_bytes(self):
        outside = self.base / "outside.ini"
        original = b"external file shared with the original game config"
        outside.write_bytes(original)
        destination = self.root / "ddraw.ini"
        destination.unlink()
        os.link(outside, destination)
        before = snapshot(self.root)
        self.install_rp()
        self.assertEqual(outside.read_bytes(), original)
        self.assertEqual(destination.read_bytes(), b"RP settings")
        code, output = self.rp_run("--remove-restoration-project")
        self.assertEqual(code, 0, output)
        self.assertEqual(outside.read_bytes(), original)
        self.assertEqual(before, snapshot(self.root))

    def test_rp_only_removal_refreshes_actual_vanilla_scenes(self):
        self.install_rp()
        self.use_logging_exporter()
        code, output = self.rp_run("--remove-restoration-project")
        self.assertEqual(code, 0, output)
        calls = self.export_log.read_text().splitlines()
        self.assertEqual(calls, ["--game-root", str(self.root), "--maps", "all", "END"])
        self.assertEqual(self.cache.read_text(), "vanilla")
        self.assertFalse((self.root / "backup/rpu/f2r-manifest.txt").exists())

    def test_failed_vanilla_refresh_retains_receipt_and_retries(self):
        self.install_rp()
        self.use_logging_exporter(fail=True)
        code, output = self.rp_run("--remove-restoration-project")
        self.assertNotEqual(code, 0, output)
        self.assertIn("vanilla 3D scenes could not be refreshed", output)
        self.assertEqual(self.cache.read_text(), "RP")
        receipt = self.root / "backup/rpu/f2r-manifest.txt"
        self.assertTrue(receipt.read_text().startswith("F2R-RP-1 restored"))
        self.assertFalse((self.root / "mods/rpu.dat").exists())
        (self.root / "patch000.dat").write_bytes(b"player edit between retries")
        self.export_fail.write_text("0")
        code, output = self.rp_run("--remove-restoration-project")
        self.assertEqual(code, 0, output)
        self.assertEqual(self.export_log.read_text().count("END"), 2)
        self.assertEqual(self.cache.read_text(), "vanilla")
        self.assertEqual((self.root / "patch000.dat").read_bytes(), b"player edit between retries")
        self.assertFalse(receipt.exists())

    def test_missing_exporter_keeps_removal_pending(self):
        self.install_rp()
        (self.root / "scene-exporter.exe").unlink()
        code, output = self.rp_run("--remove-restoration-project")
        self.assertNotEqual(code, 0, output)
        self.assertIn("vanilla 3D scenes could not be refreshed", output)
        self.assertTrue((self.root / "backup/rpu/f2r-manifest.txt").exists())

    def test_window_checkbox_removes_only_rp(self):
        self.window_removal()

    def test_window_failed_refresh_disables_play_and_retries(self):
        self.window_removal(fail_refresh=True)

    def test_window_partial_removal_blocks_play_and_recovers(self):
        self.window_removal(partial_removal=True)

    def window_removal(self, fail_refresh=False, partial_removal=False):
        import configparser
        self.install_rp()
        if fail_refresh:
            self.use_logging_exporter(fail=True)
        if partial_removal:
            with read_lock(self.root / "mods/rpu.dat"):
                code, output = self.rp_run("--remove-restoration-project")
            self.assertNotEqual(code, 0, output)
            self.assertTrue((self.root / "mods/rpu.dat").exists())
            self.assertTrue((self.root / "backup/rpu/f2r-manifest.txt").read_text().startswith("F2R-RP-1 pending"))
        settings = self.base / "ui.ini"
        language = os.environ.get("F2R_RP_UI_LANGUAGE", "en")
        settings.write_text(f"[launcher]\nauto_updates={int(partial_removal)}\nlanguage={language}\n", encoding="utf-8")
        (self.release / "info.json").write_text(json.dumps({"tag_name": "new-build", "body": "Scratch fixture"}))
        startup = subprocess.STARTUPINFO()
        startup.dwFlags = subprocess.STARTF_USESHOWWINDOW
        startup.wShowWindow = 4  # SW_SHOWNOACTIVATE
        env = {**os.environ, **getattr(self, "launcher_env", {})}
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
            # HWND creation precedes the initial disk-state sync and local release reply.
            deadline = time.monotonic() + 15
            while time.monotonic() < deadline and "new-build" not in ui.text(ui.user.GetDlgItem(window, 1009)):
                time.sleep(0.05)
            self.assertIn("new-build", ui.text(ui.user.GetDlgItem(window, 1009)), "scratch launcher did not finish initialization")
            checkbox = ui.user.GetDlgItem(window, 1005)
            self.assertTrue(ui.user.IsWindowEnabled(checkbox))
            self.assertTrue(ui.checked(window), "installed RP must paint a ticked checkbox")
            if partial_removal:
                self.assertTrue(ui.user.IsWindowEnabled(checkbox), "auto-update must leave pending recovery for the removal retry")
                self.assertFalse(ui.user.IsWindowEnabled(ui.user.GetDlgItem(window, 1014)), "a pending partial restore must block Play even with rpu.dat still present")
            before_ce = (self.root / "fallout2-ce.exe").read_bytes()
            ui.user.SendMessageW(checkbox, 0xF5, 0, 0)  # BM_CLICK, no focus change.
            deadline = time.monotonic() + 15
            while time.monotonic() < deadline:
                if not (self.root / "mods/rpu.dat").exists() and ui.user.IsWindowEnabled(checkbox):
                    break
                time.sleep(0.05)
            self.assertFalse((self.root / "mods/rpu.dat").exists())
            self.assertTrue(ui.user.IsWindowEnabled(checkbox))
            if fail_refresh:
                self.assertTrue(ui.checked(window), "pending removal must stay ticked for retry")
                self.assertFalse(ui.user.IsWindowEnabled(ui.user.GetDlgItem(window, 1014)), "Play must stay disabled with stale RP scenes")
                self.assertEqual(self.cache.read_text(), "RP")
                self.export_fail.write_text("0")
                ui.user.SendMessageW(checkbox, 0xF5, 0, 0)
                deadline = time.monotonic() + 15
                while time.monotonic() < deadline:
                    if not (self.root / "backup/rpu/f2r-manifest.txt").exists() and ui.user.IsWindowEnabled(checkbox):
                        break
                    time.sleep(0.05)
                self.assertEqual(self.cache.read_text(), "vanilla")
                self.assertEqual(self.export_log.read_text().count("END"), 2)
            self.assertFalse(ui.checked(window), "successful removal must paint an unticked checkbox")
            self.assertTrue(ui.user.IsWindowEnabled(ui.user.GetDlgItem(window, 1014)), "Play is enabled only after refresh succeeds")
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
