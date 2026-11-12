import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent / 'eval'))

from PIL import Image, ImageDraw

from eval import sheets_differ


class SheetsDifferTests(unittest.TestCase):
    def test_different_dimensions_differ(self):
        self.assertTrue(sheets_differ(Image.new("RGB", (2, 2)), Image.new("RGB", (3, 2))))

    def test_identical_images_do_not_differ(self):
        image = Image.new("RGB", (8, 8), (40, 80, 120))
        self.assertFalse(sheets_differ(image, image.copy()))

    def test_channel_change_below_tolerance_does_not_differ(self):
        previous = Image.new("RGB", (10, 10), (20, 20, 20))
        new = previous.copy()
        ImageDraw.Draw(new).point([(x % 10, x // 10) for x in range(40)], fill=(27, 20, 20))
        self.assertFalse(sheets_differ(previous, new))

    def test_exactly_32_qualifying_pixels_do_not_differ(self):
        previous = Image.new("RGB", (10, 10), (20, 20, 20))
        new = previous.copy()
        ImageDraw.Draw(new).point([(x % 10, x // 10) for x in range(32)], fill=(28, 20, 20))
        self.assertFalse(sheets_differ(previous, new))

    def test_33_qualifying_pixels_differ(self):
        previous = Image.new("RGB", (10, 10), (20, 20, 20))
        new = previous.copy()
        ImageDraw.Draw(new).point([(x % 10, x // 10) for x in range(33)], fill=(28, 20, 20))
        self.assertTrue(sheets_differ(previous, new))


if __name__ == "__main__":
    unittest.main()


\r\n