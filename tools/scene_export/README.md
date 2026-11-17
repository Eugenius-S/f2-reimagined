# Scene exporter

`export.py` turns Fallout 2 maps into the 3D scenes the game draws. It builds one scene per map elevation from the game's own files, which it only reads. The launcher runs the same code packed as `scene-exporter.exe` (see the standalone build below).

## Commands

Run from the repository root with Python 3.12 or newer and the packages in `requirements.txt` (NumPy, Pillow). Check every switch with `python tools/scene_export/export.py --help`.

| Command | What it does |
|---|---|
| `export.py --map kladwtwn --elevation 0` | Builds one map elevation, always, current or not. `--map` defaults to `kladwtwn`, `--elevation` to `0`. |
| `export.py --maps klamath,den` | Builds every elevation of each listed map or group that is not current. The elevations come from each map's header. |
| `export.py --maps all` | Every map of every named group. `all` does not include `rp`. |
| `export.py --maps all,rp` | Also the Restoration Project's own maps. The launcher asks for this when RP is installed. |
| `export.py --maps all --check` | Builds nothing and writes nothing. Exit 0 when every requested elevation has a complete, current scene; exit 3 otherwise, with one `not current: <map> e<n>: <reason>` line per scene. |
| `export.py --maps all --force` | Rebuilds every requested scene, current or not. |

Groups: `klamath`, `arroyo`, `vault15`, `den`, `modoc`, `vaultcity`, `gecko`, `brokenhills`, `newreno`, `redding`, `ncr`, `vault13depot`, `sanfran`, `militarybase`, `encdesertcoast`, `enccitycave`, `encspecial`, plus `rp`. A name that is not a group is taken as a map name. Names may repeat; each map is built once.

Where the game is:

- `--project-root <worktree>`: the game is `<worktree>/game` (the default is this repository).
- `--game-root <game>`: the game folder itself, holding `fallout2.cfg` and the DAT files.

Where the scenes go:

- By default `<game>/mod-cache/scene/<map>/<elevation>/`.
- `--cache-root <folder>` puts them in `<folder>/<map>/<elevation>/` instead. A dev-tools build of the game reads such a cache through the `F2CE_SCENE_CACHE` environment variable, so a worktree can test its own scenes while a running game keeps its cache.

Output of a `--maps` run:

1. First line: `{"maps_total": ..., "scenes_total": ...}`.
2. Then one line per scene:
   - `{"output": ..., "current": true}` for a scene that is kept;
   - `rebuilding <map> e<n>: <reason>`, followed by the build's JSON summary;
   - `FAILED <map> e<n>: <error>` on stderr.

One failing map does not stop the others. The exit code is 0 when nothing failed and 1 otherwise.

Tests: `python -m unittest discover -s tools/scene_export -p "test*.py"`. They use small synthetic maps and archives, no game data.

## Game files it reads

The exporter follows the order in which CE opens its sources (`gameDbInit`), highest priority first:

1. `master_patches` and `critter_patches` from `[system]` in `fallout2.cfg`. The project points both at `data`, so loose files there win.
2. The mods in `mods/mods_order.txt`, the last listed first. A missing entry is skipped. If `mods/*.dat` exists without the order file, the exporter refuses rather than guess the order.
3. The `PatchFile` chain from `ddraw.ini` `[Misc]` (`patch000.dat` and up, while they exist).
4. `ce.dat`, then `f2_res_dat`.
5. `critter_dat`, then `master_dat` (both required).
6. The game folder itself, as CE's working-directory fallback.

Relative paths in `fallout2.cfg` count from the game folder. DAT files are opened read-only and never extracted. The exporter writes nothing in the game folder outside the scene cache.

## When a scene is current

Each scene folder holds `provenance.json`. It records:

- the map and elevation;
- the hash of every file the build wrote;
- every game resource the build read or looked for, with the source that won;
- hashes of the exporter code and of its shape rule files (`shapes/fixed_cards.json`, `shapes/overrides.json`);
- the archives, the source order and the config file hashes;
- an input fingerprint over the code and rule hashes and the resources read.

It holds no timestamp.

A scene is current when all of these hold:

- its provenance names this map and elevation;
- the folder holds exactly the files in its hash table, each with its recorded hash;
- every file an export always writes is there (`scene.json`, `tiles.png`, `light.png`, `floor.bin`, `walls.bin`, `baked.bin`, `projection.bin`, `shadows.bin`);
- every file the scene itself names is there: its atlas pages, the wall pages `walls.bin` draws from, and `roof_fringe.png` when `floor.bin` has roof runs;
- `scene.json` parses and names this scene, the wall and floor headers read, and each PNG is a valid PNG;
- replaying the recorded reads gives the same fingerprint.

A scene goes stale when a game file it read changes, or when the exporter code or a shape rule file changes. A config change or a mod that the scene never read leaves it current. `--check` and a `--maps` run use this same rule.

## How a scene is replaced

A build never writes into the scene the game reads:

1. The exporter takes `<elevation>.lock` beside the scene, so only one exporter works on a scene at a time. The operating system releases the lock when the process ends, however it ends. The lock file holds no data and stays.
2. It finishes or undoes whatever an earlier stopped run left (see Recovery).
3. It builds into `<elevation>.staging`. Before the build it writes a provenance stub there that marks the folder as its own.
4. It checks the staged scene with the same rule as a current scene, including the map, the elevation and the inputs.
5. It moves the scene to `<elevation>.old`, moves staging into its place, then deletes `.old`.

Windows cannot swap a non-empty folder in one step, so between the two moves of step 5 there is no scene. If the second move fails, `.old` goes back at once. A hard stop at that moment leaves `.old` for the next run to restore. `--check` stays read-only and looks only at the scene folder: during that gap it reports `no scene yet`, so the launcher runs the export, which restores or rebuilds the scene.

The cache path is checked before anything is written: it must resolve inside the cache root, with no symlink or junction in any folder on the way.

## Recovery

When the next run takes the lock, the exporter repairs what an earlier run left behind:

- **A `.staging` folder** is an unfinished build. It is deleted only if the exporter can prove it made the folder: the folder is empty, or its `provenance.json` names this scene.
- **An `.old` folder with no scene** means the run stopped between the two moves. `.old` becomes the scene again.
- **An `.old` folder with a complete scene** means the new scene is in place. `.old` is deleted.
- **An `.old` folder with a damaged scene:** if `.old` is complete, it replaces the damaged scene.
- **A damaged scene with a damaged or unreadable `.old`:** the exporter keeps both and stops with `<scene>: <problem>; <scene>.old: <problem>; both kept`.

Safe steps when it stops:

- **`another exporter is writing this scene`:** another export holds the lock. Let it finish, or close it, then run again. Leave the `.lock` file alone.
- **`both kept`:** often a file is held open by the game, an antivirus or a backup tool. Close the game, wait for the scan to finish, and run again. Once `.old` can be read, the exporter restores it by itself.
  - If it still stops, move `<elevation>.old` out of the cache to a folder of your own, as a copy you keep.
  - Then run the export, which rebuilds the scene from the game files.
  - Do not delete `.old`: it may be the last complete copy of that scene.
- **`was not made by this exporter; it stays`:** the folder holds something the exporter cannot prove it wrote. Look at what is inside, move it out of the cache if you want to keep it, then run again.
- **`refusing a link` or `escaped the cache root`:** a symlink or junction sits on the cache path. Point `--cache-root` at a real folder, or replace the link with a real folder.

The scenes are a cache: `--force` rebuilds any of them from the game files.

## Verified and not verified

The synthetic tests in `test_export.py` verify:

- the source priority;
- the map groups, and the elevations read from map headers;
- the current-scene rule and the `--check` exit codes;
- the staging publish under injected failures and hard stops at each step;
- the ownership checks and the link and junction refusals (the junction cases run on Windows only);
- the shape rules in the fingerprint.

Those tests do not cover:

- a real export or `--check` of `all,rp`, and how long it takes;
- the lock between two real processes;
- a move while the game or an antivirus holds a real cache file open;
- the launcher end to end.

A finished export confirms two things: the game data parsed into a scene, and the scene's files are consistent. It does not confirm that the game draws the scene correctly, nor the scene's collision, lighting or speed.

## Standalone exporter

`build_standalone.ps1 [-OutDir <folder>]` uses PyInstaller to pack this whole folder, `vendor` and `shapes` included, into `scene-exporter.exe`. The default output folder is `build/exporter-standalone`. `standalone.py` runs the packed copy, so the exe reads the same shape rule files as the source tree and takes the same switches.

## Scene files

| File | Contents | Defined in |
|---|---|---|
| `scene.json`, `tiles.png`, `light.png`, `rock.png`, `sprites<n>.png` | the donor scene: atlas pages, floor tiles, light, rock | `vendor/fp/scene.py` |
| `floor.bin` | `F2FL` v7: floor cells, roof layer, roof wall-top runs, cut roof squares, water | `floor_sidecar.py` |
| `roof_fringe.png` | strips of the broken wall tops under roof edges, written when `floor.bin` has runs | `roof_fringe.py` |
| `walls.bin` | `F2WL` v5: triangle batches per atlas page, including the fly-only back pages | `wall_sidecar.py` |
| `baked.bin` | `F2BK` v7: the map objects the game skips because `walls.bin` holds them, and the ones it lifts onto a solid | `baked_sidecar.py` |
| `projection.bin` | `F2PJ` v1: the scene projection CE uses to place the hero | `projection_sidecar.py` |
| `shadows.bin` | `F2BS` v1: shadow-free sprites of boards whose art carries a painted shadow | `shadow_sidecar.py` |
| `provenance.json` | see "When a scene is current" | `export.py` |

Each module in the last column holds the exact layout and version history of its file.

## Donor code and license

`vendor/f2lib` and `vendor/fp` are a modified subset of `fallout2-first-person` at revision `0da1cd449f1dc7f2f6986ff7e8728a4b6ae20584`. The project changed them for the 3D scenes, so they are not verbatim copies. The donor repository itself is untouched. `wall_sidecar.py` and other modules here also hold modified donor-derived routines. The donor and fallout2-ce are licensed under the Sustainable Use License 1.0, which is not an OSI-approved open source license. `NOTICE.md`, `VENDORED.md`, `LICENSE` and `LICENSE-fallout2-ce.md` carry the attribution, the modification notices and the license texts.

## History

The exporter began as a one-map adapter for `kladwtwn` e0 that refused active mods. Later work added:

- map groups, every elevation and `rp`;
- mod sources;
- the scene cache with incremental rebuilds;
- `--check` for the launcher (task 091);
- the standalone build.

The task cards under `tasks/` record the changes since then.
