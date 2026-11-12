import base64
import struct
import unittest

import gallery_spots


def scene(floor_indices=(), roof_indices=()):
    floor = [0] * (gallery_spots.GRID * gallery_spots.GRID)
    roof = [0] * len(floor)
    for index in floor_indices:
        floor[index] = 1
    for index in roof_indices:
        roof[index] = 1
    return {
        "floor": base64.b64encode(struct.pack(f"<{len(floor)}H", *floor)).decode("ascii"),
        "roof": base64.b64encode(struct.pack(f"<{len(roof)}H", *roof)).decode("ascii"),
    }


class GallerySpotsTest(unittest.TestCase):
    def test_roof_patches_use_four_neighbors_without_row_wrap(self):
        roof = [0] * (gallery_spots.GRID * gallery_spots.GRID)
        roof[10 * gallery_spots.GRID + 10] = 1
        roof[10 * gallery_spots.GRID + 11] = 1
        roof[11 * gallery_spots.GRID + 11] = 1
        roof[20 * gallery_spots.GRID + 20] = 1
        roof[21 * gallery_spots.GRID + 21] = 1  # diagonal only
        roof[5 * gallery_spots.GRID + 99] = 1
        roof[6 * gallery_spots.GRID] = 1  # adjacent in memory, opposite map edges

        patches = gallery_spots.roof_patches(roof)

        self.assertEqual(
            [[599], [600], [1010, 1011, 1111], [2020], [2121]],
            sorted(sorted(patch) for patch in patches),
        )

    def test_spots_choose_large_roof_centres_and_limit_selections(self):
        roof = []
        for row in range(0, 70, 10):
            for col in range(0, 70, 10):
                roof.extend(row * gallery_spots.GRID + col + offset for offset in (0, 1, 2, 3, 4, 5))
        selected = gallery_spots.spots(scene(roof_indices=roof))

        self.assertEqual(gallery_spots.MAX_VIEWS, len(selected))
        self.assertEqual(
            [207, 227, 247, 267, 287, 307],
            selected,
        )

    def test_spots_accept_six_square_roof_and_reject_five_square_roof(self):
        five_square_roof = [80 * gallery_spots.GRID + offset for offset in range(5)]
        self.assertEqual([], gallery_spots.spots(scene(roof_indices=five_square_roof)))

        six_square_roof = [90 * gallery_spots.GRID + offset for offset in range(6)]
        self.assertEqual([36207], gallery_spots.spots(scene(roof_indices=six_square_roof)))

    def test_floor_grid_selects_expected_centres(self):
        floor = (12 * gallery_spots.GRID + 14, 12 * gallery_spots.GRID + 34,
                 32 * gallery_spots.GRID + 14, 32 * gallery_spots.GRID + 34)
        self.assertEqual(
            [5029, 5069, 13029, 13069],
            gallery_spots.spots(scene(floor_indices=floor)),
        )

    def test_square_to_tile_returns_engine_coordinates(self):
        self.assertEqual(201, gallery_spots.square_to_tile(0))
        self.assertEqual(5015, gallery_spots.square_to_tile(1207))


if __name__ == "__main__":
    unittest.main()
