import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

from PIL import Image


SCRIPT = Path(__file__).with_name("check_png_pixel.py")


class CheckPngPixelTests(unittest.TestCase):
    def test_cli_exit_codes(self):
        with tempfile.TemporaryDirectory() as directory:
            image_path = Path(directory) / "pixel.png"
            Image.new("RGB", (1, 1), (12, 34, 56)).save(image_path)

            cases = [
                ([image_path, 0, 0, 12, 34, 56], 0),
                ([image_path, 0, 0, 12, 34, 57], 1),
                ([image_path, 1, 0, 12, 34, 56], 1),
                ([], 2),
            ]
            for arguments, expected_code in cases:
                with self.subTest(arguments=arguments):
                    result = subprocess.run(
                        [sys.executable, str(SCRIPT), *(str(value) for value in arguments)],
                        capture_output=True,
                        text=True,
                        check=False,
                    )
                    self.assertEqual(result.returncode, expected_code, result.stderr)


if __name__ == "__main__":
    unittest.main()
