"""Tests for the whole-game shape sweep without game data."""
import contextlib
import io
import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch

import sweep_all


class SweepAllTests(unittest.TestCase):
    def test_maps_and_elevations_are_swept_and_failures_are_reported(self):
        game_files = unittest.mock.Mock()
        game_files.glob.return_value = ["maps/ZED.MAP", "maps/alpha.map"]
        builds = []
        build_calls = []

        class Builder:
            def __init__(self, _game_files, map_name, elevation):
                self.map_name = map_name
                self.elevation = elevation
                builds.append((map_name, elevation))
                if (map_name, elevation) == ("alpha", 1):
                    raise ValueError("bad elevation")
                if (map_name, elevation) == ("zed", 2):
                    raise SystemExit("unsupported elevation")
                self.props = [1, 2]
                self.rests = [3]

            def build_tiles(self):
                build_calls.append(("tiles", self.map_name, self.elevation))

            def build_objects(self):
                build_calls.append(("objects", self.map_name, self.elevation))

        output = io.StringIO()
        with patch.object(sweep_all, "ReadOnlyGameFiles", return_value=game_files), \
                patch.object(sweep_all, "SceneBuilder", Builder), \
                patch.object(sweep_all.sys, "argv", ["sweep_all.py"]), \
                contextlib.redirect_stdout(output):
            sweep_all.main()

        self.assertEqual(
            builds,
            [(name, elevation) for name in ("alpha", "zed") for elevation in (0, 1, 2)],
        )
        successful_builds = [("alpha", 0), ("alpha", 2), ("zed", 0), ("zed", 1)]
        self.assertEqual(
            build_calls,
            [
                (method, map_name, elevation)
                for map_name, elevation in successful_builds
                for method in ("tiles", "objects")
            ],
        )
        self.assertIn("2 map files seen", output.getvalue())
        self.assertIn("alpha e0: OK props=2 rests=1", output.getvalue())
        self.assertIn("alpha e1: ValueError: bad elevation", output.getvalue())
        self.assertIn("zed e2: SystemExit: unsupported elevation", output.getvalue())
        self.assertIn("done: 4 ok, 2 failed", output.getvalue())
        game_files.close.assert_called_once_with()

    def test_named_output_is_closed_after_sweep(self):
        game_files = unittest.mock.Mock()
        game_files.glob.return_value = []
        opened = []
        real_open = open

        def tracked_open(*args, **kwargs):
            stream = real_open(*args, **kwargs)
            opened.append(stream)
            return stream

        with tempfile.TemporaryDirectory() as temp_dir:
            output_path = Path(temp_dir) / "sweep.txt"
            with patch.object(sweep_all, "ReadOnlyGameFiles", return_value=game_files), \
                    patch.object(sweep_all.sys, "argv", ["sweep_all.py", str(output_path)]), \
                    patch("builtins.open", side_effect=tracked_open):
                sweep_all.main()

            self.assertTrue(opened[0].closed)
            self.assertEqual(output_path.read_text(encoding="utf-8"), "0 map files seen\ndone: 0 ok, 0 failed\n")
            game_files.close.assert_called_once_with()


if __name__ == "__main__":
    unittest.main()
