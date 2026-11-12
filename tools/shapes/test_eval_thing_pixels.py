"""Tests for counting an object's pixels against its empty twin."""

import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent / "eval"))

from PIL import Image, ImageDraw

from eval import thing_pixels


class ThingPixelsTests(unittest.TestCase):
    def test_counts_pixels_at_or_above_channel_threshold(self):
        with tempfile.TemporaryDirectory() as temp:
            thing_path = Path(temp) / "thing.png"
            empty_path = Path(temp) / "empty.png"
            empty = Image.new("RGB", (4, 1), (40, 40, 40))
            thing = empty.copy()
            draw = ImageDraw.Draw(thing)
            draw.point((0, 0), fill=(48, 40, 40))  # exactly at channel threshold
            draw.point((1, 0), fill=(49, 40, 40))  # above channel threshold
            draw.point((2, 0), fill=(47, 40, 40))  # below channel threshold
            thing.save(thing_path)
            empty.save(empty_path)

            self.assertEqual(thing_pixels(thing_path, empty_path), 2)

    def test_rejects_shots_with_different_dimensions(self):
        with tempfile.TemporaryDirectory() as temp:
            thing_path = Path(temp) / "thing.png"
            empty_path = Path(temp) / "empty.png"
            Image.new("RGB", (2, 2)).save(thing_path)
            Image.new("RGB", (3, 2)).save(empty_path)

            with self.assertRaisesRegex(SystemExit, "differ in size"):
                thing_pixels(thing_path, empty_path)


if __name__ == "__main__":
    unittest.main()
