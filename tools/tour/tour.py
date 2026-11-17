"""The fly tour (task 078): fixed turned views of every Klamath map, before | after.

    py -3 tools/tour/tour.py --before SIDE [--after SIDE] [--maps kladwtwn,klamall] [--reuse] [--out DIR]
    py -3 tools/tour/tour.py --before CACHE --rp-game-root RP_GAME [--maps abbey,sfsheng]
    py -3 tools/tour/tour.py --coverage [--maps ...] [--expect kladwtwn:0,kladwtwn:1] [--rp-game-root RP_GAME]

--coverage shoots nothing: it prints which requested maps, map elevations,
named spots and turns the spot list covers and which it misses (also printed
before every tour). --expect adds map elevations the selection must have.

A side is either a checkout (its build/fallout2-ce.exe and the scenes its own
tools/dev-snapshot.ps1 reads) or a scene cache folder (<map>/<elevation>/
scene.json, from export.py --cache-root), shot with this checkout's build.
--after defaults to this checkout. So a scene change compares main's cache
(or a checkout of main) against the branch, and an engine change compares two
checkouts.

Every spot in the selected list is seen from the
fly camera at each of its turns (077: within 45 degrees), at the game
camera's pitch, in full daylight. One game run per map elevation and side: a
map tour (dev-snapshot -Tour), as the shapes eval shoots.

Shots go to build/tour/<before|after>/<map>-e<elevation>/, sheets to
build/tour/sheets/ (or --out): one JPEG per map elevation (pages of
ROWS_PER_PAGE spots), each row a spot and each turn a before | after pair,
and changed.jpg (then changed-2.jpg, ...) with only the spots whose views
differ, a red frame round each changed pair, or a note that none did.
--reuse keeps a side's shots when the same side, stops, exe and scene
provenance were shot before. A run whose sidecars show another map, tile or
no 3D scene, or a spot whose turns are one picture, stops the tour. Judging
the views stays with the eye.
"""
from __future__ import annotations

import argparse
import hashlib
import io
import json
import shutil
import subprocess
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[1]
OUT = ROOT / "build" / "tour"
ROWS_PER_PAGE = 8
# Each cell shows the middle of the shot: the spot sits at the screen centre.
CROP = (0.2, 0.15, 0.8, 0.85)
CELL_W, CELL_H, HEAD, PAIR_GAP, TURN_GAP = 330, 214, 20, 2, 12
# Runs in the test instance are deterministic, so a real change is many
# pixels (the shapes eval's tolerance).
DIFF_CHANNEL = 8
DIFF_PIXELS = 32
MAX_JPEG = 1_000_000  # CI refuses larger files in runs/
# A view this black in its middle shows nothing: the spot is off the map.
EMPTY_SHARE = 0.9


def powershell_args(command):
    """Run a dev-snapshot command in Windows PowerShell."""
    return ["powershell", "-NoProfile", "-ExecutionPolicy", "Bypass", "-Command", command]


def spot_groups(data, maps):
    """The selected spots by (map, elevation); every map when maps is empty."""
    groups = {}
    for spot in data["spots"]:
        if not maps or spot["map"] in maps:
            groups.setdefault((spot["map"], spot["elevation"]), []).append(spot)
    return groups


def load_spots(maps, path):
    data = json.loads(path.read_text(encoding="utf-8"))
    groups = spot_groups(data, maps)
    if not groups:
        raise SystemExit("no spots for those maps")
    return data["turns"], data.get("distance", -1), groups


def coverage(data, maps=None, expected=()):
    """The report lines: requested maps, each covered map elevation with its
    spots, then every requested map and expected map elevation with no spot."""
    turns = data["turns"]
    groups = spot_groups(data, maps)
    lines = ["turns: " + " ".join(f"{turn:+d}" for turn in turns),
             "requested maps: " + (", ".join(sorted(maps)) if maps else "every configured map")]
    for (map_name, elevation), spots in sorted(groups.items()):
        lines.append(f"covered {map_name} e{elevation}: {len(spots)} spots x {len(turns)} turns "
                     f"= {len(spots) * len(turns)} views")
        lines += [f"  tile {spot['tile']} {spot['name']}" for spot in spots]
    if not groups:
        lines.append("covered: none")
    missing = [f"missing {map_name}: no configured spots" for map_name in sorted(maps or ())
               if not any(key[0] == map_name for key in groups)]
    missing += [f"missing {map_name} e{elevation}: expected, no configured spots"
                for map_name, elevation in expected if (map_name, elevation) not in groups]
    lines += missing or ["missing: none"]
    if not expected:
        lines.append("elevations: not checked, no expected map elevations given (--expect)")
    lines.append("A named spot proves only its own view, not every object of its class.")
    return lines


def map_elevations(text):
    """'kladwtwn:0,kladwtwn:1' as [('kladwtwn', 0), ('kladwtwn', 1)]."""
    return [(map_name, int(elevation)) for map_name, _, elevation in (item.partition(":") for item in text.split(","))]


def side_kind(side):
    """('checkout', root) or ('cache', folder)."""
    path = Path(side).resolve()
    if (path / "build" / "fallout2-ce.exe").is_file() and (path / "tools" / "dev-snapshot.ps1").is_file():
        return "checkout", path
    if any(path.glob("*/*/scene.json")):
        return "cache", path
    raise SystemExit(f"{side}: neither a built checkout nor a scene cache")


def file_hash(path):
    return hashlib.sha256(path.read_bytes()).hexdigest() if path.is_file() else None


def scene_folder(kind, path, map_name, elevation):
    """Where the side's scenes of one map elevation come from: the cache
    side itself, else the checkout's build/scene-cache, else the game's
    mod-cache (dev-snapshot.ps1's order)."""
    if kind == "cache":
        return path / map_name / str(elevation)
    own = path / "build" / "scene-cache"
    if own.is_dir():
        return own / map_name / str(elevation)
    game = path / "game"
    if not game.is_dir():
        main = subprocess.run(["git", "-C", str(path), "worktree", "list", "--porcelain"],
                              capture_output=True, text=True).stdout.splitlines()[0]
        game = Path(main.removeprefix("worktree ")) / "game"
    return game / "mod-cache" / "scene" / map_name / str(elevation)


def check_views(folder, map_name, elevation, spots, turns):
    """Each view's sidecar shows the 3D scene of the asked map elevation at
    the spot, and a spot's turns differ (the fly camera took off): a scene
    that failed to start falls back to the game's own picture."""
    from PIL import Image, ImageChops
    sidecars = {}
    for path in folder.glob("*.json"):
        data = json.loads(path.read_text(encoding="utf-8"))
        sidecars[data.get("image")] = data
    shots = sorted(folder.glob("*.png"))[1:]
    problems = []
    for index, spot in enumerate(spots):
        part = shots[index * len(turns):(index + 1) * len(turns)]
        for turn, shot in zip(turns, part):
            data = sidecars.get(shot.name, {})
            where = f"{map_name} e{elevation} tile {spot['tile']} turn {turn:+d}"
            if (str(data.get("map", "")).lower().removesuffix(".map") != map_name
                    or data.get("elevation") != elevation or data.get("view_tile") != spot["tile"]):
                problems.append(f"{where}: shot at {data.get('map')} e{data.get('elevation')} "
                                f"tile {data.get('view_tile')}")
            elif not (data.get("scene_3d_enabled") and data.get("scene_view") == "full"
                      and data.get("renderer") == "scene-gl"):
                problems.append(f"{where}: no 3D scene ({data.get('renderer')}, {data.get('scene_view')})")
        images = [Image.open(shot).convert("RGB") for shot in part]
        if len(images) > 1 and all(ImageChops.difference(images[0], other).getbbox() is None
                                   for other in images[1:]):
            problems.append(f"{map_name} e{elevation} tile {spot['tile']}: every turn is the same picture")
    if problems:
        raise SystemExit("views not taken in the 3D scene:\n  " + "\n  ".join(problems))


def shoot(name, side, turns, distance, groups, reuse, rp_game_root=None, skip_failed=False):
    kind, path = side_kind(side)
    root = path if kind == "checkout" else ROOT
    if kind == "checkout" and not (path / "build" / "scene-cache").is_dir():
        print(f"{name}: {path} has no build/scene-cache, so its scenes come from the game's mod-cache", flush=True)
    for (map_name, elevation), spots in groups.items():
        try:
            shoot_group(name, side, kind, path, root, turns, distance, map_name, elevation,
                        spots, reuse, rp_game_root)
        except SystemExit as stop:
            if not skip_failed:
                raise
            # The group's folder keeps no stamp, so --reuse treats it as stale.
            print(f"{name} {map_name} e{elevation}: SKIPPED, {stop}", flush=True)
            with (OUT / "skipped.log").open("a", encoding="utf-8") as log:
                log.write(f"{name} {map_name} e{elevation}: {stop}\n")


def shoot_group(name, side, kind, path, root, turns, distance, map_name, elevation,
            spots, reuse, rp_game_root):
    stops = [f"{map_name}:{elevation}:{spot['tile']}:-1:{turn}:{spot.get('distance', distance)}"
             for spot in spots for turn in turns]
    folder = OUT / name / f"{map_name}-e{elevation}"
    # A rebuilt exe or a re-exported scene makes the old shots stale.
    stamp = {"side": str(path), "kind": kind, "stops": stops,
             "exe": file_hash(root / "build" / "fallout2-ce.exe"),
             "scene": file_hash(scene_folder(kind, path, map_name, elevation) / "provenance.json"),
             "rp_game_root": str(rp_game_root) if rp_game_root else None,
             "still_snapshot": rp_game_root is None,
             "startup_preview": rp_game_root is None,
             "skip_script_blocking": rp_game_root is not None,
             "rp_mod": file_hash(rp_game_root / "mods" / "rpu.dat") if rp_game_root else None}
    stamp_file = folder / "tour.json"
    if (reuse and stamp_file.is_file() and json.loads(stamp_file.read_text(encoding="utf-8")) == stamp
            and len(list(folder.glob("*.png"))) == len(stops) + 1):
        print(f"{name} {map_name} e{elevation}: reused", flush=True)
        return
    if folder.exists():
        shutil.rmtree(folder)
    folder.mkdir(parents=True)
    # -Command, quoted: through -File a list with commas becomes an array.
    command = (f"& '{root / 'tools' / 'dev-snapshot.ps1'}' -Map {map_name} -Elevation {elevation} "
               f"-Tour '{','.join(stops)}' -Ambient 65536 -OutputDirectory '{folder}' "
               f"-TimeoutSeconds {120 + 4 * len(stops)} -StepTimeoutSeconds 120")
    if rp_game_root is None:
        command += " -Still"
    else:
        # RP map-enter scripts may leave a dialogue request pending until
        # the game advances. Let the dev snapshot dismiss blocking dialog
        # or movie requests, and do not freeze startup in still mode.
        command += " -NoPreview -SkipScriptBlocking"
    if kind == "cache":
        command += f" -SceneCache '{path}'"
    if rp_game_root:
        command += f" -GameRoot '{rp_game_root}'"
    run = subprocess.run(powershell_args(command), capture_output=True, text=True, errors="replace")
    if run.returncode != 0:
        raise SystemExit(f"{name} {map_name} e{elevation}: dev-snapshot failed: {run.stderr[-600:]}")
    # The first snapshot is the start, before the tour.
    shots = sorted(folder.glob("*.png"))
    if len(shots) != len(stops) + 1:
        raise SystemExit(f"{name} {map_name} e{elevation}: {len(shots) - 1} views for {len(stops)} stops")
    check_views(folder, map_name, elevation, spots, turns)
    stamp_file.write_text(json.dumps(stamp, indent=1) + "\n", encoding="utf-8")
    print(f"{name} {map_name} e{elevation}: {len(stops)} views", flush=True)


def views(name, map_name, elevation):
    return sorted((OUT / name / f"{map_name}-e{elevation}").glob("*.png"))[1:]


def changed_pixels(before, after):
    from PIL import ImageChops
    if before.size != after.size:
        return before.width * before.height
    red, green, blue = ImageChops.difference(before, after).split()
    strongest = ImageChops.lighter(ImageChops.lighter(red, green), blue)
    return sum(strongest.histogram()[DIFF_CHANNEL:])


def empty(image):
    middle = cell(image).convert("L")
    return sum(middle.histogram()[:8]) > EMPTY_SHARE * middle.width * middle.height


def font(size):
    from PIL import ImageFont
    try:
        return ImageFont.load_default(size=size)
    except TypeError:
        return ImageFont.load_default()


def cell(image):
    w, h = image.size
    image = image.crop((int(w * CROP[0]), int(h * CROP[1]), int(w * CROP[2]), int(h * CROP[3])))
    image.thumbnail((CELL_W, CELL_H))
    return image


def spot_row(spot, map_name, elevation, turns, pairs):
    """One spot: a header, then a before | after pair per turn. Returns the
    row image and whether any pair changed."""
    from PIL import Image, ImageDraw
    pair_w = 2 * CELL_W + PAIR_GAP
    row = Image.new("RGB", (len(turns) * pair_w + (len(turns) - 1) * TURN_GAP, HEAD + CELL_H), (24, 24, 24))
    draw = ImageDraw.Draw(row)
    any_changed = False
    notes = []
    for column, (turn, (before, after)) in enumerate(zip(turns, pairs)):
        x = column * (pair_w + TURN_GAP)
        before_image = Image.open(before).convert("RGB")
        after_image = Image.open(after).convert("RGB")
        count = changed_pixels(before_image, after_image)
        changed = count > DIFF_PIXELS
        any_changed |= changed
        if changed:
            notes.append(f"{turn:+d}: {count} px")
        if empty(after_image):
            print(f"empty view: {map_name} e{elevation} tile {spot['tile']} turn {turn:+d}")
        row.paste(cell(before_image), (x, HEAD))
        row.paste(cell(after_image), (x + CELL_W + PAIR_GAP, HEAD))
        label = font(12)
        draw.text((x + 4, HEAD + 3), f"before {turn:+d}", fill=(255, 255, 255), font=label)
        draw.text((x + CELL_W + PAIR_GAP + 4, HEAD + 3), f"after {turn:+d}", fill=(255, 255, 255), font=label)
        if changed:
            draw.rectangle((x, HEAD, x + pair_w - 1, HEAD + CELL_H - 1), outline=(230, 40, 40), width=3)
    status = f"changed ({', '.join(notes)})" if any_changed else "same"
    draw.text((4, 3), f"{map_name} e{elevation}  tile {spot['tile']}  {spot['name']}  {status}",
              fill=(255, 90, 90) if any_changed else (255, 230, 0), font=font(14))
    return row, any_changed


def save_page(rows, path):
    """Rows stacked into one JPEG under the size CI accepts."""
    from PIL import Image
    page = Image.new("RGB", (max(r.width for r in rows), sum(r.height for r in rows) + 4 * (len(rows) - 1)),
                     (60, 60, 60))
    y = 0
    for r in rows:
        page.paste(r, (0, y))
        y += r.height + 4
    for quality in (80, 72, 64, 56):
        buffer = io.BytesIO()
        page.save(buffer, "JPEG", quality=quality, optimize=True)
        if buffer.tell() <= MAX_JPEG:
            break
    path.write_bytes(buffer.getvalue())


def sheets(turns, groups, out, skip_failed=False):
    if out.exists():
        for old in out.glob("*.jpg"):
            old.unlink()
    out.mkdir(parents=True, exist_ok=True)
    changed_rows = []
    total = changed = skipped = 0
    for (map_name, elevation), spots in groups.items():
        before, after = views("before", map_name, elevation), views("after", map_name, elevation)
        expected = len(spots) * len(turns)
        if len(before) != expected or len(after) != expected:
            if not skip_failed:
                raise SystemExit(f"{map_name} e{elevation}: {len(before)} before and {len(after)} after views, "
                                 f"{expected} expected; shoot first")
            skipped += 1
            print(f"{map_name} e{elevation}: skipped in the sheets, "
                  f"{len(before)} before and {len(after)} after views, {expected} expected", flush=True)
            continue
        rows = []
        for index, spot in enumerate(spots):
            part = slice(index * len(turns), (index + 1) * len(turns))
            row, differs = spot_row(spot, map_name, elevation, turns, list(zip(before[part], after[part])))
            rows.append(row)
            total += 1
            if differs:
                changed += 1
                changed_rows.append(row)
                print(f"changed: {map_name} e{elevation} tile {spot['tile']} {spot['name']}")
        pages = [rows[start:start + ROWS_PER_PAGE] for start in range(0, len(rows), ROWS_PER_PAGE)]
        for number, page in enumerate(pages, 1):
            suffix = f"-{number}" if len(pages) > 1 else ""
            save_page(page, out / f"{map_name}-e{elevation}{suffix}.jpg")
    # changed.jpg always exists: the first page, or a note that nothing changed.
    for start in range(0, len(changed_rows), ROWS_PER_PAGE):
        suffix = f"-{start // ROWS_PER_PAGE + 1}" if start else ""
        save_page(changed_rows[start:start + ROWS_PER_PAGE], out / f"changed{suffix}.jpg")
    if not changed_rows:
        from PIL import Image, ImageDraw
        note = Image.new("RGB", (900, 40), (24, 24, 24))
        ImageDraw.Draw(note).text((8, 10), f"fly tour: no spot changed of {total}", fill=(255, 230, 0), font=font(16))
        note.save(out / "changed.jpg", quality=90)
    print(f"{changed} of {total} spots changed"
          + (f", {skipped} map elevations skipped" if skipped else "") + f"; sheets in {out}")


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--before", help="a built checkout or a scene cache folder (required unless --coverage)")
    parser.add_argument("--after", default=str(ROOT), help="the same (default: this checkout)")
    parser.add_argument("--maps", help="comma-separated map names (default: every map in spots.json)")
    parser.add_argument("--rp-game-root", type=Path,
                        help="explicit RP game folder; selects rp_spots.json and runs the tour against that install")
    parser.add_argument("--reuse", action="store_true", help="keep shots of the same side and stops")
    parser.add_argument("--skip-failed", action="store_true",
                        help="keep going when one map elevation fails to shoot; its folder keeps no stamp, "
                             "so a later --reuse run shoots it again")
    parser.add_argument("--out", type=Path, default=OUT / "sheets", help="where the sheets go")
    parser.add_argument("--coverage", action="store_true", help="print the coverage report and shoot nothing")
    parser.add_argument("--expect", type=map_elevations, default=[],
                        help="comma-separated map:elevation pairs the selection must cover")
    args = parser.parse_args()
    if not args.coverage and not args.before:
        parser.error("--before is required unless --coverage")
    rp_game_root = args.rp_game_root.resolve() if args.rp_game_root else None
    spots_path = HERE / ("rp_spots.json" if rp_game_root else "spots.json")
    maps = set(args.maps.split(",")) if args.maps else None
    print(f"tour coverage ({spots_path.name}):")
    print("\n".join(coverage(json.loads(spots_path.read_text(encoding="utf-8")), maps, args.expect)), flush=True)
    if args.coverage:
        return 0
    if rp_game_root:
        if not (rp_game_root / "master.dat").is_file() or not (rp_game_root / "critter.dat").is_file():
            raise SystemExit("--rp-game-root must contain master.dat and critter.dat")
        if not (rp_game_root / "mods" / "rpu.dat").is_file():
            raise SystemExit("--rp-game-root must contain mods/rpu.dat")
    turns, distance, groups = load_spots(maps, spots_path)
    shoot("before", args.before, turns, distance, groups, args.reuse, rp_game_root, args.skip_failed)
    shoot("after", args.after, turns, distance, groups, args.reuse, rp_game_root, args.skip_failed)
    sheets(turns, groups, args.out, args.skip_failed)


if __name__ == "__main__":
    sys.exit(main())
