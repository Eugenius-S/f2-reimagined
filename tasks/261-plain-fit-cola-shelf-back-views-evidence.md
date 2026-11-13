---
id: 261
title: Verify plain-fit cola and shelf back textures
milestone: M1
status: blocked
owner: gpt
suggested: gpt
priority: normal
tier: 3
claude-only: no
user-check: yes
depends-on: [125]
reviewed: no
---

## Goal

Verify that the opposite art slots used as back textures on plain-fit cola
machines and shelves are actual rear drawings rather than unrelated variants.

## Context

- Task 125's acceptance names a cola machine and shelf, but its Log says both
  shots failed framing: the cola was behind a fence and the shelf was too
  small at fly distance.
- The 125 coverage gate checks opaque-pixel coverage only. It cannot determine
  whether an opposite slot is semantically a rear view or a variant.
- Task 259 checks whether scenery turns with the fly camera. This task checks
  only the opposite slots used on solid back faces; do not change or duplicate
  the sprite-turn rule.

## Acceptance

- Capture a plain-fit cola machine and a shelf solid from a front control view
  and a matched rear fly quarter, before and after selecting the far face's
  source art. Frame each object large enough to read the art, with the far face
  unobstructed.
- For each object, identify its art file, placement/tile, near rotation and
  opposite slot. Compare the opposite slot to the game's original art and state
  whether it depicts the object's rear or a different variant/damage state.
- Confirm in the matched before/after frames that the chosen source matches the
  object from behind and has no see-through texels. If a shelf slot is a
  variant, record it as such and do not describe it as a rear drawing.
- Store matched JPEG pairs and JSON sidecars under
  `runs/261-plain-fit-cola-shelf-back-views/`. Record map, elevation, object,
  art, tile, camera turns, build commit and setting state. Keep each image
  under 1 MB.
- If either object cannot be framed or its art cannot be identified, record
  the exact attempt and blocker; do not claim that object verified.

## Out of scope

Sprite rotation behavior, changes to the back-sprite coverage threshold,
vehicle models, and changing the art assets.

## Log

### 2026-11-12 gpt review
Done: split the unverified cola and shelf back-texture evidence from task 259,
which covers scenery rotation exceptions. ID 261 is free after checking task
files, remote task branches and PRs through ID 299; PR #262 is repository URL
maintenance, not task 262.
Next: capture readable matched views and determine whether each opposite slot
is truly the object's rear art.
Check: not run; evidence task.

- 2026-11-13 claude: same as 259: move the hero out of the frame with `-HeroTile`, and take the rear view with `-Fly -FlyTurn 180` from a spot clear of the wall.
### 2026-11-12 gpt
Done: identified the plain-fit shelf solid as `shelf002.frm` (`Boxes and
Sacks`), `broken1.map` elevation 0, tile 9678, record index 436, map object
ID 535, FID `0x02000940`, near rotation 2. Captured opposite fly quarters in
`runs/261-plain-fit-cola-shelf-back-views-evidence/`. Original DAT art confirms
slot 5 is `UNUSED ART`; slots 1-4 are partial shelf variants. The exported
solid has no separate back source, so this is not a verified rear drawing.
Next: blocked on readable cola-machine evidence. The first attempt at
`denbus2` elevation 0 tile 22887 (`FlyTurn=180`, distance 5) showed the target
behind a wall. The second attempt at the same target (`HeroTile=22887`, solo
radius 5, `FlyTurn=180`, distance 4) removed the wall but placed the test hero
in front of the machine. Per the two-attempt limit, no further cola capture
was attempted.
Check: `powershell -NoProfile -ExecutionPolicy Bypass -File
tools/configure-build.ps1 Release` completed (794/794); `tools/dev-snapshot.ps1`
captured the shelf at turns 0 and 180; `py -3 tools/scene_export/export.py`
exported the Den and `broken1`; `py -3 tools/shapes/inventory.py` identified
the shelf as `box+back`. No tests run; no code or game files changed. The
cola pair, clear far-face view, and a no-see-through confirmation are not
verified. Draft PR #460 remains open.
