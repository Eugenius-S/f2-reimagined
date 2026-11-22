import ctypes
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
                    self.assertEqual(user.SendMessageW(voice, 0x146, 0, 0), 1)
                    for index in range(5):
                        self.assertEqual(user.SendMessageW(text, 0x14e, index, 0), index)  # CB_SETCURSEL
                        user.SendMessageW(window, 0x111, 1004 | (1 << 16), text)  # CBN_SELCHANGE
                    self.assertIn("levy", settings.read_text().lower())
                    user.SendMessageW(user.GetDlgItem(window, 1005), 0xF5, 0, 0)  # BM_CLICK
                    self.assertEqual(user.SendMessageW(text, 0x146, 0, 0), 2)
                    self.assertEqual(user.SendMessageW(voice, 0x146, 0, 0), 1)
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
