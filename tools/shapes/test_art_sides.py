import unittest
from types import SimpleNamespace
from unittest.mock import patch

import art_sides


class FakeGameFiles:
    def __init__(self):
        self.closed = False

    def art_list(self, obj_type):
        return {
            0: [None, "single.frm", "MULTI.FRM", "missing.frm", "notes.txt"],
            2: ["Scenery.FRM"],
        }[obj_type]

    def exists(self, path):
        return not path.endswith("missing.frm")

    def read(self, path):
        return path

    def close(self):
        self.closed = True


class ArtSidesTests(unittest.TestCase):
    def test_returns_multidirection_items_and_scenery(self):
        game_files = FakeGameFiles()

        def parse(path):
            return {
                "art/items/single.frm": SimpleNamespace(direction_count=1, direction_map=[0]),
                "art/items/MULTI.FRM": SimpleNamespace(direction_count=2, direction_map=[0, 1]),
                "art/scenery/Scenery.FRM": SimpleNamespace(direction_count=3, direction_map=[0, 0, 1]),
            }[path]

        with patch.object(art_sides, "ReadOnlyGameFiles", return_value=game_files), patch.object(
            art_sides.Frm, "from_bytes", side_effect=parse
        ):
            result = art_sides.sides("unused")

        self.assertEqual(
            result,
            [
                ("items", 2, "multi.frm", 2, [0, 1]),
                ("scenery", 0, "scenery.frm", 3, [0, 0, 1]),
            ],
        )
        self.assertTrue(game_files.closed)

    def test_closes_game_files_when_frm_parsing_raises(self):
        game_files = FakeGameFiles()
        with patch.object(art_sides, "ReadOnlyGameFiles", return_value=game_files), patch.object(
            art_sides.Frm, "from_bytes", side_effect=ValueError("bad FRM")
        ):
            with self.assertRaisesRegex(ValueError, "bad FRM"):
                art_sides.sides("unused")

        self.assertTrue(game_files.closed)


if __name__ == "__main__":
    unittest.main()
