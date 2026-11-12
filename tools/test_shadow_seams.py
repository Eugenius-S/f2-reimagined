"""Regression tests for left-out faces in shadow seam measurements."""

import unittest

import numpy as np

from shadow_seams import measure


class ShadowSeamsTests(unittest.TestCase):
    def test_enclosed_lit_and_shadow_components_are_counted(self):
        factor = np.zeros((40, 40), dtype=float)
        ignored = np.zeros((40, 40), dtype=bool)
        card = np.zeros((40, 40), dtype=bool)

        factor[8:22, 8:22] = 1.0
        factor[12:14, 12:14] = 0.0
        factor[29:31, 29:31] = 1.0
        found = measure(factor, ignored, card, area=12, edge=4)

        self.assertEqual(int(found["holes"].sum()), 4)
        self.assertEqual(int(found["specks"].sum()), 4)

    def test_lit_region_open_to_ignored_face_is_not_a_hole(self):
        factor = np.ones((40, 40), dtype=float)
        ignored = np.zeros((40, 40), dtype=bool)
        card = np.zeros((40, 40), dtype=bool)

        ignored[15:25, 20:25] = True
        factor[18:22, 17:20] = 0.0

        unmasked = measure(factor, np.zeros_like(ignored), card, area=12, edge=4)
        found = measure(factor, ignored, card, area=12, edge=4)

        self.assertEqual(int(unmasked["holes"].sum()), 12)
        self.assertEqual(int(found["holes"].sum()), 0)


if __name__ == "__main__":
    unittest.main()
