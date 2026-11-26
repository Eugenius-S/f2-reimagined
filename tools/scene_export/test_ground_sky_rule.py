"""Task 285: the ground heuristic must not call a map indoor when its own script
carries a day level and a night level (set_light_level literals).

The old rule (commonest floor tile darker than DARK_TILE and no roofs) sent
NCR 4's desert, the slavers' camp and the Car out of gas underground, so the
3D never dimmed them at night while the classic did. The literals are the
game's own proof the map lies under the sky.
"""
import sys
import unittest
from pathlib import Path
from types import SimpleNamespace

import numpy as np

HERE = Path(__file__).resolve().parent
sys.path[:0] = [str(HERE), str(HERE / "vendor")]

from vendor.fp import scene as fp_scene


def builder(grey, levels):
    """A stub with one commonest tile painted `grey` (0..255) and the given
    script literals; build_ground reads everything else off the stub."""
    sheet = np.zeros((200, 200, 4), np.uint8)
    cell = sheet[fp_scene.TILE_MARGIN:fp_scene.TILE_MARGIN + fp_scene.TILE_RES,
                 fp_scene.TILE_MARGIN:fp_scene.TILE_MARGIN + fp_scene.TILE_RES]
    cell[..., :3] = grey
    cell[..., 3] = 255
    gf = SimpleNamespace(scripts=SimpleNamespace(name=lambda index: "testscript" if index >= 0 else None),
                         read=lambda path: b"" if levels is None else
                         b"".join(b"\xC0\x01" + int(v).to_bytes(4, "big", signed=True) + b"\x80\xE9" for v in levels))
    stub = SimpleNamespace(
        floor=np.array([2 if False else 3] * 4),   # the commonest floor id is 3
        roof=np.zeros(4, np.uint32),               # no roofs anywhere
        tile_cells={2: (0, 0), 3: (fp_scene.TILE_MARGIN + fp_scene.TILE_RES, 0)},
        gf=gf,
        map=SimpleNamespace(script_index=1),
    )
    stub.script_light_levels = lambda: set(levels or [])
    return stub, sheet


class GroundSkyRuleTests(unittest.TestCase):
    def test_a_dark_roofless_map_with_a_day_and_a_night_level_stays_outdoor(self):
        stub, sheet = builder(20, [49, 100])
        fp_scene.SceneBuilder.build_ground(stub, sheet)
        self.assertFalse(stub.indoor)

    def test_a_dark_roofless_map_with_one_pinned_level_stays_underground(self):
        stub, sheet = builder(20, [49])
        fp_scene.SceneBuilder.build_ground(stub, sheet)
        self.assertTrue(stub.indoor)

    def test_a_dark_roofless_map_without_any_level_stays_underground(self):
        stub, sheet = builder(20, [])
        fp_scene.SceneBuilder.build_ground(stub, sheet)
        self.assertTrue(stub.indoor)

    def test_two_close_levels_without_a_day_one_do_not_prove_the_sky(self):
        # the military bases' [49, 50] pair: no bright day level, still underground
        stub, sheet = builder(20, [49, 50])
        fp_scene.SceneBuilder.build_ground(stub, sheet)
        self.assertTrue(stub.indoor)

    def test_a_bright_floor_is_outdoor_whatever_the_script_says(self):
        stub, sheet = builder(200, [49])
        fp_scene.SceneBuilder.build_ground(stub, sheet)
        self.assertFalse(stub.indoor)


if __name__ == "__main__":
    unittest.main()
