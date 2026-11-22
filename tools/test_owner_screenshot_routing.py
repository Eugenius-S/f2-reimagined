"""Guard the shared owner screenshot route until a graphical playtest runs."""
from pathlib import Path
import re
import unittest

SOURCE = Path(__file__).resolve().parents[1] / "ce" / "src"


class OwnerScreenshotRoutingTest(unittest.TestCase):
    def test_plain_f12_runs_before_modal_dispatch_and_preserves_region_mark(self):
        source = (SOURCE / "input.cc").read_text(encoding="utf-8")
        shared = source.split("int inputGetInput()", 1)[1].split("void get_input_position", 1)[0]
        route = re.search(
            r"if\s*\(v3 == KEY_F12 && !owner_shot::markMode\(\)\)\s*"
            r"\{\s*owner_shot::handleKey\(v3\);\s*return -1;",
            shared,
        )
        self.assertIsNotNone(route)
        self.assertLess(route.start(), shared.index("_GNW_check_menu_bars(v3)"))

    def test_modal_loops_do_not_have_separate_legacy_f12_paths(self):
        for name in ("automap", "options", "pipboy", "preferences"):
            with self.subTest(menu=name):
                source = (SOURCE / (name + ".cc")).read_text(encoding="utf-8")
                self.assertIn("inputGetInput()", source)
                self.assertNotIn("KEY_F12", source)


if __name__ == "__main__":
    unittest.main()
