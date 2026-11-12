import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

from PIL import Image


SCRIPT = Path(__file__).with_name("patch_pixel.py")


class PatchPixelTests(unittest.TestCase):
    def test_cli_averages_patch_and_reports_location(self):
        with tempfile.TemporaryDirectory() as directory:
            image_path = Path(directory) / "uneven.png"
            image = Image.new("RGB", (3, 3))
            image.putdata(
                [
                    (0, 1, 2), (3, 4, 5), (6, 7, 8),
                    (9, 10, 11), (12, 13, 14), (15, 16, 17),
                    (18, 19, 20), (21, 22, 23), (24, 25, 26),
                ]
            )
            image.save(image_path)

            cases = [
                ([image_path, 1, 1, 1], "uneven.png @(1,1) r1: rgb(12, 13, 14)"),
                ([image_path, 1, 1, 0], "uneven.png @(1,1) r0: rgb(12, 13, 14)"),
            ]
            for arguments, expected_output in cases:
                with self.subTest(radius=arguments[-1]):
                    result = subprocess.run(
                        [sys.executable, str(SCRIPT), *(str(value) for value in arguments)],
                        capture_output=True,
                        text=True,
                        check=False,
                    )
                    self.assertEqual(result.returncode, 0, result.stderr)
                    self.assertEqual(result.stdout.strip(), expected_output)


if __name__ == "__main__":
    unittest.main()
