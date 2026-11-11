import hashlib
import shutil
import subprocess
import tempfile
import unittest
from pathlib import Path

from tour import powershell_args


@unittest.skipUnless(shutil.which("powershell"), "Windows PowerShell is required")
class PowerShellArgsTests(unittest.TestCase):
    def test_snapshot_script_hashes_when_module_autoloading_starts_disabled(self):
        with tempfile.TemporaryDirectory() as directory:
            sample = Path(directory) / "sample.txt"
            sample.write_text("tour", encoding="utf-8")
            snapshot = Path(directory) / "dev-snapshot.ps1"
            snapshot.write_text(
                "$ErrorActionPreference = 'Stop'; "
                f"(Get-FileHash -LiteralPath '{sample}' -Algorithm SHA256).Hash",
                encoding="utf-8")
            command = f"& '{snapshot}'"
            disable_autoloading = "$PSModuleAutoLoadingPreference = 'None'; "

            baseline = subprocess.run(
                ["powershell", "-NoProfile", "-ExecutionPolicy", "Bypass", "-Command",
                 disable_autoloading + command], capture_output=True, text=True, errors="replace")
            fixed = subprocess.run(powershell_args(command, before_prelude=disable_autoloading),
                                   capture_output=True, text=True, errors="replace")

        self.assertNotEqual(baseline.returncode, 0, "baseline unexpectedly resolved Get-FileHash")
        self.assertIn("Get-FileHash", baseline.stderr)
        self.assertEqual(fixed.returncode, 0, fixed.stderr)
        self.assertEqual(fixed.stdout.strip(), hashlib.sha256(b"tour").hexdigest().upper())


if __name__ == "__main__":
    unittest.main()
