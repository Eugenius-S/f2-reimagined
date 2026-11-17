import hashlib
import json
import re
import shutil
import subprocess
import tempfile
import unittest
from unittest import mock
from pathlib import Path

from tour import ROOT, check_views, powershell_args, side_kind


class SideKindTests(unittest.TestCase):
    def test_accepts_checkout_and_scene_cache(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            checkout = root / "checkout"
            (checkout / "build").mkdir(parents=True)
            (checkout / "build" / "fallout2-ce.exe").touch()
            (checkout / "tools").mkdir()
            (checkout / "tools" / "dev-snapshot.ps1").touch()

            cache = root / "cache"
            scene = cache / "klamath" / "0"
            scene.mkdir(parents=True)
            (scene / "scene.json").touch()

            self.assertEqual(side_kind(str(checkout)), ("checkout", checkout.resolve()))
            self.assertEqual(side_kind(str(cache)), ("cache", cache.resolve()))

    def test_rejects_directory_that_is_neither_checkout_nor_cache(self):
        with tempfile.TemporaryDirectory() as directory:
            with self.assertRaisesRegex(SystemExit, "neither a built checkout nor a scene cache"):
                side_kind(directory)


class TourViewTests(unittest.TestCase):
    def test_accepts_different_images_for_valid_3d_turns(self):
        from PIL import Image

        with tempfile.TemporaryDirectory() as directory:
            folder = Path(directory)
            Image.new("RGB", (8, 8), (20, 40, 60)).save(folder / "000-start.png")
            Image.new("RGB", (8, 8), (20, 40, 60)).save(folder / "001-turn-0.png")
            Image.new("RGB", (8, 8), (220, 180, 140)).save(folder / "002-turn-90.png")
            for name, turn in (("001-turn-0.png", 0), ("002-turn-90.png", 90)):
                (folder / f"{name}.json").write_text(
                    '{"image": "' + name + '", "map": "klamath", "elevation": 0, '
                    '"view_tile": 123, "scene_3d_enabled": true, "scene_view": "full", '
                    '"renderer": "scene-gl", "turn": ' + str(turn) + '}',
                    encoding="utf-8",
                )

            check_views(folder, "klamath", 0, [{"tile": 123}], [0, 90])

    def test_rejects_identical_images_for_different_turns(self):
        from PIL import Image

        with tempfile.TemporaryDirectory() as directory:
            folder = Path(directory)
            image = Image.new("RGB", (8, 8), (20, 40, 60))
            image.save(folder / "000-start.png")
            image.save(folder / "001-turn-0.png")
            image.save(folder / "002-turn-90.png")
            for name, turn in (("001-turn-0.png", 0), ("002-turn-90.png", 90)):
                (folder / f"{name}.json").write_text(
                    '{"image": "' + name + '", "map": "klamath", "elevation": 0, '
                    '"view_tile": 123, "scene_3d_enabled": true, "scene_view": "full", '
                    '"renderer": "scene-gl", "turn": ' + str(turn) + '}',
                    encoding="utf-8",
                )

            with self.assertRaisesRegex(SystemExit, "every turn is the same picture"):
                check_views(folder, "klamath", 0, [{"tile": 123}], [0, 90])


class TourSidecarTests(unittest.TestCase):
    def test_rejects_scene_shot_from_the_wrong_view_tile(self):
        from PIL import Image
        from tour import check_views

        with tempfile.TemporaryDirectory() as directory:
            folder = Path(directory)
            spot = {"tile": 12000}
            Image.new("RGB", (2, 2), (0, 0, 0)).save(folder / "00-start.png")
            for index, tile in enumerate((12000, 12001)):
                image_name = f"{index + 1:02d}-shot.png"
                Image.new("RGB", (2, 2), (index * 50, 0, 0)).save(folder / image_name)
                metadata = {
                    "image": image_name,
                    "map": "klamath",
                    "elevation": 0,
                    "view_tile": tile,
                    "scene_3d_enabled": True,
                    "scene_view": "full",
                    "renderer": "scene-gl",
                }
                (folder / f"{index + 1:02d}-shot.json").write_text(json.dumps(metadata), encoding="utf-8")

            with self.assertRaises(SystemExit) as error:
                check_views(folder, "klamath", 0, [spot], [0, 1])
            self.assertIn("tile 12000 turn +1: shot at klamath e0 tile 12001", str(error.exception))

            metadata_path = folder / "02-shot.json"
            metadata = json.loads(metadata_path.read_text(encoding="utf-8"))
            metadata["view_tile"] = 12000
            metadata_path.write_text(json.dumps(metadata), encoding="utf-8")
            check_views(folder, "klamath", 0, [spot], [0, 1])


SPOTS = [{"tile": 100, "name": "gate"}, {"tile": 200, "name": "well"}]
TURNS = [0, 90]


def write_views(folder, map_name="klamath", elevation=0, spots=SPOTS, turns=TURNS):
    """A valid saved tour as dev-snapshot leaves it: a start shot, then one
    PNG and sidecar per spot and turn, each view a different picture."""
    from PIL import Image
    folder.mkdir(parents=True, exist_ok=True)
    Image.new("RGB", (40, 30), (5, 5, 5)).save(folder / "devshot-000.png")
    index = 0
    for spot in spots:
        for _turn in turns:
            index += 1
            name = f"devshot-{index:03d}.png"
            Image.new("RGB", (40, 30), (40 * index % 256, 120, 200 - 30 * index)).save(folder / name)
            (folder / f"devshot-{index:03d}.json").write_text(json.dumps({
                "image": name, "map": map_name.upper() + ".MAP", "elevation": elevation,
                "view_tile": spot["tile"], "scene_3d_enabled": True, "scene_view": "full",
                "renderer": "scene-gl"}), encoding="utf-8")


def wrong_tile(folder):
    path = folder / "devshot-003.json"
    data = json.loads(path.read_text(encoding="utf-8"))
    data["view_tile"] = 999
    path.write_text(json.dumps(data), encoding="utf-8")


DEFECTS = {
    "missing sidecar": (lambda folder: (folder / "devshot-002.json").unlink(), "no sidecar"),
    "wrong tile": (wrong_tile, r"tile 200 turn \+0: shot at"),
    "corrupt png": (lambda folder: (folder / "devshot-003.png").write_bytes(b"not a png"), "not a readable PNG"),
    "incomplete turns": (lambda folder: [(folder / f"devshot-004.{ext}").unlink() for ext in ("png", "json")],
                         "3 views for 4 stops"),
}


class SavedTourTests(unittest.TestCase):
    """A saved tour counts only if it passes the same checks as a fresh one (task 296)."""

    def setUp(self):
        import tour
        self.tour = tour
        self.directory = tempfile.TemporaryDirectory()
        root = Path(self.directory.name)
        self.cache = root / "cache"
        (self.cache / "klamath" / "0").mkdir(parents=True)
        (self.cache / "klamath" / "0" / "scene.json").write_text("{}", encoding="utf-8")
        self.groups = {("klamath", 0): SPOTS}
        self.shots = 0
        self.fake_output = write_views
        patches = [mock.patch.object(tour, "OUT", root / "tour"),
                   mock.patch.object(tour.subprocess, "run", side_effect=self.fake_snapshot)]
        for patch in patches:
            patch.start()
            self.addCleanup(patch.stop)
        self.addCleanup(self.directory.cleanup)

    def fake_snapshot(self, args, **_):
        """dev-snapshot -Tour: writes the views into its -OutputDirectory."""
        self.shots += 1
        self.fake_output(Path(re.search(r"-OutputDirectory '([^']+)'", args[-1]).group(1)))
        return subprocess.CompletedProcess(args, 0, "", "")

    def folder(self, name="before"):
        return self.tour.OUT / name / "klamath-e0"

    def shoot(self, name="before", reuse=True, skip_failed=False):
        self.tour.shoot(name, str(self.cache), TURNS, -1, self.groups, reuse, skip_failed=skip_failed)

    def test_check_views_refuses_each_defect(self):
        for defect, (spoil, message) in DEFECTS.items():
            with self.subTest(defect):
                folder = Path(self.directory.name) / defect
                write_views(folder)
                self.tour.check_views(folder, "klamath", 0, SPOTS, TURNS)
                spoil(folder)
                with self.assertRaisesRegex(SystemExit, message):
                    self.tour.check_views(folder, "klamath", 0, SPOTS, TURNS)

    def test_a_valid_saved_tour_is_reused(self):
        self.shoot()
        self.shoot()
        self.assertEqual(self.shots, 1)

    def test_a_saved_tour_with_a_defect_is_shot_again(self):
        for defect, (spoil, _message) in DEFECTS.items():
            with self.subTest(defect):
                self.shoot()
                shots = self.shots
                spoil(self.folder())
                self.shoot()
                self.assertEqual(self.shots, shots + 1)
                self.tour.check_views(self.folder(), "klamath", 0, SPOTS, TURNS)

    def test_a_reshot_tour_that_is_still_bad_stops(self):
        self.shoot()
        spoil, message = DEFECTS["corrupt png"]
        spoil(self.folder())
        self.fake_output = lambda folder: (write_views(folder), spoil(folder))
        with self.assertRaisesRegex(SystemExit, message):
            self.shoot()
        self.assertFalse((self.folder() / "tour.json").exists())

    def test_sheets_skip_a_full_but_invalid_group(self):
        self.shoot("before")
        self.shoot("after")
        wrong_tile(self.folder("after"))
        out = Path(self.directory.name) / "sheets"
        with self.assertRaisesRegex(SystemExit, "shot at"):
            self.tour.sheets(TURNS, self.groups, out)
        self.tour.sheets(TURNS, self.groups, out, skip_failed=True)
        self.assertFalse((out / "klamath-e0.jpg").exists())

    def test_fresh_and_reused_tours_make_the_same_sheets(self):
        out = Path(self.directory.name) / "sheets"
        self.shoot("before", reuse=False)
        self.shoot("after", reuse=False)
        self.tour.sheets(TURNS, self.groups, out)
        fresh = {path.name: path.read_bytes() for path in out.glob("*.jpg")}
        self.shoot("before")
        self.shoot("after")
        self.assertEqual(self.shots, 2)
        self.tour.sheets(TURNS, self.groups, out)
        self.assertEqual({path.name: path.read_bytes() for path in out.glob("*.jpg")}, fresh)
        self.assertIn("klamath-e0.jpg", fresh)


@unittest.skipUnless(shutil.which("powershell"), "Windows PowerShell is required")
class PowerShellArgsTests(unittest.TestCase):
    def test_snapshot_hashing_works_when_module_autoloading_is_disabled(self):
        snapshot_source = (ROOT / "tools" / "dev-snapshot.ps1").read_text(encoding="utf-8-sig")
        self.assertNotIn("Get-FileHash", snapshot_source)
        helper = re.search(
            r"(?ms)^function Get-Sha256FileHash\b.*?^}\s*$", snapshot_source)
        self.assertIsNotNone(helper, "dev-snapshot SHA256 helper was not found")
        with tempfile.TemporaryDirectory() as directory:
            sample = Path(directory) / "sample.txt"
            sample.write_text("tour", encoding="utf-8")
            quote = lambda path: "'" + str(path).replace("'", "''") + "'"
            command = helper.group(0) + "; Get-Sha256FileHash " + quote(sample)
            disable_autoloading = "$PSModuleAutoLoadingPreference = 'None'; "

            baseline = subprocess.run(
                ["powershell", "-NoProfile", "-ExecutionPolicy", "Bypass", "-Command",
                 disable_autoloading + "Get-FileHash -LiteralPath " + quote(sample)],
                capture_output=True, text=True, errors="replace")
            fixed = subprocess.run(powershell_args(disable_autoloading + command),
                                   capture_output=True, text=True, errors="replace")

        self.assertNotEqual(baseline.returncode, 0, "baseline unexpectedly resolved Get-FileHash")
        self.assertIn("Get-FileHash", baseline.stderr)
        self.assertEqual(fixed.returncode, 0, fixed.stderr)
        self.assertEqual(fixed.stdout.strip(), hashlib.sha256(b"tour").hexdigest().upper())


if __name__ == "__main__":
    unittest.main()
