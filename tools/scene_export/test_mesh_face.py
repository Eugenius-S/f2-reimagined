"""276: a wall face whose picture is mostly see-through is a mesh (the Den's
chain-link) and keeps its holes: filling them made the black wall with
specks. A solid wall's picture is mostly opaque and fills as before (115)."""

import sys
import unittest
from pathlib import Path

import numpy as np

sys.path.insert(0, str(Path(__file__).resolve().parent))
sys.path.insert(0, str(Path(__file__).resolve().parent / "vendor"))
sys.dont_write_bytecode = True

from vendor.fp.scene import mesh_face


def face(opaque_share, shape=(40, 40)):
    """RGBA with `opaque_share` of the texels opaque, the rest the holes."""
    rgba = np.zeros((*shape, 4), np.uint8)
    count = int(opaque_share * rgba[..., 3].size)
    flat = rgba[..., 3].reshape(-1)
    flat[:count] = 255
    return rgba


class MeshFacesKeepTheirHoles(unittest.TestCase):
    def test_chain_link_is_a_mesh(self):
        # the fence FRMs measure 42-58% opaque
        for share in (0.42, 0.5, 0.58):
            self.assertTrue(mesh_face(face(share)), f"{share:.0%} opaque reads as a mesh")

    def test_solid_walls_fill_as_before(self):
        # the solid walls measure 90%+ opaque
        for share in (0.9, 0.96, 1.0):
            self.assertFalse(mesh_face(face(share)), f"{share:.0%} opaque reads as a solid wall")

    def test_the_threshold_sits_between_the_two_classes(self):
        self.assertTrue(mesh_face(face(0.69)))
        self.assertFalse(mesh_face(face(0.71)))

    def test_fully_transparent_picture_is_a_mesh(self):
        self.assertTrue(mesh_face(face(0.0)))


if __name__ == "__main__":
    unittest.main()
