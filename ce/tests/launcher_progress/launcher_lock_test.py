"""The per-root writer lock (309): aliases collide, separate roots do not,
a dead holder's lock is gone, and the release check never takes the lock.

The harness comes from launcher_install_test.py: a disposable game root and
a local HTTP release, the real launcher exe, --no-start."""

import contextlib
import ctypes
import functools
import http.server
import os
import subprocess
import sys
import threading
import time
import unittest
from pathlib import Path

from launcher_install_test import ART, LAUNCHER, LauncherFixture, QuietHandler, REQUIRED, read_lock

sys.stdout.reconfigure(encoding="utf-8", errors="replace")
sys.stderr.reconfigure(encoding="utf-8", errors="replace")
LOCK = "f2reimagined.lock"
# A separate process that holds the lock file: READY only after it owns it.
HOLDER = (
    "import ctypes, sys, time\n"
    "k = ctypes.WinDLL('kernel32', use_last_error=True)\n"
    "k.CreateFileW.restype = ctypes.c_void_p\n"
    "h = k.CreateFileW(sys.argv[1], 0xC0000000, 0, None, 3, 0x400, None)\n"
    "if not h: sys.stdout.write('FAIL'); sys.stdout.flush(); sys.exit(1)\n"
    "sys.stdout.write('READY'); sys.stdout.flush()\n"
    "time.sleep(60)\n"
)


class GatedHandler(QuietHandler):
    # The first request waits until the test opens the gate: the launcher
    # that sent it stands past its root lock, before any write (311).
    def do_GET(self):
        with self.server.mutex:
            self.server.paths.append(self.path)
            first = len(self.server.paths) == 1
        if first:
            self.server.arrived.set()
            self.server.gate.wait(60)
        super().do_GET()


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

    def parked_launcher(self):
        # A real launcher worker parked on its first release request, which
        # it sends only after it holds the root lock.
        server = http.server.ThreadingHTTPServer(("127.0.0.1", 0),
            functools.partial(GatedHandler, directory=str(self.release)))
        server.mutex, server.paths = threading.Lock(), []
        server.arrived, server.gate = threading.Event(), threading.Event()
        thread = threading.Thread(target=server.serve_forever, daemon=True)
        thread.start()
        self.addCleanup(server.server_close)
        self.addCleanup(thread.join)
        self.addCleanup(server.shutdown)
        self.addCleanup(server.gate.set)
        url = f"http://127.0.0.1:{server.server_port}"
        worker = subprocess.Popen([str(LAUNCHER), "--game-root", str(self.root),
            "--update-url", url, "--mirror-url", url, "--ui-language", "en", "--no-start"],
            stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        self.addCleanup(worker.wait, 10)
        self.addCleanup(worker.kill)
        self.assertTrue(server.arrived.wait(30), "the parked launcher reached the release")
        self.assertTrue((self.root / LOCK).exists(), "the parked launcher holds the root lock")
        return worker, server

    def test_second_launcher_refuses_a_live_one(self):
        # Two real launcher processes on one root: the second refuses before
        # any network or write, then the first finishes its install (311).
        first, server = self.parked_launcher()
        # The live holder opened the lock with no sharing, so it is not read.
        before = {p.relative_to(self.root): p.read_bytes()
                  for p in self.root.rglob("*") if p.name != LOCK and p.is_file()}
        code, output = self.run_launcher()
        self.assertNotEqual(code, 0, output)
        self.assertIn("already running", output)
        self.assertNotIn("release build", output)
        after = {p.relative_to(self.root): p.read_bytes()
                 for p in self.root.rglob("*") if p.name != LOCK and p.is_file()}
        self.assertEqual(before, after)
        server.gate.set()
        firstOutput = first.communicate(timeout=90)[0].decode("utf-8", errors="replace")
        print(f"\n{self.id()} first: exit {first.returncode}\n{firstOutput}", flush=True)
        self.assertEqual(first.returncode, 0, firstOutput)
        self.assertEqual((self.root / "version.txt").read_bytes(), b"new-build")
        self.assertFalse((self.root / LOCK).exists())

    def test_killed_launcher_releases_the_lock(self):
        # A real launcher killed while it holds the root: the OS closes its
        # handle, and the next worker walks in and installs (311).
        first, _server = self.parked_launcher()
        first.kill()
        first.wait(timeout=10)
        code, output = self.run_launcher()
        self.assertEqual(code, 0, output)
        self.assertEqual((self.root / "version.txt").read_bytes(), b"new-build")
        self.assertFalse((self.root / LOCK).exists())

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
        holder = subprocess.Popen([sys.executable, "-c", HOLDER, str(self.root / LOCK)],
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

    def test_holder_without_the_lock_never_says_ready(self):
        # A failed open must read as FAIL, never READY: otherwise the killed
        # holder test could pass without any holder owning the lock (312).
        missing = self.base / "no-such-folder" / LOCK
        result = subprocess.run([sys.executable, "-c", HOLDER, str(missing)], capture_output=True, timeout=30)
        self.assertNotIn(b"READY", result.stdout)
        self.assertTrue(result.stdout.startswith(b"FAIL"), result.stdout)
        self.assertNotEqual(result.returncode, 0)

    def test_release_check_never_takes_the_lock(self):
        with hold_lock(self.root / LOCK):
            code, output = self.run_launcher_at(self.root, extra=("--check-release",))
            self.assertEqual(code, 0, output)
            self.assertIn("RELEASE_VERSION:", output)


if __name__ == "__main__":
    unittest.main()
