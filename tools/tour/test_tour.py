import hashlib
import re
import shutil
import subprocess
import tempfile
import unittest
from pathlib import Path

from tour import ROOT, powershell_args


@unittest.skipUnless(shutil.which("powershell"), "Windows PowerShell is required")
class PowerShellArgsTests(unittest.TestCase):
    def test_snapshot_hashing_works_when_module_autoloading_is_disabled(self):
        snapshot_source = (ROOT / "tools" / "dev-snapshot.ps1").read_text(encoding="utf-8-sig")
        self.assertNotIn("Get-FileHash", snapshot_source)
        helper = re.search(
            r"(?ms)^function Get-Sha256FileHash\b.*?^}\s*$", snapshot_source)
        self.assertIsNotNone(helper, "dev-snapshot SHA256 helper was not found")
        with tempfile.TemporaryDirectory() as directory:
            sample = Path(directory) / "sample.txt"
            sample.write_text("tour", encoding="utf-8")
            quote = lambda path: "'" + str(path).replace("'", "''") + "'"
            command = helper.group(0) + "; Get-Sha256FileHash " + quote(sample)
            disable_autoloading = "$PSModuleAutoLoadingPreference = 'None'; "

            baseline = subprocess.run(
                ["powershell", "-NoProfile", "-ExecutionPolicy", "Bypass", "-Command",
                 disable_autoloading + "Get-FileHash -LiteralPath " + quote(sample)],
                capture_output=True, text=True, errors="replace")
            fixed = subprocess.run(powershell_args(disable_autoloading + command),
                                   capture_output=True, text=True, errors="replace")

        self.assertNotEqual(baseline.returncode, 0, "baseline unexpectedly resolved Get-FileHash")
        self.assertIn("Get-FileHash", baseline.stderr)
        self.assertEqual(fixed.returncode, 0, fixed.stderr)
        self.assertEqual(fixed.stdout.strip(), hashlib.sha256(b"tour").hexdigest().upper())


if __name__ == "__main__":
    unittest.main()
