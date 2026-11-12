"""CLI tests for tools/image_diff.py using synthetic PNG files."""

import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

from PIL import Image


SCRIPT = Path(__file__).with_name("image_diff.py")


class ImageDiffCliTests(unittest.TestCase):
    def test_identical_images_report_no_changes_or_mask(self):
        with tempfile.TemporaryDirectory() as temp_dir:
            root = Path(temp_dir)
            first, second, mask = (root / name for name in ("first.png", "second.png", "mask.png"))
            Image.new("RGB", (3, 2), (12, 34, 56)).save(first)
            Image.new("RGB", (3, 2), (12, 34, 56)).save(second)

            result = subprocess.run(
                [sys.executable, str(SCRIPT), str(first), str(second), str(mask)],
                capture_output=True, text=True, check=False,
            )

            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
            self.assertIn("changed(>8): 0", result.stdout)
            self.assertFalse(mask.exists())

    def test_one_bright_pixel_reports_one_change_and_writes_mask(self):
        with tempfile.TemporaryDirectory() as temp_dir:
            root = Path(temp_dir)
            first, second, mask = (root / name for name in ("first.png", "second.png", "mask.png"))
            Image.new("RGB", (3, 2), (0, 0, 0)).save(first)
            changed = Image.new("RGB", (3, 2), (0, 0, 0))
            changed.putpixel((1, 1), (255, 255, 255))
            changed.save(second)

            result = subprocess.run(
                [sys.executable, str(SCRIPT), str(first), str(second), str(mask)],
                capture_output=True, text=True, check=False,
            )

            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
            self.assertIn("changed(>8): 1", result.stdout)
            self.assertTrue(mask.is_file())
            with Image.open(mask) as image:
                self.assertEqual(image.size, (3, 2))
                self.assertEqual(image.mode, "L")
                self.assertEqual(image.getpixel((1, 1)), 255)
                self.assertEqual(image.getpixel((0, 0)), 0)

    def test_different_sizes_fail_without_writing_mask(self):
        with tempfile.TemporaryDirectory() as temp_dir:
            root = Path(temp_dir)
            first, second, mask = (root / name for name in ("first.png", "second.png", "mask.png"))
            Image.new("RGB", (3, 2), (0, 0, 0)).save(first)
            Image.new("RGB", (2, 2), (0, 0, 0)).save(second)

            result = subprocess.run(
                [sys.executable, str(SCRIPT), str(first), str(second), str(mask)],
                capture_output=True, text=True, check=False,
            )

            self.assertEqual(result.returncode, 1)
            self.assertIn("sizes differ", result.stdout)
            self.assertFalse(mask.exists())


if __name__ == "__main__":
    unittest.main()
