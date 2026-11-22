import ctypes
import os
from ctypes import wintypes
import pathlib
import shutil
import subprocess
import sys
import tempfile
import time
import unittest

EXE = sys.argv.pop(1)
user = ctypes.WinDLL("user32", use_last_error=True)
user.GetDlgItem.argtypes = (wintypes.HWND, ctypes.c_int)
user.GetDlgItem.restype = wintypes.HWND
user.SendMessageW.argtypes = (wintypes.HWND, wintypes.UINT, wintypes.WPARAM, wintypes.LPARAM)
user.SendMessageW.restype = wintypes.LPARAM
user.SetWindowTextW.argtypes = (wintypes.HWND, wintypes.LPCWSTR)
user.SetWindowTextW.restype = wintypes.BOOL
user.PostMessageW.argtypes = (wintypes.HWND, wintypes.UINT, wintypes.WPARAM, wintypes.LPARAM)
user.GetWindowThreadProcessId.argtypes = (wintypes.HWND, ctypes.POINTER(wintypes.DWORD))
CALLBACK = ctypes.WINFUNCTYPE(wintypes.BOOL, wintypes.HWND, wintypes.LPARAM)
user.EnumWindows.argtypes = (CALLBACK, wintypes.LPARAM)


class TestUi(unittest.TestCase):
    @unittest.skipUnless(os.environ.get("F2R_VOICE_KNOWN_DAT"), "verified BGforge DAT required")
    def test_manual_voice_visible_and_original_disables_it(self):
        with tempfile.TemporaryDirectory() as temp:
            root = pathlib.Path(temp)
            for name in ("master.dat", "critter.dat"):
                (root / name).write_bytes(b"fixture")
            (root / "fallout2.cfg").write_bytes(b"[system]\r\nlanguage=english\r\n")
            shutil.copyfile(pathlib.Path(EXE).with_name("launcher_routes_tests.exe"), root / "fallout2-ce.exe")
            mods = root / "mods"
            mods.mkdir()
            shutil.copyfile(os.environ["F2R_VOICE_KNOWN_DAT"], mods / "upu_russian_sound.dat")
            order = mods / "mods_order.txt"
            order.write_bytes(b"upu_russian_sound.dat\r\nplayer.dat\r\n")
            proc = subprocess.Popen([EXE, "--ui", "--no-activate", "--game-root", str(root),
                "--settings-file", str(root / "ui.ini"), "--update-url", "http://127.0.0.1:1",
                "--release-info-url", "http://127.0.0.1:1"])
            window = None
            try:
                deadline = time.monotonic() + 20
                while time.monotonic() < deadline and not window:
                    found = []
                    @CALLBACK
                    def enumerate_window(hwnd, _):
                        pid = wintypes.DWORD()
                        user.GetWindowThreadProcessId(hwnd, ctypes.byref(pid))
                        if pid.value == proc.pid and user.GetDlgItem(hwnd, 1017): found.append(hwnd)
                        return True
                    user.EnumWindows(enumerate_window, 0)
                    window = found[0] if found else None
                    if not window: time.sleep(0.05)
                self.assertTrue(window)
                voice = user.GetDlgItem(window, 1017)
                self.assertEqual(user.SendMessageW(voice, 0x147, 0, 0), 1)  # manual is selected
                user.SendMessageW(voice, 0x14e, 0, 0)
                user.SendMessageW(window, 0x111, 1017 | (1 << 16), voice)
                deadline = time.monotonic() + 15
                while time.monotonic() < deadline and b"upu_russian_sound.dat" in order.read_bytes(): time.sleep(0.05)
                self.assertEqual(order.read_bytes(), b"player.dat\r\n")
            finally:
                if window: user.PostMessageW(window, 0x10, 0, 0)
                try: proc.wait(timeout=5)
                except subprocess.TimeoutExpired:
                    proc.terminate()
                    proc.wait(timeout=5)

    def test_text_and_voice_choices_and_rp_filter(self):
        for language in ("en", "ru"):
            with self.subTest(language=language), tempfile.TemporaryDirectory() as temp:
                root = pathlib.Path(temp)
                for name in ("master.dat", "critter.dat"):
                    (root / name).write_bytes(b"fixture")
                settings = root / "ui.ini"
                settings.write_text(f"[launcher]\nlanguage={language}\n")
                proc = subprocess.Popen([EXE, "--ui", "--no-activate", "--game-root", str(root),
                                         "--settings-file", str(settings), "--update-url", "http://127.0.0.1:1",
                                         "--release-info-url", "http://127.0.0.1:1"])
                try:
                    window = None
                    deadline = time.monotonic() + 15
                    while time.monotonic() < deadline and not window:
                        found = []
                        @CALLBACK
                        def enumerate_window(hwnd, _):
                            pid = wintypes.DWORD()
                            user.GetWindowThreadProcessId(hwnd, ctypes.byref(pid))
                            if pid.value == proc.pid and user.GetDlgItem(hwnd, 1004):
                                found.append(hwnd)
                            return True
                        user.EnumWindows(enumerate_window, 0)
                        window = found[0] if found else None
                        if not window:
                            time.sleep(0.05)
                    self.assertTrue(window, "launcher window did not appear")
                    text = user.GetDlgItem(window, 1004)
                    voice = user.GetDlgItem(window, 1017)
                    self.assertEqual(user.SendMessageW(text, 0x146, 0, 0), 5)  # CB_GETCOUNT
                    voice_count = user.SendMessageW(voice, 0x146, 0, 0)
                    self.assertGreaterEqual(voice_count, 1)
                    if voice_count > 1:
                        user.SendMessageW(voice, 0x14e, 1, 0)
                        user.SendMessageW(window, 0x111, 1017 | (1 << 16), voice)
                        self.assertIn("voice_choice", settings.read_text().lower())
                    for index in range(5):
                        self.assertEqual(user.SendMessageW(text, 0x14e, index, 0), index)  # CB_SETCURSEL
                        user.SendMessageW(window, 0x111, 1004 | (1 << 16), text)  # CBN_SELCHANGE
                    self.assertIn("levy", settings.read_text().lower())
                    user.SendMessageW(user.GetDlgItem(window, 1005), 0xF5, 0, 0)  # BM_CLICK
                    self.assertEqual(user.SendMessageW(text, 0x146, 0, 0), 5)
                    self.assertEqual(user.SendMessageW(voice, 0x146, 0, 0), 3)
                    labels = []
                    for index in range(5):
                        buffer = ctypes.create_unicode_buffer(256)
                        user.SendMessageW(text, 0x148, index, ctypes.addressof(buffer))
                        labels.append(buffer.value)
                    self.assertTrue(all(any(expected in label for label in labels) for expected in ('RPU', '1C', 'Fargus', 'Levy')))
                    other = root / "other"
                    (other / "mods").mkdir(parents=True)
                    for name in ("master.dat", "critter.dat"):
                        (other / name).write_bytes(b"fixture")
                    (other / "mods/rpu.dat").write_bytes(b"RP")
                    path = ctypes.create_unicode_buffer(str(other))
                    user.SendMessageW(user.GetDlgItem(window, 1002), 0x000C, 0, ctypes.addressof(path))
                    self.assertEqual(user.SendMessageW(text, 0x146, 0, 0), 2)
                    self.assertTrue((other / "mods/rpu.dat").exists())
                    other2 = root / "recovery"
                    other2.mkdir()
                    for name in ("master.dat", "critter.dat"):
                        (other2 / name).write_bytes(b"fixture")
                    shutil.copyfile(pathlib.Path(EXE).with_name("launcher_routes_tests.exe"), other2 / "fallout2-ce.exe")
                    transaction = other2 / ".f2r-text-transaction"
                    transaction.mkdir()
                    path = ctypes.create_unicode_buffer(str(other2))
                    user.SendMessageW(user.GetDlgItem(window, 1002), 0x000C, 0, ctypes.addressof(path))
                    self.assertEqual(user.SendMessageW(text, 0x146, 0, 0), 5)
                    user.SendMessageW(window, 0x111, 1014, user.GetDlgItem(window, 1014))
                    deadline = time.monotonic() + 10
                    while transaction.exists() and time.monotonic() < deadline:
                        time.sleep(0.05)
                    self.assertFalse(transaction.exists(), "Play did not run transaction recovery")
                finally:
                    if window:
                        user.PostMessageW(window, 0x10, 0, 0)
                    try:
                        proc.wait(timeout=5)
                    except subprocess.TimeoutExpired:
                        proc.terminate()
                        proc.wait(timeout=5)


if __name__ == "__main__":
    unittest.main()
