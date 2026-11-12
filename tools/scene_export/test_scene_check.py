"""Tests for scene metadata validation."""

import unittest

from scene_check import one_map_elevation


class OneMapElevationTests(unittest.TestCase):
    def test_valid_map_names_and_elevations(self):
        for name in ("klamath", "vault_13", "map2"):
            for elevation in (0, 1, 2):
                with self.subTest(name=name, elevation=elevation):
                    self.assertIs(one_map_elevation({"map": name, "elevation": elevation}), True)

    def test_invalid_map_names(self):
        for name in ("../klamath", "klamath/n", "Klamath", ""):
            with self.subTest(name=name):
                self.assertIs(one_map_elevation({"map": name, "elevation": 0}), False)

    def test_invalid_or_missing_elevation(self):
        for scene in (
            {"map": "klamath"},
            {"map": "klamath", "elevation": -1},
            {"map": "klamath", "elevation": 3},
        ):
            with self.subTest(scene=scene):
                self.assertIs(one_map_elevation(scene), False)


if __name__ == "__main__":
    unittest.main()
