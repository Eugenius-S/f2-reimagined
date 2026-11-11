import hashlib
import shutil
import subprocess
import tempfile
import unittest
from pathlib import Path

from tour import powershell_args


@unittest.skipUnless(shutil.which("powershell"), "Windows PowerShell is required")
class PowerShellArgsTests(unittest.TestCase):
    def test_hash_cmdlet_is_available_in_snapshot_process(self):
        with tempfile.TemporaryDirectory() as directory:
            sample = Path(directory) / "sample.txt"
            sample.write_text("tour", encoding="utf-8")
            command = (f"Get-FileHash -LiteralPath '{sample}' -Algorithm SHA256 "
                       "| Select-Object -ExpandProperty Hash")

            result = subprocess.run(powershell_args(command), capture_output=True, text=True,
                                    errors="replace")

        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(result.stdout.strip(), hashlib.sha256(b"tour").hexdigest().upper())


if __name__ == "__main__":
    unittest.main()
