"""The fixture export smoke run: the source exporter and the packaged
exporter both export the same minimal synthetic game tree (297), each into
an isolated copy outside the checkout, and every output that does not
depend on absolute paths or packaging provenance comes out identical.

The publish workflow runs this after packaging and before upload:

    python tools/scene_export/fixture_smoke.py --staging build/exporter-standalone --work build/fixture-smoke

--staging points at the packaged exporter's folder (scene-exporter.exe);
--work defaults to a fresh temp directory outside this repository. The two
fixture game trees are separate copies, so neither run can see the other's
files; the comparison covers scene.json, every sidecar and every image by
sha256, with provenance.json excluded (it records absolute paths). A PNG
must also decode on both sides.

--self-test is the local negative test: the same smoke against a fixture
missing art/tiles/fixture_tile.frm must fail and name that path. The full
self-test performs a real (failing-fast) export, so it belongs to CI or a
heavy slot, not to a casual run.
"""

import argparse
import hashlib
import json
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
sys.path.insert(0, str(HERE / "vendor"))

from fixture import BUNDLED_RESOURCES, build_fixture, check_fixture  # noqa: E402

# Everything export_scene writes; provenance.json records absolute paths and
# is deliberately excluded from the comparison. rock.png is conditional: a
# floor-only fixture has no rock, and the exporter writes it only when the
# builder has one.
COMPARED_OUTPUTS = ("scene.json", "floor.bin", "walls.bin", "projection.bin", "baked.bin",
                    "shadows.bin", "tiles.png", "light.png", "roof_fringe.png", "sprites0.png")
CONDITIONAL_OUTPUTS = ("rock.png",)


def sha256(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def run_export(exporter, game_root, cache_root):
    """One export through `exporter` ('source' runs this tree's export.py)."""
    game_root = Path(game_root)
    cache_root = Path(cache_root)
    if exporter == "source":
        command = [sys.executable, str(HERE / "export.py"), "--game-root", str(game_root),
                   "--map", "fixture", "--elevation", "0", "--cache-root", str(cache_root), "--force"]
    else:
        packaged = Path(exporter) / "scene-exporter.exe"
        if not packaged.is_file():
            raise SystemExit(f"no packaged exporter at {packaged}")
        # The packaged exe runs with the fixture as its working directory, so
        # no lookup can silently reach this checkout.
        command = [str(packaged), "--game-root", ".", "--map", "fixture",
                   "--elevation", "0", "--cache-root", str(cache_root), "--force"]
    result = subprocess.run(command, capture_output=True, text=True, timeout=600,
                            cwd=str(game_root))
    if result.returncode != 0:
        tail = (result.stdout + result.stderr).strip().splitlines()[-6:]
        raise SystemExit(f"the {exporter} exporter failed (exit {result.returncode}):\n" + "\n".join(tail))
    return result


def scene_dir(cache_root):
    # export.py's scene_directory: <cache-root>/<map>/<elevation>.
    return Path(cache_root) / "fixture" / "0"


def compare_outputs(left, right):
    """Path-free and provenance-free comparison; returns a list of problems."""
    problems = []
    left, right = Path(left), Path(right)
    required = [name for name in COMPARED_OUTPUTS]
    optional = [name for name in CONDITIONAL_OUTPUTS
                if (left / name).is_file() or (right / name).is_file()]
    for name in required + optional:
        lf, rf = left / name, right / name
        if not lf.is_file() or not rf.is_file():
            problems.append(f"missing output {name} (left: {lf.is_file()}, right: {rf.is_file()})")
            continue
        if sha256(lf) != sha256(rf):
            if name.endswith(".json"):
                problems.extend(json_diff(lf, rf, name))
            else:
                problems.append(f"{name}: sha256 differs ({sha256(lf)[:12]} vs {sha256(rf)[:12]})")
        if name.endswith(".png"):
            for side, path in (("source", lf), ("packaged", rf)):
                try:
                    from PIL import Image
                    with Image.open(path) as image:
                        image.verify()
                except Exception as error:  # noqa: BLE001 - a broken PNG is the failure
                    problems.append(f"{name} ({side}) does not decode: {error}")
    for extra in sorted(p.name for p in left.iterdir() if p.name != "provenance.json"):
        if not (right / extra).is_file():
            problems.append(f"the source run wrote {extra}, the packaged run did not")
    return problems


def json_diff(left, right, name):
    """Field-level difference of two JSON outputs, ignoring values that
    carry absolute paths (they differ by design between the two fixtures)."""
    lvalue = json.loads(Path(left).read_text(encoding="utf-8"))
    rvalue = json.loads(Path(right).read_text(encoding="utf-8"))
    problems = []
    for key in sorted(set(lvalue) | set(rvalue)):
        if key not in lvalue or key not in rvalue:
            problems.append(f"{name}: key '{key}' exists only on one side")
            continue
        if lvalue[key] != rvalue[key]:
            lv, rv = json.dumps(lvalue[key]), json.dumps(rvalue[key])
            if ":" in lv or ":" in rv:
                continue  # a value with a drive/URL separator: path-shaped, skipped
            problems.append(f"{name}: key '{key}' differs: {lv[:120]} vs {rv[:120]}")
    return problems


def run_smoke(staging, work, source_root=None, frozen_root=None, build=True):
    """The whole smoke: two isolated fixtures, two exports, one comparison.
    Fixture roots can be injected for the negative test (then they are not
    rebuilt: a broken fixture must stay broken)."""
    work = Path(work)
    work.mkdir(parents=True, exist_ok=True)
    source_root = Path(source_root or work / "fixture-source")
    frozen_root = Path(frozen_root or work / "fixture-packaged")
    if build:
        build_fixture(source_root)
        build_fixture(frozen_root)
    # The bundled-resource precondition: a broken fixture fails here, with
    # its path, before any export runs.
    for label, root in (("source", source_root), ("packaged", frozen_root)):
        failures = check_fixture(root)
        if failures:
            raise SystemExit(f"FIXTURE_SMOKE FAIL the {label} fixture is incomplete: " + "; ".join(failures))

    run_export("source", source_root, work / "cache-source")
    run_export(staging, frozen_root, work / "cache-packaged")

    problems = compare_outputs(scene_dir(work / "cache-source"), scene_dir(work / "cache-packaged"))
    if problems:
        for problem in problems:
            print(f"FIXTURE_SMOKE {problem}")
        raise SystemExit(f"FIXTURE_SMOKE FAIL {len(problems)} problems")
    print("FIXTURE_SMOKE PASS the source and packaged exports agree on every compared output")


def run_negative(staging, work):
    """The deliberately-missing-resource negative: the smoke must fail and
    name the path."""
    work = Path(work)
    work.mkdir(parents=True, exist_ok=True)
    broken = work / "fixture-broken"
    build_fixture(broken)
    missing = "art/tiles/fixtile.frm"
    (broken / missing).unlink()
    # The bundled-resource precondition names it before any export starts.
    failures = check_fixture(broken)
    if not any(missing in failure for failure in failures):
        raise SystemExit(f"the broken fixture was not caught by its own precondition: {failures}")
    print(f"FIXTURE_SMOKE negative precondition named the missing resource: {failures[0]}")
    # And the full smoke on this broken fixture fails too (a fast failure at
    # the first art load, before any scene is written).
    try:
        run_smoke(staging, work / "negative", frozen_root=broken, build=False)
    except SystemExit as error:
        print(f"FIXTURE_SMOKE negative run failed as required: {str(error).strip().splitlines()[-1]}")
        return
    raise SystemExit("the negative smoke unexpectedly passed on a broken fixture")


def self_test():
    staging = None
    work = Path(tempfile.mkdtemp(prefix="f2r-fixture-smoke-"))
    try:
        # The light half: the generator and its bundled-resource precondition.
        complete = work / "fixture-complete"
        build_fixture(complete)
        problems = check_fixture(complete)
        if problems:
            raise SystemExit(f"the complete fixture failed its own precondition: {problems}")
        # Round-trips through the readers the exporter itself uses.
        from f2lib import map as map_lib
        from f2lib.frm import Frm
        game_map = map_lib.MapFile.load("fixture", str(complete))
        if 0 not in game_map.tiles or len(game_map.tiles[0]) != 10000:
            raise SystemExit("the generated map does not round-trip its squares")
        art = Frm.from_bytes((complete / "art/tiles/fixture_tile.frm").read_bytes())
        if not art.stored or art.stored[0][0].width != 80:
            raise SystemExit("the generated tile FRM does not round-trip")
        # A missing bundled resource fails with its path.
        broken = work / "fixture-broken"
        build_fixture(broken)
        missing = "scripts/scripts.lst"
        (broken / missing).unlink()
        failures = check_fixture(broken)
        if len(failures) != 1 or missing not in failures[0]:
            raise SystemExit(f"the missing-resource precondition named the wrong thing: {failures}")
        print("FIXTURE_SMOKE self-test (generator, precondition, round-trips): ok")
        # The heavy half: real exports, source and packaged, plus the negative.
        if staging:
            run_negative(staging, work)
            run_smoke(staging, work)
        else:
            print("FIXTURE_SMOKE --staging was not given: the export halves run in CI "
                  "(the publish job) or in a heavy slot; nothing was exported here.")
    finally:
        shutil.rmtree(work, ignore_errors=True)
    return 0


def main(argv=None):
    parser = argparse.ArgumentParser(description="The 297 fixture export smoke run.")
    parser.add_argument("--staging", help="the packaged exporter folder (scene-exporter.exe inside)")
    parser.add_argument("--work", default=None, help="the work directory (default: a fresh temp dir outside the checkout)")
    parser.add_argument("--self-test", action="store_true",
                        help="run the generator tests and, with --staging, the real and negative export halves")
    args = parser.parse_args(argv)
    if args.self_test:
        return self_test()
    if not args.staging:
        parser.error("--staging is required for the smoke run (or use --self-test)")
    work = args.work or Path(tempfile.mkdtemp(prefix="f2r-fixture-smoke-"))
    run_smoke(args.staging, work)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
