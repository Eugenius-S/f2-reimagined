"""The interrupted-update recovery (307) at worker level: a seeded marker,
journal and backup, the real launcher exe, --rollback, no network at all.

Covers the audit matrix's I2 (blocked restore, retry finishes it), the
empty-marker block (an unreadable marker is a pending state) and the
repair-in-one-step rule (no second rollback past the repaired build)."""

import os
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

from launcher_install_test import LAUNCHER, read_lock

sys.stdout.reconfigure(encoding="utf-8", errors="replace")
sys.stderr.reconfigure(encoding="utf-8", errors="replace")

BACKUPS = "f2reimagined-backups"
MARKER = "update-in-progress.txt"
MANIFEST = "update-manifest.txt"


def run_launcher(args):
    result = subprocess.run(args, capture_output=True, timeout=90)
    output = result.stdout.decode("utf-8", errors="replace") + result.stderr.decode("utf-8", errors="replace")
    return result.returncode, output


@unittest.skipUnless(os.name == "nt", "the launcher uses Windows system tools")
class RecoveryFixture(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(prefix="f2r-recovery-test-")
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name) / "game"
        self.root.mkdir()
        # The installed state: version stamp v1, a mixed half-installed build.
        (self.root / "fallout2-ce.exe").write_bytes(b"new-exe")
        (self.root / "ce.dat").write_bytes(b"new-dat")
        (self.root / "version.txt").write_bytes(b"v1")
        (self.root / "master.dat").write_bytes(b"old")
        (self.root / "critter.dat").write_bytes(b"old")
        self.backup = self.root / BACKUPS / "v1"
        self.backup.mkdir(parents=True)
        (self.backup / "fallout2-ce.exe").write_bytes(b"old-exe")
        (self.backup / "ce.dat").write_bytes(b"old-dat")
        (self.backup / "version.txt").write_bytes(b"v1")
        # The legacy receipt contract: a backup without a receipt promises its
        # version stamp, which is what the repair selects it by.
        (self.root / BACKUPS / MARKER).write_text("v1\n", encoding="ascii")
        (self.root / BACKUPS / MANIFEST).write_text(
            "Y fallout2-ce.exe\nY ce.dat\nY version.txt\n", encoding="ascii")

    def run_rollback(self):
        code, output = run_launcher([str(LAUNCHER), "--game-root", str(self.root), "--rollback"])
        print(f"\n{self.id()}: exit {code}\n{output}", flush=True)
        return code, output

    def assert_restored(self):
        self.assertEqual((self.root / "fallout2-ce.exe").read_bytes(), b"old-exe")
        self.assertEqual((self.root / "ce.dat").read_bytes(), b"old-dat")
        self.assertEqual((self.root / "version.txt").read_bytes(), b"v1")
        self.assertFalse((self.root / BACKUPS / MARKER).exists())


class RecoveryTests(RecoveryFixture):
    def test_blocked_restore_keeps_state_and_retry_finishes(self):
        # A read lock on the game file denies the restore's write: recovery
        # names the file and keeps the marker and backup; once it is gone,
        # the same command finishes the repair.
        with read_lock(self.root / "ce.dat"):
            code, output = self.run_rollback()
            self.assertNotEqual(code, 0, output)
            self.assertIn("ce.dat", output)
            self.assertTrue((self.root / BACKUPS / MARKER).exists())
            self.assertEqual((self.root / "ce.dat").read_bytes(), b"new-dat")
        code, output = self.run_rollback()
        self.assertEqual(code, 0, output)
        self.assert_restored()

    def test_empty_marker_is_blocked_not_skipped(self):
        (self.root / BACKUPS / MARKER).write_text("", encoding="ascii")
        before = {p.name: p.read_bytes() for p in self.root.iterdir() if p.is_file()}
        code, output = self.run_rollback()
        self.assertNotEqual(code, 0, output)
        self.assertIn("marker", output)
        self.assertEqual({p.name: p.read_bytes() for p in self.root.iterdir() if p.is_file()}, before)
        self.assertTrue((self.root / BACKUPS / MARKER).exists())

    def test_rollback_repairs_in_one_step(self):
        # The backup's version equals the installed stamp: the repair happens
        # and reports once; no second rollback walks past the repaired build.
        code, output = self.run_rollback()
        self.assertEqual(code, 0, output)
        self.assert_restored()
        self.assertIn("v1", output)


if __name__ == "__main__":
    unittest.main()
