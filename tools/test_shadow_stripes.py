import unittest

from shadow_stripes import stripes


class ShadowStripeTests(unittest.TestCase):
    def test_flat_columns_have_no_transitions(self):
        result = stripes([0.8] * 4, [0.0] * 4, 4, 1, (0, 4, 0, 1), min_pixels=1)

        self.assertEqual(result, {
            "face_columns": 4,
            "worst_column_step": 0.0,
            "steps_over_threshold": 0,
            "longest_run_over_threshold": 0,
            "verdict": "PASS",
        })

    def test_alternating_columns_report_the_stripe_extent(self):
        # The four column means alternate between 0.2 and 0.8: each of the
        # three adjacent pairs differs by 0.6, forming one run of three.
        result = stripes([0.2, 0.8, 0.2, 0.8], [0.0] * 4, 4, 1,
                         (0, 4, 0, 1), min_pixels=1)

        self.assertEqual(result, {
            "face_columns": 4,
            "worst_column_step": 0.6,
            "steps_over_threshold": 3,
            "longest_run_over_threshold": 3,
            "verdict": "FAIL",
        })

    def test_no_usable_columns_return_none(self):
        self.assertIsNone(stripes([0.8] * 3, [1.0] * 3, 3, 1,
                                  (0, 3, 0, 1), min_pixels=1))
        self.assertIsNone(stripes([0.8] * 3, [0.0] * 3, 3, 1,
                                  (0, 3, 0, 1), min_pixels=2))


if __name__ == "__main__":
    unittest.main()
