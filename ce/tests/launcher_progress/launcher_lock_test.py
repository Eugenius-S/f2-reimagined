"""The per-root writer lock (309): aliases collide, separate roots do not,
a dead holder's lock is gone, and the release check never takes the lock.

The harness comes from launcher_install_test.py: a disposable game root and
a local HTTP release, the real launcher exe, --no-start."""

import contextlib
import ctypes
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


@contextlib.contextmanager
def hold_lock(path):
    # The harness read_lock opens with OPEN_EXISTING (creation 3), so the
    # lock file must stand before it is held; the launcher itself opens with
    # no sharing, which any existing holder blocks.
    path.write_bytes(b"")
    with read_lock(path):
        yield


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
        with hold_lock(self.root / LOCK):
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
        # relative path to it does too; so does a junction to it.
        junction = self.base / "game-junction"
        subprocess.run(["cmd", "/c", "mklink", "/J", str(junction), str(self.root)],
                       capture_output=True, check=True)
        self.addCleanup(lambda: subprocess.run(["cmd", "/c", "rmdir", str(junction)], capture_output=True))
        with hold_lock(self.root / LOCK):
            code, output = self.run_launcher_at(str(self.root).upper())
            self.assertNotEqual(code, 0, output)
            self.assertIn("already running", output)
            code, output = self.run_launcher_at("game", cwd=self.base)
            self.assertNotEqual(code, 0, output)
            self.assertIn("already running", output)
            code, output = self.run_launcher_at(junction)
            self.assertNotEqual(code, 0, output)
            self.assertIn("already running", output)

    def test_another_root_is_independent(self):
        other = self.base / "other-game"
        other.mkdir()
        for name in ("master.dat", "critter.dat", *REQUIRED):
            (other / name).write_bytes(b"old-build")
        with hold_lock(self.root / LOCK):
            code, output = self.run_launcher_at(other)
            self.assertEqual(code, 0, output)
            self.assertEqual((other / "version.txt").read_bytes(), b"new-build")

    def test_killed_holder_releases_the_lock(self):
        # The holder really owns the lock (exclusive open, checked handle,
        # DELETE_ON_CLOSE), signals READY only after acquiring it, and the
        # parent proves a contender cannot enter before the kill; when the
        # holder's process dies, the OS closes the handle, the file goes
        # away, and the next worker walks in.
        holderScript = (
            "import ctypes, sys, time\n"
            "k = ctypes.WinDLL('kernel32', use_last_error=True)\n"
            "k.CreateFileW.restype = ctypes.c_void_p\n"
            "h = k.CreateFileW(sys.argv[1], 0xC0000000, 0, None, 3, 0x400, None)\n"
            "if not h:\n"
            "    sys.stdout.write('FAIL'); sys.stdout.flush(); sys.exit(1)\n"
            "sys.stdout.write('READY'); sys.stdout.flush()\n"
            "time.sleep(60)\n"
        )
        holder = subprocess.Popen([sys.executable, "-c", holderScript, str(self.root / LOCK)],
                                  stdout=subprocess.PIPE)
        self.addCleanup(holder.kill)
        (self.root / LOCK).write_bytes(b"")  # the child opens with OPEN_EXISTING
        try:
            state = holder.stdout.read(5).decode("ascii")
        except OSError:
            state = ""
        self.assertEqual(state, "READY", "the holder acquired the real lock file")
        # A contender's exclusive open of the same file must fail while the
        # holder stands (the proof the lock excludes, before the kill).
        kernel = ctypes.WinDLL("kernel32", use_last_error=True)
        kernel.CreateFileW.restype = ctypes.c_void_p
        contender = kernel.CreateFileW(str(self.root / LOCK), 0xC0000000, 0, None, 3, 0, None)
        self.assertEqual(contender, ctypes.c_void_p(-1).value,
                         "a second exclusive open is refused while the holder stands")
        holder.kill()
        holder.wait(timeout=10)
        code, output = self.run_launcher()
        self.assertEqual(code, 0, output)
        self.assertEqual((self.root / "version.txt").read_bytes(), b"new-build")
        self.assertFalse((self.root / LOCK).exists())

    def test_release_check_never_takes_the_lock(self):
        with hold_lock(self.root / LOCK):
            code, output = self.run_launcher_at(self.root, extra=("--check-release",))
            self.assertEqual(code, 0, output)
            self.assertIn("RELEASE_VERSION:", output)


if __name__ == "__main__":
    unittest.main()
