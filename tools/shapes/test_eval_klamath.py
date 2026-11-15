"""Contract tests for the whole-map shape check and weighted score."""

import json
import sys
import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parent / 'eval'))

import klamath


class KlamathEvaluationTests(unittest.TestCase):
    def test_check_flags_lost_sides_but_keeps_good_and_unmeasured_views(self):
        entries = [{"frm": "box.frm"}, {"frm": "chair.frm"}]
        with tempfile.TemporaryDirectory() as temporary:
            output = Path(temporary)
            for frm in ("box.frm", "chair.frm"):
                for angle in (0, 90, 180, 270):
                    if frm == "chair.frm" and angle != 90:
                        continue
                    for root in ("shots", "empties"):
                        folder = output / root / frm / str(angle)
                        folder.mkdir(parents=True)
                        (folder / "shot.png").touch()

            def pixels(shot, _empty):
                angle = int(shot.parent.name)
                if shot.parts[-3] == "box.frm":
                    return {0: 1000, 90: 200, 180: 1000, 270: 800}[angle]
                return 1000

            with (patch.object(klamath, "OUT", output),
                  patch.object(klamath, "entries", return_value=entries),
                  patch.object(klamath, "thing_pixels", side_effect=pixels)):
                klamath.check(0.3)

            flags = json.loads((output / "flags.json").read_text())
            self.assertEqual(set(flags), {"box.frm", "chair.frm"})
            self.assertEqual(flags["box.frm"]["why"], "loses the thing from a side")
            self.assertEqual(flags["box.frm"]["shares_of_view0"], {"90": 0.2})
            self.assertEqual(flags["chair.frm"]["why"], "not measurable")

    def test_score_weights_cards_verdicts_flags_and_unjudged_shapes(self):
        data = [
            {"frm": "flat.frm", "count": 3, "today": ["card"]},
            {"frm": "pass.frm", "count": 2, "today": ["box"]},
            {"frm": "fail.frm", "count": 1, "today": ["box"]},
            {"frm": "lost.frm", "count": 4, "today": ["box"]},
            {"frm": "unmeasured.frm", "count": 5, "today": ["box"]},
            {"frm": "unknown.frm", "count": 6, "today": ["box"]},
        ]
        flags = {
            "fail.frm": {"why": "loses the thing from a side"},
            "lost.frm": {"why": "loses the thing from a side", "shares_of_view0": {"90": 0.2}},
            "unmeasured.frm": {"why": "not measurable"},
        }
        verdicts = {"pass.frm": {"verdict": "pass"}, "fail.frm": {"verdict": "fail", "why": "occluded"}}
        with tempfile.TemporaryDirectory() as temporary:
            output = Path(temporary)
            (output / "flags.json").write_text(json.dumps(flags))
            with (patch.object(klamath, "OUT", output),
                  patch.object(klamath, "rows", return_value=data),
                  patch.object(klamath, "verdict_data", return_value=verdicts),
                  patch("builtins.print") as printed):
                klamath.score()

        summary = printed.call_args_list[0].args[0]
        self.assertIn("weighted score 48% pass, 23.8% fail, 28.6% unjudged (1 FRMs)", summary)
        self.assertIn("failures: fail.frm (eye: occluded), lost.frm", printed.call_args_list[1].args[0])


if __name__ == "__main__":
    unittest.main()
