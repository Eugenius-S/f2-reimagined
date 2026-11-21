"""The map and elevation list map-health.ps1 measures (271).

The map list comes from the exporter's map groups (tools/scene_export/export.py
MAP_GROUPS; "all" is every group, and --maps rp adds the Restoration Project's
own maps when the game ships them). Each map's elevations come from the map
file itself, exactly like export.py --maps: an elevation without squares is
not playable and is left out. A map file the game data does not carry is
reported under "missing", not silently dropped.

Prints one JSON object (the rows are objects, not pairs: PowerShell 5.1
flattens nested arrays when it reads JSON):
    {"maps": [{"map": "kladwtwn", "elevation": 0}, ...],
     "missing": ["someday"], "groups": [...]}
"""

import argparse
import json
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent / "scene_export"))

import export  # noqa: E402  (local module, after the sys.path change)


def map_targets(game_root: Path, groups: list[str]) -> tuple[list, list]:
    """The [map, elevation] pairs and the names that are missing.

    Mirrors export.py main --maps: each item is a map name or a group, and
    every elevation with squares counts. A map file that cannot be read at
    all (the game data does not carry it) lands in the missing list.
    """
    names: list[str] = []
    seen = set()
    for item in groups:
        if item == "all":
            group = [name for members in export.MAP_GROUPS.values() for name in members]
        elif item == "rp":
            group = list(export.RP_MAPS)
        else:
            group = export.MAP_GROUPS.get(item, (item,))
        for name in group:
            if name not in seen:
                seen.add(name)
                names.append(name)

    targets = []
    missing = []
    gf = export.ReadOnlyGameFiles(game_root)
    try:
        for name in names:
            try:
                elevations = sorted(export.MapFile.load(name, gf).tiles)
            except (OSError, ValueError, SystemExit):
                missing.append(name)
                continue
            targets.extend({"map": name, "elevation": elevation} for elevation in elevations)
    finally:
        gf.close()
    return targets, missing


def main(argv=None):
    parser = argparse.ArgumentParser(description="The map/elevation list for map-health.ps1 (271).")
    parser.add_argument("--game-root", required=True, help="the game folder the test game reads resources from")
    parser.add_argument("--groups", default="all",
                        help="comma-separated export groups or map names (default: all)")
    args = parser.parse_args(argv)

    groups = [item.strip() for item in args.groups.split(",") if item.strip()]
    targets, missing = map_targets(Path(args.game_root), groups)
    print(json.dumps({"maps": targets, "missing": sorted(missing), "groups": groups},
                     sort_keys=True, separators=(",", ":")))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
