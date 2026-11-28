"""274: an upper elevation's scene carries the lower elevations' wall
objects (the original 2D draws the lower scene through the upper floor's
void squares), visual only: they never join this floor's blocking. The
rows' serial numbers name the builder's combined object list."""

import sys
import unittest
from pathlib import Path

import numpy as np

sys.path.insert(0, str(Path(__file__).resolve().parent))
sys.path.insert(0, str(Path(__file__).resolve().parent / "vendor"))

from f2lib import ids
from f2lib.map import OBJECT_HIDDEN, MapFile, MapObject
from vendor.fp.scene import SceneBuilder

sys.dont_write_bytecode = True

OBJ_FIELDS = ("id", "tile", "x", "y", "sx", "sy", "frame", "rotation", "fid", "flags", "elevation",
              "pid", "cid", "light_distance", "light_intensity", "outline", "sid", "script_index")


def wall_obj(pid_index, tile, flags=0):
    obj = MapObject()
    for name in OBJ_FIELDS:
        setattr(obj, name, 0)
    obj.pid = ids.make_pid(ids.OBJ_TYPE_WALL, pid_index)
    obj.tile = tile
    obj.flags = flags
    obj.elevation = 0
    return obj


def scenery_obj(pid_index, tile):
    obj = wall_obj(pid_index, tile)
    obj.pid = ids.make_pid(ids.OBJ_TYPE_SCENERY, pid_index)
    return obj


def two_level_map():
    m = MapFile()
    m.name = "TEST.MAP"
    m.tiles = {0: np.zeros(10000, dtype=np.uint32), 1: np.zeros(10000, dtype=np.uint32)}
    e0_wall_a = wall_obj(10, 5000)
    e0_wall_b = wall_obj(11, 5001)
    e0_wall_hidden = wall_obj(12, 5002, flags=OBJECT_HIDDEN)
    e0_scenery = scenery_obj(13, 5003)
    e1_critter_like = scenery_obj(14, 5000)      # an upper-floor object at a wall hex
    m.objects[0] = [e0_wall_a, e0_wall_b, e0_wall_hidden, e0_scenery]
    m.objects[1] = [e1_critter_like]
    return m, e0_wall_a, e0_wall_b, e0_wall_hidden, e0_scenery, e1_critter_like


class UpperFloorTakesLowerWalls(unittest.TestCase):
    def test_e1_carries_the_lower_walls(self):
        m, wall_a, wall_b, hidden, scenery, upper = two_level_map()
        builder = SceneBuilder(None, m, elevation=1)
        self.assertIn(id(wall_a), builder.below_wall_ids)
        self.assertIn(id(wall_b), builder.below_wall_ids)
        self.assertNotIn(id(hidden), builder.below_wall_ids)
        self.assertNotIn(id(scenery), builder.below_wall_ids)
        # the walls below draw first, the floor's own objects after them
        self.assertEqual(builder.objects[0], wall_a)
        self.assertEqual(builder.objects[1], wall_b)
        self.assertIn(upper, builder.objects)
        self.assertNotIn(hidden, builder.objects)
        self.assertNotIn(scenery, builder.objects)
        # the serials name the combined list: the elevation's own objects
        # first, the walls below after them
        self.assertEqual(builder.serial[id(upper)], 0)
        self.assertEqual(builder.serial[id(wall_a)], 1)
        self.assertEqual(builder.serial[id(wall_b)], 2)
        self.assertEqual(len(builder.everything), 3)

    def test_ground_floor_stays_alone(self):
        m, wall_a, wall_b, hidden, scenery, upper = two_level_map()
        builder = SceneBuilder(None, m, elevation=0)
        self.assertEqual(builder.below_wall_ids, set())
        self.assertEqual(builder.everything, m.objects[0])
        # the ground floor's own walls are its walls, not "below" ones
        self.assertIn(wall_a, builder.objects)
        self.assertIn(scenery, builder.objects)
        self.assertNotIn(upper, builder.objects)

    def test_serials_cover_every_below_wall(self):
        m, wall_a, wall_b, hidden, scenery, upper = two_level_map()
        builder = SceneBuilder(None, m, elevation=1)
        for obj in builder.objects:
            self.assertIn(id(obj), builder.serial)
        self.assertEqual(len(builder.serial), len(builder.everything))


if __name__ == "__main__":
    unittest.main()
