import ctypes
import hashlib
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
user.IsWindowEnabled.argtypes = (wintypes.HWND,)
user.IsWindowEnabled.restype = wintypes.BOOL
user.PostMessageW.argtypes = (wintypes.HWND, wintypes.UINT, wintypes.WPARAM, wintypes.LPARAM)
user.GetWindowThreadProcessId.argtypes = (wintypes.HWND, ctypes.POINTER(wintypes.DWORD))
CALLBACK = ctypes.WINFUNCTYPE(wintypes.BOOL, wintypes.HWND, wintypes.LPARAM)
user.EnumWindows.argtypes = (CALLBACK, wintypes.LPARAM)


class TestUi(unittest.TestCase):
    @unittest.skipUnless(os.environ.get('F2R_RP_TEXT_PACKS'), 'external pinned RP and voice DATs required')
    def test_play_recovers_excluded_rp_voice(self):
        packs = pathlib.Path(os.environ['F2R_RP_TEXT_PACKS'])
        with tempfile.TemporaryDirectory(prefix='f2r-rp-voice-ui-') as temp:
            root = pathlib.Path(temp)
            mods = root / 'mods'
            mods.mkdir()
            for name in ('master.dat', 'critter.dat'):
                (root / name).write_bytes(b'fixture')
            (root / 'fallout2.cfg').write_bytes(b'[system]\r\nlanguage=english\r\n')
            shutil.copyfile(pathlib.Path(EXE).with_name('launcher_routes_tests.exe'), root / 'fallout2-ce.exe')
            shutil.copyfile(packs / 'proof/rpu-2.4.34/mods/rpu.dat', mods / 'rpu.dat')
            voice = mods / 'f2r-voice-rp-1c.dat'
            shutil.copyfile(packs / 'for-google-drive-voice/1c-voice.dat', voice)
            digest = hashlib.sha256(voice.read_bytes()).hexdigest()
            receipt = mods / 'f2r-voice-choice.txt'
            receipt.write_text('rp-1c\nhash=' + digest + '\n')
            (mods / 'f2r-text-choice.txt').write_text('original\n')
            order = mods / 'mods_order.txt'
            order.write_bytes(voice.name.encode() + b'\r\nplayer.dat\r\n')
            settings = root / 'ui.ini'
            settings.write_text('[launcher]\nlanguage=en\nauto_updates=0\n')
            proc = subprocess.Popen([EXE, '--ui', '--no-activate', '--game-root', str(root),
                                     '--settings-file', str(settings), '--update-url', 'http://127.0.0.1:1',
                                     '--release-info-url', 'http://127.0.0.1:1'], creationflags=subprocess.CREATE_NO_WINDOW)
            window = None
            try:
                deadline = time.monotonic() + 20
                while time.monotonic() < deadline and not window:
                    found = []
                    @CALLBACK
                    def enumerate_window(hwnd, _):
                        pid = wintypes.DWORD()
                        user.GetWindowThreadProcessId(hwnd, ctypes.byref(pid))
                        if pid.value == proc.pid and user.GetDlgItem(hwnd, 1014): found.append(hwnd)
                        return True
                    user.EnumWindows(enumerate_window, 0)
                    window = found[0] if found else None
                    if not window: time.sleep(0.05)
                self.assertTrue(window)
                user.SendMessageW(window, 0x111, 1014, user.GetDlgItem(window, 1014))
                deadline = time.monotonic() + 30
                while time.monotonic() < deadline and receipt.read_text().splitlines()[0] != 'original': time.sleep(0.05)
                self.assertEqual(receipt.read_text().strip(), 'original')
                self.assertFalse(voice.exists())
                self.assertEqual(order.read_bytes(), b'player.dat\r\n')
                deadline = time.monotonic() + 20
                while time.monotonic() < deadline and not user.IsWindowEnabled(user.GetDlgItem(window, 1014)): time.sleep(0.05)
                self.assertTrue(user.IsWindowEnabled(user.GetDlgItem(window, 1014)))
                user.SendMessageW(window, 0x111, 1014, user.GetDlgItem(window, 1014))
                status = ctypes.create_unicode_buffer(512)
                user.SendMessageW(user.GetDlgItem(window, 1008), 0xD, len(status), ctypes.addressof(status))
                self.assertIn('running', status.value.lower())
            finally:
                if window: user.PostMessageW(window, 0x10, 0, 0)
                try: proc.wait(timeout=5)
                except subprocess.TimeoutExpired:
                    proc.terminate()
                    proc.wait(timeout=5)
                deadline = time.monotonic() + 20
                while (mods / 'rpu.dat').exists():
                    try:
                        (mods / 'rpu.dat').unlink()
                    except PermissionError:
                        if time.monotonic() >= deadline: raise
                        time.sleep(0.1)

    @unittest.skipUnless(os.environ.get('F2R_RP_TEXT_PACKS'), 'external pinned RP and text DATs required')
    def test_play_with_adapted_rp_text(self):
        packs = pathlib.Path(os.environ['F2R_RP_TEXT_PACKS'])
        with tempfile.TemporaryDirectory(prefix='f2r-rp-ui-') as temp:
            root = pathlib.Path(temp)
            mods = root / 'mods'
            mods.mkdir()
            for name in ('master.dat', 'critter.dat'):
                (root / name).write_bytes(b'fixture')
            (root / 'fallout2.cfg').write_bytes(b'[system]\r\nlanguage=russian\r\n')
            shutil.copyfile(pathlib.Path(EXE).with_name('launcher_routes_tests.exe'), root / 'fallout2-ce.exe')
            shutil.copyfile(packs / 'proof/rpu-2.4.34/mods/rpu.dat', mods / 'rpu.dat')
            shutil.copyfile(packs / 'for-google-drive-rp-text/rp-1c-text.dat', mods / 'steam-1c-text.dat')
            (mods / 'f2r-text-choice.txt').write_text('1c\n')
            (mods / 'mods_order.txt').write_bytes(b'rpu.dat\r\nsteam-1c-text.dat\r\n')
            settings = root / 'ui.ini'
            settings.write_text('[launcher]\nlanguage=en\nauto_updates=0\n')
            proc = subprocess.Popen([EXE, '--ui', '--no-activate', '--game-root', str(root),
                                     '--settings-file', str(settings), '--update-url', 'http://127.0.0.1:1',
                                     '--release-info-url', 'http://127.0.0.1:1'], creationflags=subprocess.CREATE_NO_WINDOW)
            window = None
            try:
                deadline = time.monotonic() + 20
                while time.monotonic() < deadline and not window:
                    found = []
                    @CALLBACK
                    def enumerate_window(hwnd, _):
                        pid = wintypes.DWORD()
                        user.GetWindowThreadProcessId(hwnd, ctypes.byref(pid))
                        if pid.value == proc.pid and user.GetDlgItem(hwnd, 1014): found.append(hwnd)
                        return True
                    user.EnumWindows(enumerate_window, 0)
                    window = found[0] if found else None
                    if not window: time.sleep(0.05)
                self.assertTrue(window)
                user.SendMessageW(window, 0x111, 1014, user.GetDlgItem(window, 1014))
                status = user.GetDlgItem(window, 1008)
                message = ctypes.create_unicode_buffer(512)
                user.SendMessageW(status, 0xD, len(message), ctypes.addressof(message))
                self.assertIn('running', message.value.lower())
                (mods / 'mods_order.txt').write_bytes(b'steam-1c-text.dat\r\n')
                user.SetWindowTextW(user.GetDlgItem(window, 1002), str(root))
                deadline = time.monotonic() + 20
                while time.monotonic() < deadline and user.SendMessageW(user.GetDlgItem(window, 1004), 0x146, 0, 0) != 2:
                    time.sleep(0.05)
                self.assertEqual(user.SendMessageW(user.GetDlgItem(window, 1004), 0x146, 0, 0), 2)
                self.assertEqual(user.SendMessageW(user.GetDlgItem(window, 1017), 0x146, 0, 0), 1)
                user.SendMessageW(window, 0x111, 1014, user.GetDlgItem(window, 1014))
                deadline = time.monotonic() + 20
                while time.monotonic() < deadline and (mods / 'f2r-text-choice.txt').read_text().splitlines()[0] != 'original':
                    time.sleep(0.05)
                self.assertEqual((mods / 'f2r-text-choice.txt').read_text().splitlines()[0], 'original')
                self.assertEqual((mods / 'mods_order.txt').read_bytes(), b'')
                deadline = time.monotonic() + 20
                while time.monotonic() < deadline and (root / '.f2r-text-transaction').exists(): time.sleep(0.05)
                deadline = time.monotonic() + 20
                while True:
                    try:
                        (mods / 'rpu.dat').unlink()
                        break
                    except PermissionError:
                        if time.monotonic() >= deadline: raise
                        time.sleep(0.1)
                shutil.copyfile(packs / 'for-google-drive-rp-text/rp-1c-text.dat', mods / 'steam-1c-text.dat')
                (mods / 'f2r-text-choice.txt').write_text('1c\n')
                (mods / 'mods_order.txt').write_bytes(b'steam-1c-text.dat\r\n')
                (root / 'fallout2.cfg').write_bytes(b'[system]\r\nlanguage=russian\r\n')
                user.SetWindowTextW(user.GetDlgItem(window, 1002), str(root))
                user.SendMessageW(window, 0x111, 1014, user.GetDlgItem(window, 1014))
                deadline = time.monotonic() + 90
                expected = 'a89666c64983553b55d042b8a24b46f07ebc8ab782e7e6b1514d902a71d9d5f6'
                while time.monotonic() < deadline:
                    dat = mods / 'steam-1c-text.dat'
                    if dat.exists() and hashlib.sha256(dat.read_bytes()).hexdigest() == expected: break
                    time.sleep(0.1)
                self.assertEqual(hashlib.sha256((mods / 'steam-1c-text.dat').read_bytes()).hexdigest(), expected)
            finally:
                if window: user.PostMessageW(window, 0x10, 0, 0)
                try: proc.wait(timeout=5)
                except subprocess.TimeoutExpired:
                    proc.terminate()
                    proc.wait(timeout=5)

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
