import http.server
import pathlib
import os
import subprocess
import sys
import tempfile
import threading
import unittest

EXE = sys.argv.pop(1)
started = threading.Event()
release = threading.Event()


class Handler(http.server.BaseHTTPRequestHandler):
    def do_GET(self):
        started.set()
        release.wait(15)
        self.send_error(404)

    def log_message(self, *args):
        pass


class TestLock(unittest.TestCase):
    def test_recovery_before_journal_and_auto_discovered_mods(self):
        with tempfile.TemporaryDirectory() as temp:
            root = pathlib.Path(temp)
            for filename in ("master.dat", "critter.dat"):
                (root / filename).write_bytes(b"fixture")
            (root / "fallout2.cfg").write_bytes(b"[system]\r\nlanguage=english\r\n")
            work = root / ".f2r-text-transaction"
            for partial in (False, True):
                work.mkdir()
                if partial:
                    (work / "started").write_bytes(b"f2r")
                    (work / "journal.tmp").write_bytes(b"partial")
                result = subprocess.run([EXE, "--game-root", str(root), "--recover-text"], capture_output=True, timeout=10)
                self.assertEqual(result.returncode, 0, result.stdout)
                self.assertFalse(work.exists())
            mods = root / "mods"
            mods.mkdir()
            for name in ("zzz.DAT", "alpha.dat"):
                (mods / name).write_bytes(b"user mod")
            result = subprocess.run([EXE, "--game-root", str(root), "--text-translation", "original"], capture_output=True, timeout=10)
            self.assertEqual(result.returncode, 0, result.stdout)
            self.assertEqual((mods / "mods_order.txt").read_bytes(), b"alpha.dat\r\nzzz.DAT\r\n")

    @unittest.skipUnless(os.environ.get("F2R_OFFICIAL_FIXTURE"), "external verified official DAT not supplied")
    def test_official_manual_is_kept_and_owned_is_removed(self):
        data = pathlib.Path(os.environ["F2R_OFFICIAL_FIXTURE"]).read_bytes()
        for owned in (False, True):
            with self.subTest(owned=owned), tempfile.TemporaryDirectory() as temp:
                root = pathlib.Path(temp)
                for filename in ("master.dat", "critter.dat"):
                    (root / filename).write_bytes(b"fixture")
                (root / "fallout2.cfg").write_bytes(b"[system]\r\nlanguage=russian\r\n")
                mods = root / "mods"
                mods.mkdir()
                (mods / "rpu_russian.dat").write_bytes(data)
                (mods / "mods_order.txt").write_bytes(b"rpu_russian.dat\r\nplayer.dat\r\n")
                (mods / "f2r-text-choice.txt").write_text("official\n" + ("official-owned=1\n" if owned else ""))
                result = subprocess.run([EXE, "--game-root", str(root), "--text-translation", "original"], capture_output=True, timeout=10)
                self.assertEqual(result.returncode, 0, result.stdout)
                self.assertEqual((mods / "rpu_russian.dat").exists(), not owned)
                self.assertEqual((mods / "mods_order.txt").read_bytes(), b"player.dat\r\n")

    @unittest.skipUnless(os.environ.get("F2R_OFFICIAL_FIXTURE"), "external verified official DAT not supplied")
    def test_remove_russian_keeps_empty_order_for_manual_official(self):
        with tempfile.TemporaryDirectory() as temp:
            root = pathlib.Path(temp)
            for filename in ("master.dat", "critter.dat"):
                (root / filename).write_bytes(b"fixture")
            (root / "fallout2.cfg").write_bytes(b"[system]\r\nlanguage=russian\r\n")
            mods = root / "mods"
            mods.mkdir()
            (mods / "rpu_russian.dat").write_bytes(pathlib.Path(os.environ["F2R_OFFICIAL_FIXTURE"]).read_bytes())
            (mods / "mods_order.txt").write_bytes(b"rpu_russian.dat\r\n")
            result = subprocess.run([EXE, "--game-root", str(root), "--remove-russian"], capture_output=True, timeout=10)
            self.assertEqual(result.returncode, 0, result.stdout)
            self.assertTrue((mods / "rpu_russian.dat").exists())
            self.assertEqual((mods / "mods_order.txt").read_bytes(), b"")

    def test_foreign_same_name_is_not_removed(self):
        with tempfile.TemporaryDirectory() as temp:
            root = pathlib.Path(temp)
            for filename in ("master.dat", "critter.dat"):
                (root / filename).write_bytes(b"fixture")
            mods = root / "mods"
            mods.mkdir()
            (mods / "rpu_russian.dat").write_bytes(b"foreign mod")
            order = b"rpu_russian.dat\r\nplayer.dat\r\n"
            (mods / "mods_order.txt").write_bytes(order)
            (root / "fallout2.cfg").write_bytes(b"[system]\r\nlanguage=russian\r\n")
            result = subprocess.run([EXE, "--game-root", str(root), "--remove-russian"], capture_output=True, timeout=10)
            self.assertNotEqual(result.returncode, 0)
            self.assertEqual((mods / "rpu_russian.dat").read_bytes(), b"foreign mod")
            self.assertEqual((mods / "mods_order.txt").read_bytes(), order)
            self.assertIn(b"language=russian", (root / "fallout2.cfg").read_bytes())
            result = subprocess.run([EXE, "--game-root", str(root), "--text-translation", "official"], capture_output=True, timeout=10)
            self.assertNotEqual(result.returncode, 0)
            self.assertEqual((mods / "rpu_russian.dat").read_bytes(), b"foreign mod")
            self.assertEqual((mods / "mods_order.txt").read_bytes(), order)

    def test_second_worker_does_not_touch_active_root(self):
        with tempfile.TemporaryDirectory() as temp:
            root = pathlib.Path(temp)
            for filename in ("master.dat", "critter.dat"):
                (root / filename).write_bytes(b"fixture")
            (root / "fallout2.cfg").write_text("[system]\nlanguage=english\n")
            server = http.server.ThreadingHTTPServer(("127.0.0.1", 0), Handler)
            threading.Thread(target=server.serve_forever, daemon=True).start()
            url = f"http://127.0.0.1:{server.server_port}"
            first = subprocess.Popen([EXE, "--game-root", str(root), "--update-url", url,
                                      "--mirror-url", url, "--no-start"],
                                     stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
            try:
                self.assertTrue(started.wait(10), "first worker did not enter the network request")
                second = subprocess.run([EXE, "--game-root", str(root), "--text-translation", "original"],
                                        capture_output=True, text=True, timeout=10)
                self.assertNotEqual(second.returncode, 0)
                self.assertIn("another launcher is already working", second.stdout)
                self.assertEqual((root / "fallout2.cfg").read_text(), "[system]\nlanguage=english\n")
            finally:
                release.set()
                first.terminate()
                first.wait(timeout=10)
                server.shutdown()
                server.server_close()


if __name__ == "__main__":
    unittest.main()
