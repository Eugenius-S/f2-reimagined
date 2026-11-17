"""The no-capture coverage report (task 303)."""
import subprocess
import sys
import unittest

from tour import HERE, coverage

DATA = {"turns": [-45, 0, 45], "spots": [
    {"map": "alpha", "elevation": 0, "tile": 100, "name": "gate"},
    {"map": "alpha", "elevation": 0, "tile": 200, "name": "well"},
    {"map": "alpha", "elevation": 1, "tile": 300, "name": "cellar"},
    {"map": "gamma", "elevation": 0, "tile": 400, "name": "tower"},
]}


class CoverageTests(unittest.TestCase):
    def test_partly_configured_two_map_request(self):
        lines = coverage(DATA, {"alpha", "beta"})
        self.assertIn("turns: -45 +0 +45", lines)
        self.assertIn("requested maps: alpha, beta", lines)
        self.assertIn("covered alpha e0: 2 spots x 3 turns = 6 views", lines)
        self.assertIn("  tile 100 gate", lines)
        self.assertIn("covered alpha e1: 1 spots x 3 turns = 3 views", lines)
        self.assertIn("missing beta: no configured spots", lines)
        self.assertIn("elevations: not checked, no expected map elevations given (--expect)", lines)
        self.assertFalse(any("gamma" in line for line in lines), "an unrequested map is in the report")

    def test_unknown_map(self):
        lines = coverage(DATA, {"nosuch"})
        self.assertIn("covered: none", lines)
        self.assertIn("missing nosuch: no configured spots", lines)

    def test_elevation_gap_in_an_expected_selection(self):
        lines = coverage(DATA, {"alpha"}, [("alpha", 0), ("alpha", 2)])
        self.assertIn("missing alpha e2: expected, no configured spots", lines)
        self.assertFalse(any(line.startswith("missing alpha e0") for line in lines))

    def test_fully_covered_selection(self):
        lines = coverage(DATA, {"alpha", "gamma"}, [("alpha", 0), ("alpha", 1), ("gamma", 0)])
        self.assertIn("missing: none", lines)
        self.assertFalse(any(line.startswith("elevations:") for line in lines))
        self.assertEqual(sum(line.startswith("covered ") for line in lines), 3)

    def test_every_configured_map_when_none_requested(self):
        lines = coverage(DATA, None)
        self.assertIn("requested maps: every configured map", lines)
        self.assertIn("covered gamma e0: 1 spots x 3 turns = 3 views", lines)
        self.assertIn("missing: none", lines)

    def test_command_line_report_needs_no_sides_or_game(self):
        run = subprocess.run([sys.executable, str(HERE / "tour.py"), "--coverage", "--maps", "kladwtwn,nosuch",
                              "--expect", "kladwtwn:0,kladwtwn:1"], capture_output=True, text=True)
        self.assertEqual(run.returncode, 0, run.stderr)
        self.assertIn("covered kladwtwn e0:", run.stdout)
        self.assertIn("missing nosuch: no configured spots", run.stdout)
        self.assertIn("missing kladwtwn e1: expected, no configured spots", run.stdout)


if __name__ == "__main__":
    unittest.main()
