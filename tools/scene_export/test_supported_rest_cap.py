"""The round prop and the texture above it must stay one model when turned."""
import unittest

import solids


class SupportedRestCapTest(unittest.TestCase):
    def test_cap_is_horizontal_at_the_solid_top_not_an_upright_cross(self):
        projection = solids.Projection(
            {"squ": [-48, 12], "sqv": [32, 24], "origin": [3984, 610],
             "hpx": 32, "bbPx": 32, "rowTilt": 0, "stack": 0.01}
        )
        sprite = [0, 1513, 646, 27, 9, -13, -9]
        row = [0, 60.4655, 48.667, 1.3315, 0.0, 0]
        self.assertTrue(hasattr(solids, "supported_rest_cap"), "supported rim geometry not implemented")
        page, corners = solids.supported_rest_cap(projection, [sprite], row)
        self.assertEqual(page, 0)
        self.assertEqual(len(corners), 4)
        self.assertEqual({p[2] for p in corners}, {row[3] + solids.REST_CAP_OFFSET})
        self.assertEqual([(p[3], p[4]) for p in corners], [(1513, 655), (1540, 655), (1540, 646), (1513, 646)])
        px0, py0 = projection.ground_to_px(row[1], row[2], row[3] + solids.REST_CAP_OFFSET)
        expected = [(-13, 4.5), (14, 4.5), (14, -4.5), (-13, -4.5)]
        for point, (ex, ey) in zip(corners, expected):
            px, py = projection.ground_to_px(*point[:3])
            self.assertAlmostEqual(px - px0, ex)
            self.assertAlmostEqual(py - py0, ey)
            self.assertEqual(point[5:], [0, 0, 0])

        plane = row[2] + 0.025
        _, game_corners = solids.supported_rest_cap(projection, [sprite], row, plane)
        for fly, game in zip(corners, game_corners):
            self.assertEqual(fly[:5], game[:5])
            self.assertAlmostEqual(game[1] + game[6], plane)
            home = [game[i] + game[i + 5] for i in range(3)]
            for a, b in zip(projection.ground_to_px(*game[:3]), projection.ground_to_px(*home)):
                self.assertAlmostEqual(a, b)


if __name__ == "__main__":
    unittest.main()
