import hashlib
import re
import shutil
import subprocess
import tempfile
import unittest
from pathlib import Path

from tour import ROOT, check_views, powershell_args


class TourViewTests(unittest.TestCase):
    def test_accepts_different_images_for_valid_3d_turns(self):
        from PIL import Image

        with tempfile.TemporaryDirectory() as directory:
            folder = Path(directory)
            Image.new("RGB", (8, 8), (20, 40, 60)).save(folder / "000-start.png")
            Image.new("RGB", (8, 8), (20, 40, 60)).save(folder / "001-turn-0.png")
            Image.new("RGB", (8, 8), (220, 180, 140)).save(folder / "002-turn-90.png")
            for name, turn in (("001-turn-0.png", 0), ("002-turn-90.png", 90)):
                (folder / f"{name}.json").write_text(
                    '{"image": "' + name + '", "map": "klamath", "elevation": 0, '
                    '"view_tile": 123, "scene_3d_enabled": true, "scene_view": "full", '
                    '"renderer": "scene-gl", "turn": ' + str(turn) + '}',
                    encoding="utf-8",
                )

            check_views(folder, "klamath", 0, [{"tile": 123}], [0, 90])

    def test_rejects_identical_images_for_different_turns(self):
        from PIL import Image

        with tempfile.TemporaryDirectory() as directory:
            folder = Path(directory)
            image = Image.new("RGB", (8, 8), (20, 40, 60))
            image.save(folder / "000-start.png")
            image.save(folder / "001-turn-0.png")
            image.save(folder / "002-turn-90.png")
            for name, turn in (("001-turn-0.png", 0), ("002-turn-90.png", 90)):
                (folder / f"{name}.json").write_text(
                    '{"image": "' + name + '", "map": "klamath", "elevation": 0, '
                    '"view_tile": 123, "scene_3d_enabled": true, "scene_view": "full", '
                    '"renderer": "scene-gl", "turn": ' + str(turn) + '}',
                    encoding="utf-8",
                )

            with self.assertRaisesRegex(SystemExit, "every turn is the same picture"):
                check_views(folder, "klamath", 0, [{"tile": 123}], [0, 90])


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
