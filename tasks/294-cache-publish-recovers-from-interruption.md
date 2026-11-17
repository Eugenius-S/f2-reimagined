---
id: 294
title: An interrupted export preserves a recoverable complete scene cache
milestone: M1
status: doing
owner: claude
suggested: glm
priority: normal
tier: 2
claude-only: no
user-check: no
depends-on: [293]
reviewed: no
---

## Goal

A failed rebuild leaves the previous complete scene usable, or recoverable on the next export, instead of mixing old and new artifacts.

## Context

`tools/scene_export/export.py:552-632` builds directly into the live directory: sidecars and images change before `provenance.json` is written. The [player cache batch](220-launcher-builds-every-map.md) and [273](273-missing-cache-builds-on-entry.md) need a complete scene while an export fails or is retried. The observed write order is a risk, not proof of the white-floor cause in [270](270-white-ground-capture.md).

## Acceptance

- Build a map/elevation in a validated sibling staging folder; validate it with 293 before publishing. Keep the previous complete cache until publication succeeds. On Windows use a recoverable rename/restore sequence; do not assume an atomic replacement of a nonempty directory.
- A second exporter targeting the same scene waits or exits clearly without modifying the first one's files. Different cache roots remain independent.
- Fault-injection fixtures stop after an artifact write and during publication, then restart. The old cache hashes remain available, recovery restores or completes one valid scene, and staging/backup cleanup stays within that scene's cache root.
- Exporter tests pass, including a successful rebuild and recovery; no game files or unrelated cache directories change.

## Out of scope

Renderer changes, background engine export (273), general filesystem transaction libraries, or an atomic-directory guarantee that Windows cannot provide.

## Log

- 2026-11-15 gpt: cut from the tooling and verification follow-up to the existing batches; source observations above are scoped gaps, not unverified game bug claims.
- 2026-11-18 claude: Fix in PR #524, stacked on #523 (293, not accepted yet; its review changes will be carried over here). Waiting for an independent review. Not a fix or a cause for 270/306's white floor.
  - Shown first: the new `test_export.PublishTests` against the old code; a build that fails after one artifact write left the live scene with a changed `scene.json` (the old code built straight into the live folder).
  - Fix in `tools/scene_export/export.py`, used by both the one-map and the `--maps` path:
    - `publish_scene` builds into `<map>/<elevation>.staging`, checks it with 293's file rule (`scene_files_problem`, split out of `stale_reason`: exactly the hashed files, none changed), then publishes: scene -> `<elevation>.old`, staging -> scene, remove `.old`. Windows cannot replace a non-empty folder in one step, so between the two renames there is no scene; the next export restores it. A failed second rename puts `.old` back at once; a failed first rename changes nothing.
    - `recover_scene` runs first under the lock: it removes a leftover staging folder, renames `.old` back when the scene is missing, and drops `.old` when the scene is complete (if the scene is damaged and `.old` complete, `.old` comes back).
    - `scene_lock`: one exporter per scene through an OS lock on `<map>/<elevation>.lock` (msvcrt on Windows, flock elsewhere), released by the OS when the process dies, so no stale lock. A second exporter stops with "another exporter is writing this scene" before touching anything. The lock sits in the cache root, so different roots stay independent.
    - Cleanup only deletes the export's own file names in that scene's `.staging`/`.old`; a link, a subfolder or any other file stops the export and stays.
    - `--check` stays read-only and looks at the live scene only.
  - Fault injection (11 cases, synthetic maps and DAT2 root): successful rebuild; failure after an artifact write; an incomplete scene that returns normally; hard stop (a `BaseException` that skips cleanup) mid-write, then a failing and a working restart; hard stop between the renames (check reports `no scene yet`, restart restores the old hashes); hard stop before `.old` removal (restart keeps the new scene, drops `.old`); failing second rename; failing first rename (file in use); damaged scene with a complete `.old`; second exporter on the same scene, another cache root alongside; a foreign file in staging. Each one also checks that the other scenes' hashes are unchanged.
  - Checks, light only: `py -3 -m unittest discover -s tools/scene_export -p "test*.py"`: 171 OK on Windows (msvcrt lock); CI runs the flock path on Linux. Mutation run of `PublishTests` over a temporary copy, 7 of 7 caught: build straight into the scene, staging not checked, no restore on a failed swap, no recovery, backup not restored, lock conflict ignored, cleanup deletes anything. `git diff --check` and `tools/check_public_content.py origin/main` clean.
  - Not run (heavy slot held by GLM): a real export through staging and its extra cost (the scene is checked by hash once more before the swap), a rename while the game or an antivirus holds a cache file open (covered only by the injected PermissionError), and the launcher end to end.
- 2026-11-18 claude: Carried 293's review fix (`05939152`) over by a merge. `scene_files_problem` now holds the new rule, so a staging scene must also pass it before publish: the contract files (`REQUIRED_OUTPUT_FILES`) whatever the manifest says, every file the scene names (`scene_references`: atlas pages, wall batch pages, `roof_fringe.png`), a `scene.json` of this map and elevation, readable wall and floor headers, valid PNGs. Synthetic fixtures share 293's `write_scene_artifacts`. Checks: 176 OK; mutations 12 of 12 (293's set) and 7 of 7 (`PublishTests`) caught on this branch.
- 2026-11-18 claude: Review fix (rereview-4 of PR #524, which reviewed `aacb9d80`). All three reproduced findings still held at `a9e5d201`; each new case below failed there first.
  - P1, an unreadable backup was deleted while the scene was damaged: `recover_scene` now removes `.old` only when the scene is complete. If the scene is damaged and `.old` is damaged or cannot be read, both stay and the export stops with `<scene>: <problem>; <backup>: <problem>; both kept`. Case: `.old` with a `PermissionError` on `tiles.png`, scene with a broken `walls.bin`; both stay byte for byte, and the next run, readable again, restores `.old` and rebuilds.
  - P2, staging was checked by its files only: `publish_scene` now checks staging with `stale_reason` (293's whole current-scene rule: this map and elevation, every file, the input fingerprint replayed) before the first rename. Cases: a whole, self-consistent `upper e1` written for `two e0`, and a wrong `inputs_sha256`; both are refused and the old scene stays.
  - P2, a familiar name was taken as ownership: `remove_scene_dir` and the backup restore now need proof the folder is the exporter's (`made_here`). Proof is an empty folder or a `provenance.json` naming this map and elevation; `publish_scene` writes one into staging before the build starts. The exception (`ours=True`) is the scene folder itself and a folder this publish just made under its lock. `provenance.json` is deleted last, so a removal stopped half way stays provably ours. Cases: foreign bytes under `tiles.png` in a staging or a backup folder next to a complete scene, a foreign `.old` with the scene missing (it never becomes the scene), a cleanup stopped after two files, a scene with a broken `provenance.json`, with and without a good backup.
  - P1, junction escape: confirmed natively on Windows with a tiny fixture before the fix. With `<cache>/<map>` a junction to a folder outside the cache, `scene_directory` accepted the path; deleting outside the cache was not exercised. Fix: `scene_directory` now compares the resolved path with the resolved cache root plus `<map>/<elevation>`, refuses a symlink or junction on every folder on the way, and `scene_lock` refuses a linked lock file. Cases (Windows only, `_winapi.CreateJunction`): a junctioned map folder and a junctioned `mod-cache` in the default cache are both refused, and nothing is written outside.
  - Checks: 183 OK. Mutations: 17 of 17 caught over `PublishTests` and `TargetTests` (the 7 before, plus: unreadable backup removed, staging checked by files only, a familiar name is ownership, a foreign backup is trusted, no staging marker, provenance removed first, the scene folder is not ours, the fresh backup is not ours, no containment check, junctions on the way allowed). 293's 12 of 12 still caught.
  - Cost: before the swap, staging now replays its recorded reads (293's check), on top of the build. Not measured; real export pending.
  - Limits: if a scene has a broken `provenance.json` and the run stops hard between moving it to `.old` and removing `.old`, the next run cannot prove the `.old` is ours and stops with that folder named. A foreign folder that copied a real provenance of this scene would pass as ours. The lock is still not native-tested across processes.
