"""The per-root writer lock (309): aliases collide, separate roots do not,
a dead holder's lock is gone, and the release check never takes the lock.

The harness comes from launcher_install_test.py: a disposable game root and
a local HTTP release, the real launcher exe, --no-start."""

import os
import subprocess
import sys
import time
import unittest
from pathlib import Path

from launcher_install_test import ART, LAUNCHER, LauncherFixture, REQUIRED, read_lock

sys.stdout.reconfigure(encoding="utf-8", errors="replace")
sys.stderr.reconfigure(encoding="utf-8", errors="replace")
LOCK = "f2reimagined.lock"


class LockTests(LauncherFixture):
    def run_launcher_at(self, root, extra=(), cwd=None):
        result = subprocess.run([str(LAUNCHER), "--game-root", str(root),
            "--update-url", self.url, "--mirror-url", self.url, "--ui-language", "en",
            "--no-start", *extra], capture_output=True, timeout=90, cwd=cwd)
        output = result.stdout.decode("utf-8", errors="replace") + result.stderr.decode("utf-8", errors="replace")
        print(f"\n{self.id()}: exit {result.returncode}\n{output}", flush=True)
        return result.returncode, output

    def tree(self, root):
        return {p.relative_to(root): p.read_bytes() for p in Path(root).rglob("*") if p.is_file()}

    def test_busy_root_changes_nothing(self):
        # A live holder of the lock file: the second worker refuses and the
        # game folder stays byte for byte as it was.
        with read_lock(self.root / LOCK):
            before = self.tree(self.root)
            code, output = self.run_launcher()
            self.assertNotEqual(code, 0, output)
            self.assertIn("already running", output)
            self.assertEqual(before, self.tree(self.root))
        # The holder let go: the same worker now walks in, installs, and its
        # DELETE_ON_CLOSE lock file is gone when the worker exits.
        code, output = self.run_launcher()
        self.assertEqual(code, 0, output)
        self.assertEqual((self.root / "version.txt").read_bytes(), b"new-build")
        self.assertFalse((self.root / LOCK).exists())

    def test_alias_paths_meet_the_same_lock(self):
        # Another case of the same folder names the same physical file; a
        # relative path to it does too.
        with read_lock(self.root / LOCK):
            code, output = self.run_launcher_at(str(self.root).upper())
            self.assertNotEqual(code, 0, output)
            self.assertIn("already running", output)
            code, output = self.run_launcher_at("game", cwd=self.base)
            self.assertNotEqual(code, 0, output)
            self.assertIn("already running", output)

    def test_another_root_is_independent(self):
        other = self.base / "other-game"
        other.mkdir()
        for name in ("master.dat", "critter.dat", *REQUIRED):
            (other / name).write_bytes(b"old-build")
        with read_lock(self.root / LOCK):
            code, output = self.run_launcher_at(other)
            self.assertEqual(code, 0, output)
            self.assertEqual((other / "version.txt").read_bytes(), b"new-build")

    def test_killed_holder_releases_the_lock(self):
        # The lock file carries FILE_FLAG_DELETE_ON_CLOSE: when the holder's
        # process dies, the OS closes the handle, the file goes away, and the
        # next worker walks in.
        holder = subprocess.Popen([sys.executable, "-c",
            "import ctypes, time, sys\n"
            "h = ctypes.WinDLL('kernel32').CreateFileW(sys.argv[1], 0x80000000, 1, None, 3, 0, None)\n"
            "time.sleep(60)\n", str(self.root / LOCK)])
        self.addCleanup(holder.kill)
        time.sleep(1.0)
        holder.kill()
        holder.wait(timeout=10)
        code, output = self.run_launcher()
        self.assertEqual(code, 0, output)
        self.assertEqual((self.root / "version.txt").read_bytes(), b"new-build")

    def test_release_check_never_takes_the_lock(self):
        with read_lock(self.root / LOCK):
            code, output = self.run_launcher_at(self.root, extra=("--check-release",))
            self.assertEqual(code, 0, output)
            self.assertIn("RELEASE_VERSION:", output)


if __name__ == "__main__":
    unittest.main()
