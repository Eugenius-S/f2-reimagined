---
id: 254
title: The tools folder holds only tools in use, and the long-dash rule is checked
milestone: M1
status: done
owner: any
suggested: glm
priority: normal
tier: 3
claude-only: no
user-check: no
depends-on: []
reviewed: yes
---

## Goal

A new agent finds the working tools at a glance, and the rule against the
long dash stops depending on memory.

## Context

- Found by the 2026-11-11 audit (`runs/audit-2026-11-11.md`, problem 14).
  Stands alone: 198 cleans the owner's projects folder, not the repository.
- `tools/` holds about 40 one-off scripts of finished tasks (`shots119.ps1`,
  `grid166.ps1`, `probe167.ps1`, `pairs120c.py`, `classic169b.ps1`,
  `walk169.ps1`, `hours166.ps1`, `reg119.ps1`, `effect_white_review.py`,
  `glow_ladder_sheets.py` and their kin). The ten checked have no reference
  outside `runs/` and `tasks/`.
- The long dash (U+2014) is in 24 tracked text files, the title of task 246
  among them. `tools/check_public_content.py` does not look for it.
- Task 225's `suggested` field holds a phrase.

## Acceptance

- Every script in `tools/` (top level) either has a reference outside
  `runs/` and `tasks/` (AGENTS.md, docs, another script, the workflow) or is
  deleted. The Log lists the deleted names; git history keeps them.
- `tools/check_public_content.py` refuses an added line with U+2014, with a
  test case next to the existing ones.
- No tracked text file under `tasks/`, `docs/`, `tools/`, `AGENTS.md` and
  the READMEs holds U+2014 (a colon, a comma, a full stop or parentheses
  take its place; the meaning stays).
- Task 225's header is valid.
- Optional, only if it stays under 15 lines: the owner shot's two detached
  writer threads (`ce/src/devtools/owner_shot.cc:465`, `:810`) are joined
  at exit, so F12 followed by an immediate quit leaves a whole PNG.

## Out of scope

Worktrees, remote branches and the main checkout (198); `runs/` evidence.

## Log

- 2026-11-11 claude: cut from the audit.

- 2026-11-11 glm
  Done:
  - Deleted 35 one-shot tools with no references outside runs/ and tasks/
    (git history keeps them): shots119, shots120, shots120c, shots120d,
    shots122, shots122fix, shots124, shots125, pairs119, pairs120, pairs120c,
    pairs125, reg119, grid166, grid167, grid169, hours166, hours167, hours169,
    hours169b, probe166, probe167, probe169, walk166, walk167, walk169,
    classic169, classic169b (all .ps1), effect_final_sheets.py,
    effect_glow_levels.py, effect_white_review.py, glow_ladder_sheets.py,
    muzzle_sheets.py, game-input-real.ps1, dev-input-display-154.txt.
    Kept: game-input.ps1 (devtools.cc names it), shadow_seams.py
    (scene_shader.cc), generate_options_frame.py (options_frame_owner.h),
    check_png_pixel.py (dev-snapshot.ps1), the verify.yml-checked scripts,
    and the generic utilities (gallery_*, image_diff, patch_pixel, crop_box,
    shadow_stripes).
  - tools/check_public_content.py now bans the long dash (U+2014) in added
    lines (label long dash (U+2014)); a test in
    tools/test_check_public_content.py pins it. 13 files had it (12 task
    files and tools/scene_export/test_export.py); every U+2014 replaced
    with a plain hyphen. Note: the file-path form
    (py -3 -m unittest tools/test_check_public_content.py) fails on the
    import path in every state, main included; the working form is
    py -3 -m unittest discover -s tools -p test_check_public_content.py,
    which is also what CI runs since 252 (7 tests green).
  - tasks/225 header: suggested was the phrase codex (P1 on #320), now any.
  - owner_shot.cc: the PNG and sidecar encode threads are joined on the
    next shot and at exit, so quitting right after F12 cannot cut a file
    mid-write.
  Check: git grep for U+2014 in tasks, docs, tools, AGENTS.md and README:
  0 hits. unittest discover: tools 7 OK, tools/regression 12 OK,
  tools/scene_export 145 OK. powershell -File tools/tasks.ps1 prints the
  board without errors. ctest 27/27; one background devshot launch check
  green.
