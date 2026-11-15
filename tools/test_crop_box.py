import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

from PIL import Image


class CropBoxCliTests(unittest.TestCase):
    def test_crop_and_scale_preserve_pixel_positions(self):
        repo = Path(__file__).resolve().parents[1]
        source_pixels = [
            (255, 0, 0), (0, 255, 0), (0, 0, 255),
            (255, 255, 0), (255, 0, 255), (0, 255, 255),
        ]

        with tempfile.TemporaryDirectory() as temp_dir:
            root = Path(temp_dir)
            source = root / "source.png"
            with Image.new("RGB", (3, 2)) as image:
                image.putdata(source_pixels)
                image.save(source)

            scaled = root / "nested" / "scaled.png"
            subprocess.run(
                [sys.executable, str(repo / "tools" / "crop_box.py"),
                 str(source), "1", "0", "3", "2", str(scaled)],
                check=True,
                capture_output=True,
                text=True,
            )
            with Image.open(scaled) as image:
                self.assertEqual(image.size, (4, 4))
                self.assertEqual(
                    [image.getpixel((x, y)) for y in range(4) for x in range(4)],
                    [color for row in ((source_pixels[1], source_pixels[2]),
                                       (source_pixels[4], source_pixels[5]))
                     for _row in range(2) for color in row for _ in range(2)],
                )

            unscaled = root / "nested" / "unscaled.png"
            subprocess.run(
                [sys.executable, str(repo / "tools" / "crop_box.py"),
                 str(source), "1", "0", "3", "2", str(unscaled), "1"],
                check=True,
                capture_output=True,
                text=True,
            )
            with Image.open(unscaled) as image:
                self.assertEqual(image.size, (2, 2))
                self.assertEqual(
                    [image.getpixel((x, y)) for y in range(2) for x in range(2)],
                    [source_pixels[1], source_pixels[2],
                     source_pixels[4], source_pixels[5]],
                )


if __name__ == "__main__":
    unittest.main()
