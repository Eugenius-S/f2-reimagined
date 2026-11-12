"""CLI coverage for the 2D/3D snapshot gallery builder."""

import json
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

from PIL import Image


SCRIPT = Path(__file__).with_name("gallery_pairs.py")


class GalleryPairsCliTests(unittest.TestCase):
    def test_pairs_shared_views_case_insensitively_and_reports_unpaired_view(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            flat, scene, output = (root / name for name in ("flat", "scene", "out"))
            flat.mkdir()
            scene.mkdir()

            def snapshot(directory, filename, map_name, tile, color):
                Image.new("RGB", (8, 6), color).save(directory / filename)
                sidecar = {
                    "map": map_name,
                    "elevation": 2,
                    "view_tile": tile,
                    "image": filename,
                }
                (directory / f"devshot-{tile}.json").write_text(
                    json.dumps(sidecar), encoding="utf-8"
                )

            snapshot(flat, "flat.png", "ARROYO.MAP", "00ab", (240, 20, 20))
            snapshot(scene, "scene.png", "arroyo.map", "00ab", (20, 30, 240))
            snapshot(flat, "solo.png", "ARROYO.MAP", "00cd", (20, 240, 20))

            result = subprocess.run(
                [sys.executable, str(SCRIPT), str(flat), str(scene), str(output)],
                capture_output=True,
                text=True,
                check=False,
            )

            self.assertEqual(result.returncode, 0, result.stderr)
            self.assertEqual(result.stdout.splitlines(), ["arroyo-e2-00ab.jpg"])
            self.assertEqual(
                result.stderr.splitlines(), ["only in one tour: ('arroyo', 2, '00cd')"]
            )
            files = list(output.iterdir())
            self.assertEqual([path.name for path in files], ["arroyo-e2-00ab.jpg"])
            with Image.open(files[0]) as pair:
                self.assertEqual(pair.format, "JPEG")
                self.assertEqual(pair.size, (8, 3))
                rgb = pair.convert("RGB")
                self.assertGreater(rgb.getpixel((1, 1))[0], 200)
                self.assertLess(rgb.getpixel((1, 1))[2], 60)
                self.assertGreater(rgb.getpixel((6, 1))[2], 200)
                self.assertLess(rgb.getpixel((6, 1))[0], 60)


if __name__ == "__main__":
    unittest.main()
