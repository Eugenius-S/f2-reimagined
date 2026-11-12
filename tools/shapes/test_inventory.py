"""Contract tests for inventory labels using exported scene row shapes."""

import unittest

from inventory import todays_shapes


def board(serial):
    return [0, 1.0, 2.0, 0, "", serial]


def prop(board_index, kind=0, wall_u=1.0, wall_v=1.0, top=1.0, seen=0):
    return [0, 1.0, 2.0, board_index, kind, 0.5, 0.5, 1.0, 1.0,
            0.0, top, wall_u, wall_v, "", 0, seen]


class TodaysShapesTests(unittest.TestCase):
    def test_empty_scene_has_no_shapes(self):
        self.assertEqual({}, todays_shapes({}))

    def test_exported_rows_classify_objects_by_serial_and_priority(self):
        scene = {
            "boards": [board(i) for i in range(100, 108)],
            "crossed": [0],
            "hidden": [1],
            "props": [
                prop(2),
                prop(3, wall_v=1.15),       # exactly at the cutoff stays a box
                prop(4, wall_v=1.16),       # a higher far edge forms a back
                prop(5, kind=1),            # round solids are drums
                prop(6, seen=2),            # game-camera duplicate is ignored
            ],
            "walls": [[0, 0, 0, 0, 0, 0, 0, "", 108]],
            "doors": [[0, 0, 0, 0, 0, 0, 0, "", 107]],
            "decals": [[0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 109]],
        }

        self.assertEqual({
            100: "card (donor: crossed)",
            101: "part of a solid",
            102: "box",
            103: "box",
            104: "box+back",
            105: "drum",
            106: "card",
            107: "card (door)",
            108: "panel",
            109: "flat",
        }, todays_shapes(scene))


if __name__ == "__main__":
    unittest.main()
