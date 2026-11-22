import http.server
import functools
import os
import pathlib
import shutil
import subprocess
import sys
import tempfile
import threading
import unittest

EXE = sys.argv.pop(1)
DATA = b"fixture voice package\n"


class Handler(http.server.BaseHTTPRequestHandler):
    payload = DATA

    def do_GET(self):
        self.send_response(200)
        self.end_headers()
        self.wfile.write(self.payload)

    def log_message(self, *args):
        pass


class TestVoice(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.root = pathlib.Path(self.tmp.name)
        for name in ("master.dat", "critter.dat"):
            (self.root / name).write_bytes(b"game")
        (self.root / "fallout2.cfg").write_bytes(b"[system]\r\nlanguage=russian\r\n")
        self.mods = self.root / "mods"
        self.mods.mkdir()
        (self.mods / "player.dat").write_bytes(b"foreign")
        (self.mods / "upu_russian_sound.dat").write_bytes(b"manual")
        (self.mods / "mods_order.txt").write_bytes(b"upu_russian_sound.dat\r\nplayer.dat\r\n")

    def run_launcher(self, *args, ok=True):
        result = subprocess.run([EXE, "--game-root", str(self.root), *args], capture_output=True, timeout=20)
        self.assertEqual(result.returncode == 0, ok, result.stdout + result.stderr)
        return result

    def test_old_journal_recovery(self):
        work = self.root / ".f2r-text-transaction"
        work.mkdir()
        (work / "journal").write_text("12222222")
        (work / "old0").write_bytes(b"old config")
        self.run_launcher("--recover-text")
        self.assertEqual((self.root / "fallout2.cfg").read_bytes(), b"old config")
        self.assertFalse(work.exists())

    def test_voice_journal_recovery(self):
        work = self.root / ".f2r-text-transaction"
        work.mkdir()
        (work / "journal").write_text("22222222212222")
        (work / "old9").write_bytes(b"previous voice")
        voice = self.mods / "f2r-voice-vanilla-1c.dat"
        voice.write_bytes(b"interrupted replacement")
        self.run_launcher("--recover-text")
        self.assertEqual(voice.read_bytes(), b"previous voice")
        self.assertFalse(work.exists())

    def test_interrupted_rp_transition_retires_incompatible_voice(self):
        voice = self.mods / "f2r-voice-vanilla-1c.dat"
        voice.write_bytes(DATA)
        (self.mods / "f2r-voice-choice.txt").write_text("vanilla-1c\nhash=16b874f8ebf84d286c4684a0a7a78c0cd00c84c35515a0eee9d889746537f2c1\n")
        (self.mods / "mods_order.txt").write_bytes(b"f2r-voice-vanilla-1c.dat\r\nplayer.dat\r\n")
        (self.mods / "rpu.dat").write_bytes(b"RP")
        marker = self.root / ".f2r-voice-rp-transition"
        marker.write_text("F2R voice RP transition\n")
        self.run_launcher("--recover-text")
        self.assertFalse(marker.exists())
        self.assertFalse(voice.exists())
        self.assertEqual((self.mods / "mods_order.txt").read_bytes(), b"player.dat\r\n")
        self.run_launcher("--recover-text")

    def test_interrupted_rp_removal_retires_voice_on_retry(self):
        voice = self.mods / "f2r-voice-rp-1c.dat"
        voice.write_bytes(DATA)
        (self.mods / "f2r-voice-choice.txt").write_text("rp-1c\nhash=16b874f8ebf84d286c4684a0a7a78c0cd00c84c35515a0eee9d889746537f2c1\n")
        (self.mods / "mods_order.txt").write_bytes(b"f2r-voice-rp-1c.dat\r\nplayer.dat\r\n")
        marker = self.root / ".f2r-voice-rp-transition"
        marker.write_text("F2R voice RP transition\n")
        self.run_launcher("--recover-text")
        self.assertFalse(marker.exists())
        self.assertFalse(voice.exists())
        self.assertEqual((self.mods / "mods_order.txt").read_bytes(), b"player.dat\r\n")
        self.run_launcher("--recover-text")

    @unittest.skipUnless(os.environ.get("F2R_VOICE_FIXTURE") and os.environ.get("F2R_VOICE_REAL_RP"), "fixture launcher and real RP required")
    def test_real_rp_voice_gate(self):
        server = http.server.ThreadingHTTPServer(("127.0.0.1", 18751), Handler)
        threading.Thread(target=server.serve_forever, daemon=True).start()
        self.addCleanup(server.server_close)
        self.addCleanup(server.shutdown)
        rp = self.mods / "rpu.dat"
        rp.write_bytes(b"unsupported")
        self.run_launcher("--voice-translation", "rp-1c", ok=False)
        shutil.copyfile(os.environ["F2R_VOICE_REAL_RP"], rp)
        self.run_launcher("--voice-translation", "rp-1c")
        self.assertEqual((self.mods / "f2r-voice-rp-1c.dat").read_bytes(), DATA)

    @unittest.skipUnless(os.environ.get("F2R_VOICE_KNOWN_DAT"), "verified BGforge DAT required")
    def test_known_manual_voice_is_restored_on_uninstall(self):
        manual = self.mods / "upu_russian_sound.dat"
        shutil.copyfile(os.environ["F2R_VOICE_KNOWN_DAT"], manual)
        original = b"# player.dat\r\nupu_russian_sound.dat\r\nplayer.dat\r\n"
        (self.mods / "mods_order.txt").write_bytes(original)
        self.run_launcher("--voice-translation", "original")
        self.assertEqual((self.mods / "mods_order.txt").read_bytes(), b"# player.dat\r\nplayer.dat\r\n")
        self.assertTrue(manual.exists())
        self.run_launcher("--uninstall")
        self.assertEqual((self.mods / "mods_order.txt").read_bytes(), original)
        self.assertTrue(manual.exists())

    @unittest.skipUnless(os.environ.get("F2R_VOICE_KNOWN_DAT"), "verified BGforge DAT required")
    def test_manual_voice_restores_lf_order(self):
        shutil.copyfile(os.environ["F2R_VOICE_KNOWN_DAT"], self.mods / "upu_russian_sound.dat")
        original = b"# player.dat\nupu_russian_sound.dat\nplayer.dat\n"
        (self.mods / "mods_order.txt").write_bytes(original)
        self.run_launcher("--voice-translation", "original")
        self.run_launcher("--uninstall")
        self.assertEqual((self.mods / "mods_order.txt").read_bytes(), original)

    @unittest.skipUnless(os.environ.get("F2R_VOICE_KNOWN_DAT"), "verified BGforge DAT required")
    def test_manual_voice_restores_repeated_blank_lines_and_appended_rows(self):
        shutil.copyfile(os.environ["F2R_VOICE_KNOWN_DAT"], self.mods / "upu_russian_sound.dat")
        order = self.mods / "mods_order.txt"
        original = b"\r\nplayer.dat\r\nupu_russian_sound.dat\r\n\r\nother.dat\r\n# note\r\n# note\r\n"
        order.write_bytes(original)
        self.run_launcher("--voice-translation", "original")
        appended = b"\r\nextra.dat\r\n# note\r\n"
        order.write_bytes(order.read_bytes() + appended)
        self.run_launcher("--uninstall")
        self.assertEqual(order.read_bytes(), original + appended)

    @unittest.skipUnless(os.environ.get("F2R_VOICE_KNOWN_DAT"), "verified BGforge DAT required")
    def test_manual_voice_restores_exact_order_with_repeated_blank_lines(self):
        shutil.copyfile(os.environ["F2R_VOICE_KNOWN_DAT"], self.mods / "upu_russian_sound.dat")
        order = self.mods / "mods_order.txt"
        original = b"\r\nplayer.dat\r\nupu_russian_sound.dat\r\n\r\nother.dat\r\n"
        order.write_bytes(original)
        self.run_launcher("--voice-translation", "original")
        self.run_launcher("--uninstall")
        self.assertEqual(order.read_bytes(), original)

    @unittest.skipUnless(os.environ.get("F2R_VOICE_KNOWN_DAT"), "verified BGforge DAT required")
    def test_manual_voice_edited_order_uses_only_unique_anchor(self):
        shutil.copyfile(os.environ["F2R_VOICE_KNOWN_DAT"], self.mods / "upu_russian_sound.dat")
        order = self.mods / "mods_order.txt"
        order.write_bytes(b"before.dat\r\nupu_russian_sound.dat\r\n\r\n# comment\r\nother.dat\r\n")
        self.run_launcher("--voice-translation", "original")
        edited = b"# comment\r\nother.dat\r\n\r\nother.dat\r\nplayer.dat\r\n"
        order.write_bytes(edited)
        self.run_launcher("--uninstall")
        self.assertEqual(order.read_bytes(), edited + b"upu_russian_sound.dat\r\n")

    @unittest.skipUnless(os.environ.get("F2R_VOICE_KNOWN_DAT"), "verified BGforge DAT required")
    def test_manual_voice_edited_order_uses_unique_predecessor(self):
        shutil.copyfile(os.environ["F2R_VOICE_KNOWN_DAT"], self.mods / "upu_russian_sound.dat")
        order = self.mods / "mods_order.txt"
        order.write_bytes(b"player.dat\r\nupu_russian_sound.dat\r\n# end\r\n")
        self.run_launcher("--voice-translation", "original")
        order.write_bytes(b"player.dat\r\n# changed\r\noverride-voice.dat\r\n")
        self.run_launcher("--uninstall")
        self.assertEqual(order.read_bytes(), b"player.dat\r\nupu_russian_sound.dat\r\n# changed\r\noverride-voice.dat\r\n")

    @unittest.skipUnless(os.environ.get("F2R_VOICE_KNOWN_DAT") and os.environ.get("F2R_TEXT_FIXTURE_DIR"), "verified text and BGforge DATs required")
    def test_combined_text_voice_and_text_switch_restore_user_order(self):
        class TextHandler(http.server.SimpleHTTPRequestHandler):
            def log_message(self, *args): pass
        server = http.server.ThreadingHTTPServer(("127.0.0.1", 18752),
            functools.partial(TextHandler, directory=os.environ["F2R_TEXT_FIXTURE_DIR"]))
        threading.Thread(target=server.serve_forever, daemon=True).start()
        self.addCleanup(server.server_close)
        self.addCleanup(server.shutdown)
        for switch in (False, True):
            with self.subTest(switch=switch):
                shutil.copyfile(os.environ["F2R_VOICE_KNOWN_DAT"], self.mods / "upu_russian_sound.dat")
                order = self.mods / "mods_order.txt"
                original = b"player.dat\r\nupu_russian_sound.dat\r\n# end\r\n"
                order.write_bytes(original)
                self.run_launcher("--text-translation", "1c", "--voice-translation", "original")
                if switch: self.run_launcher("--text-translation", "levy")
                with order.open("ab") as out: out.write(b"override-voice.dat\r\n")
                self.run_launcher("--uninstall")
                self.assertEqual(order.read_bytes(), original + b"override-voice.dat\r\n")
        order.write_bytes(original)
        self.run_launcher("--text-translation", "1c")
        self.run_launcher("--voice-translation", "original")
        self.run_launcher("--text-translation", "levy")
        with order.open("ab") as out: out.write(b"override-voice.dat\r\n")
        self.run_launcher("--uninstall")
        self.assertEqual(order.read_bytes(), original + b"override-voice.dat\r\n")

    @unittest.skipUnless(os.environ.get("F2R_VOICE_KNOWN_DAT"), "verified BGforge DAT required")
    def test_uninstall_when_suppressed_manual_dat_was_removed_or_replaced(self):
        manual = self.mods / "upu_russian_sound.dat"
        order = self.mods / "mods_order.txt"
        for replace in (False, True):
            shutil.copyfile(os.environ["F2R_VOICE_KNOWN_DAT"], manual)
            order.write_bytes(b"upu_russian_sound.dat\r\nplayer.dat\r\n")
            self.run_launcher("--voice-translation", "original")
            if replace: manual.write_bytes(b"foreign")
            else: manual.unlink()
            self.run_launcher("--uninstall")
            self.assertEqual(order.read_bytes(), b"player.dat\r\n")
            self.assertFalse((self.mods / "f2r-voice-manual-order.txt").exists())
            if replace: self.assertEqual(manual.read_bytes(), b"foreign")

    def test_directories_at_transaction_targets_are_preserved(self):
        for name in ("f2r-voice-choice.txt", "f2r-voice-manual-order.txt", "f2r-text-choice.txt",
                     "f2r-voice-vanilla-1c.dat", "steam-1c-text.dat"):
            with self.subTest(name=name):
                path = self.mods / name
                path.mkdir()
                (path / "keep.txt").write_text("do not delete")
                self.run_launcher("--voice-translation", "original", ok=False)
                self.assertEqual((path / "keep.txt").read_text(), "do not delete")
                shutil.rmtree(path)

    def test_reparse_target_is_preserved(self):
        outside = self.root / "outside"
        outside.mkdir()
        (outside / "keep.txt").write_text("do not delete")
        junction = self.mods / "f2r-voice-choice.txt"
        result = subprocess.run(["cmd", "/c", "mklink", "/J", str(junction), str(outside)], capture_output=True)
        if result.returncode: self.skipTest("junction creation unavailable")
        self.run_launcher("--voice-translation", "original", ok=False)
        self.assertEqual((outside / "keep.txt").read_text(), "do not delete")
        self.assertTrue(junction.exists())

    def test_unavailable_and_foreign_file_untouched(self):
        path = self.mods / "f2r-voice-vanilla-1c.dat"
        path.write_bytes(b"foreign")
        before = (self.mods / "mods_order.txt").read_bytes()
        self.run_launcher("--voice-translation", "original")
        self.assertEqual(path.read_bytes(), b"foreign")
        self.assertEqual((self.mods / "mods_order.txt").read_bytes(), before)
        self.run_launcher("--voice-translation", "unknown", ok=False)

    @unittest.skipUnless(os.environ.get("F2R_VOICE_FIXTURE"), "fixture-configured launcher required")
    def test_install_switch_failure_and_legacy_text(self):
        server = http.server.ThreadingHTTPServer(("127.0.0.1", 18751), Handler)
        threading.Thread(target=server.serve_forever, daemon=True).start()
        self.addCleanup(server.server_close)
        self.addCleanup(server.shutdown)
        voice = self.mods / "f2r-voice-vanilla-1c.dat"
        self.run_launcher("--voice-translation", "vanilla-1c")
        self.assertEqual(voice.read_bytes(), DATA)
        self.assertTrue((self.mods / "mods_order.txt").read_bytes().endswith(b"f2r-voice-vanilla-1c.dat\r\n"))
        self.assertIn(b"hash=16b874f8ebf84d286c4684a0a7a78c0cd00c84c35515a0eee9d889746537f2c1", (self.mods / "f2r-voice-choice.txt").read_bytes())
        (self.mods / "rpu.dat").write_bytes(b"manual RP")
        self.run_launcher("--voice-translation", "vanilla-1c", ok=False)
        self.assertEqual(voice.read_bytes(), DATA)
        (self.mods / "rpu.dat").unlink()
        self.assertIn(b"language=russian", (self.root / "fallout2.cfg").read_bytes())
        self.run_launcher("--text-translation", "original")
        self.assertEqual(voice.read_bytes(), DATA)
        before = (self.mods / "mods_order.txt").read_bytes()
        (self.root / "fallout2.cfg").write_bytes(b"[system]\r\nlanguage=russian\r\n")
        cfg = (self.root / "fallout2.cfg").read_bytes()
        Handler.payload = b"x" * len(DATA)
        self.addCleanup(setattr, Handler, "payload", DATA)
        result = self.run_launcher("--text-translation", "original", "--voice-translation", "vanilla-1c", ok=False)
        self.assertIn(b"SHA-256", result.stdout + result.stderr)
        self.assertEqual((self.mods / "mods_order.txt").read_bytes(), before)
        self.assertEqual((self.root / "fallout2.cfg").read_bytes(), cfg)
        self.assertEqual(voice.read_bytes(), DATA)
        self.run_launcher("--voice-translation", "original")
        self.assertFalse(voice.exists())
        self.assertTrue((self.mods / "upu_russian_sound.dat").exists())
        self.assertEqual((self.mods / "player.dat").read_bytes(), b"foreign")
        Handler.payload = DATA
        self.run_launcher("--voice-translation", "vanilla-1c")
        self.run_launcher("--uninstall")
        self.assertFalse(voice.exists())
        self.assertEqual((self.mods / "upu_russian_sound.dat").read_bytes(), b"manual")
        self.assertEqual((self.mods / "player.dat").read_bytes(), b"foreign")


if __name__ == "__main__":
    unittest.main()
