"""The 297 fixture generator: everything runs without any export.

Light checks only: the generated tree parses back through the same readers
the exporter uses, and the bundled-resource precondition catches a missing
resource by its path. The real source/packaged export halves live in
fixture_smoke.py and run in the publish job or a heavy slot.
"""

import sys
import tempfile
import unittest
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
sys.path.insert(0, str(HERE / "vendor"))

from fixture import BUNDLED_RESOURCES, build_fixture, check_fixture  # noqa: E402
from f2lib import map as map_lib  # noqa: E402
from f2lib.frm import Frm  # noqa: E402


class FixtureTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(prefix="f2r-fixture-")
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name) / "game"
        build_fixture(self.root)

    def test_every_bundled_resource_stands(self):
        self.assertEqual(check_fixture(self.root), [])

    def test_map_round_trips_through_the_exporter_reader(self):
        from f2lib import GameFiles
        gf = GameFiles(base=str(self.root))
        game_map = map_lib.MapFile.load("fixture", gf)
        self.assertIn(0, game_map.tiles)
        self.assertEqual(len(game_map.tiles[0]), 10000)
        self.assertEqual(game_map.base_name.lower(), "fixture")
        self.assertEqual(game_map.script_index, 0)

    def test_tile_frm_round_trips(self):
        art = Frm.from_bytes((self.root / "art/tiles/fixtile.frm").read_bytes())
        self.assertEqual(art.stored[0][0].width, 80)
        self.assertEqual(art.stored[0][0].height, 36)
        # The pixels are deterministic, not copied from anywhere.
        self.assertEqual(art.stored[0][0].pixels[0:4], bytes((i * 5) % 251 for i in range(4)))

    def test_color_pal_has_the_full_lut(self):
        data = (self.root / "color.pal").read_bytes()
        self.assertGreaterEqual(len(data), 768 + 32768)

    def test_missing_resource_fails_with_its_path(self):
        (self.root / "scripts/scripts.lst").unlink()
        failures = check_fixture(self.root)
        self.assertEqual(len(failures), 1)
        self.assertIn("scripts/scripts.lst", failures[0])
        (self.root / "art/tiles/fixtile.frm").unlink()
        failures = check_fixture(self.root)
        self.assertEqual(len(failures), 2)
        self.assertTrue(any("art/tiles/fixtile.frm" in f for f in failures))

    def test_no_copyrighted_names_in_the_tree(self):
        # The whole fixture is synthetic: every file name the tree carries
        # comes from this generator, not from the game's own data.
        names = {str(p.relative_to(self.root)).replace("\\", "/") for p in self.root.rglob("*") if p.is_file()}
        self.assertTrue(names <= set(BUNDLED_RESOURCES), names - set(BUNDLED_RESOURCES))


if __name__ == "__main__":
    unittest.main()
