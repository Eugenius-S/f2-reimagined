---
id: 193
title: The UI batch, text flush left with room, full-screen screens on a fading backdrop, wide art
milestone: M1
status: doing
owner: any
suggested: any
priority: high
tier: 3
claude-only: no
user-check: yes
depends-on: []
reviewed: no
---

## Goal

The UI batch (the owner, 2026-11-04: take the UI together). One agent takes
it in one PR, in three parts: A, the text below; B, the full-screen
screens' backdrop and corners; C, the wide art of 128 (`tasks/128-*.md`,
its rights first), which closes with this batch.

Part A, the text. Every text block drawn by the sharp interface text pass starts each line at
the same left edge, with no indent on the first line, and its lines stand
at about 130% of the font size apart (at least 120%).

## Context

- Folded in (the owner, 2026-11-04,
  `runs/inbox/2026-11-04-message-log-left-edge.png`): the same goes for the
  message log window on the interface bar. Each line's bullet and text start
  further right line by line, a slanted left edge against the owner's red
  guide, and the lines are packed tight. Align the left edge and give the
  lines a little more room, the same as the dialogue. 151 (done) fixed the
  log's left edges once; find what brought the slant back.
- Folded in (2026-11-04 inbox sweep), the inbox line of 2026-10-25: the
  dialogue answer options keep a hanging indent. The bullet marker stands
  left of the text column and every wrapped line aligns with the first
  letter after the bullet, so the text has a straight left edge. The replies
  have no indent at all (flush left, as above).
- Batch: the UI text. 140 (letters cut in the dialogue answers, gpt's draft
  #206) is the same text layout code; whoever takes 193 starts from 140's
  merged state.
- The owner, 2026-11-04 (a screenshot in chat): a dialogue reply, green mono
  text on the dark grid panel, "The spirits with me, fren! They be all
  around. Sometimes talk." The first line starts indented (a leading space),
  the wrapped second line flush left; the lines sit too tight.
- The renderer is `renderSharpText` in `ce/src/scene_renderer/text_draw.cc`
  (task 151 fixed the message log's leading bullet there). Find where the
  first line's indent comes from (a leading space in the string, or the pen
  start) and drop it by a rule for every run, not for this one string.
- The line height applies to every block of this renderer: dialogue replies,
  the message log, floating text over critters. Keep each block inside its
  panel: a block that no longer fits scrolls or wraps the way it did before.
- Part of the UI batch (tier 3).

## Part B, the full-screen screens

- The owner, 2026-11-04, with a frame from the showcase video of the
  dialogue screen centred on black
  (`runs/inbox/2026-11-04-fullscreen-black-backdrop.webp`): everywhere a
  screen was full-screen and the world now shows round it, the same black
  backdrop, darkening smoothly with a fade; the character screen, dialogue,
  the world map, the Pip-Boy in general; and round the panel's corners a
  little, by 4 to 6 pixels.
- Linked to 173: the world map, dialogs and menus stay 2D,
  and this backdrop is what sits round them on a local map.
- Screens: the character screen, dialogue, the world map, the Pip-Boy, and
  every other screen that filled 640x480 in the original (inventory, barter,
  loot, options, save and load, the skilldex if it is full-screen).
- The fade: about 150 to 250 ms, eased, in on open and out on close, one
  shared value for every screen.
- The corners: the panel art cut round by 4 to 6 window pixels to the
  backdrop, at any scale.

## Acceptance

- The message log: every line's bullet starts at the same x, wrapped lines
  line up with the text after the bullet, and the line spacing matches the
  dialogue's; before | after of the log at x1 and x2, at 16:9 and 21:9.
- A wrapped dialogue answer option: the bullet left of the text, every line
  of the text on one left edge.

- Before | after devshots of the same dialogue reply panel, the message log
  and a floating text in `runs/193-text-indent-and-line-height/`; the first
  and the wrapped lines start at the same x, measured in the Log.
- With sharp text off the game's own view stays pixel for pixel; ctest
  passes.

Part B:

- Before | after of each listed screen at 16:9 and 21:9 in
  `runs/193-text-indent-and-line-height/`.
- A frame-step capture of the fade in and the fade out.
- The corners checked at x1 and x2.
- The original view regression stays pixel for pixel; ctest passes.

Part C: the Acceptance of 128.

## Out of scope

The font itself, colours.

## Log

### 2026-11-06 gpt

Part A code is on draft PR #310 at `a9f75174`; CI run `42097178128` passed.
The HUD log and dialogue fixtures confirm the changed text placement and
line pitch. The dialogue fixture currently has no wrapped answer, and the
required floating-text, sharp-text-off pixel comparison, and full x1/x2
before/after set are still missing.

Part B remains unverified. A Pip-Boy fixture capture at 1920x1080 produced
the same framebuffer as the prior fixture capture, so it does not prove that
the backdrop, fade, corners, or the 21:9 behavior changed. A generic window
hook prototype was removed because that capture did not validate it. Further
captures must read scene data from the task worktree's `build/scene-cache`.
The 03:05 fixture log used the main checkout's existing cache, whose files
were last modified on 2026-11-03; later captures used a copy in the task
worktree's `build/scene-cache`. No files in the main checkout cache were
changed.

The 03:14 inventory fixture still shows the 3D scene around the inventory
panel at 1920x1080. The test executable contained the generic backdrop
prototype (`2ee641f2...`), so that prototype did not establish the requested
backdrop. It was removed; the screenshot is kept only in ignored build output.

Part C remains blocked on art rights: Task 128's variant-6 assets cannot be
used, and no replacement art has been authored. Keep PR #310 draft until
the missing acceptance is implemented and evidenced.

### 2026-11-06 gpt
Done: rebased the existing task branch onto current `origin/main` and rebuilt
Release; CTest passes 20/20. Fresh sharp-text captures confirm the log fixture
at 16:9 and 21:9, the real dialogue fixture, and the floating-text lifecycle.
The dialogue fixture has no wrapped answer, so that line of acceptance is
still unproven. Added `runs/193-text-indent-and-line-height/diagnostics/current-ui-audit.jpg`.
The audit exposed a capture bug: the old GL snapshot path redrew only the SDL
surface after swap, erasing the scene and sometimes saving an all-black frame
while the UI fixture still reported PASS. It now reads the presented front
buffer. The corrected inventory capture is valid and visibly shows the world
around the panel, confirming that Part B's backdrop/corners are still absent.
No fade, full screen inventory backdrop or rounded corners were implemented.
Part C is still blocked on the rights resolution for task 128 art. PR #310
stays draft despite green CI until these acceptance gaps are resolved.
Next: implement and validate the shared backdrop/corner treatment, obtain a
wrapped dialogue answer and rights-cleared art, then capture the full before
and after set at x1/x2 and 16:9/21:9.
Check: `powershell -NoProfile -ExecutionPolicy Bypass -File tools/configure-build.ps1 Release`,
`ctest --test-dir build --output-on-failure`, and the fresh runs under
`runs/193-text-indent-and-line-height/fresh/`.

### 2026-11-06 gpt
Done: merged latest `origin/main` (`ad243e1c`) into the existing PR branch; the
overlap in `ce/src/devtools/devtools.cc` auto-merged. Reconfigured and rebuilt
Release, then reran CTest 20/20 and scene-export tests 135/135. No new visual
acceptance was added: the inventory backdrop/fade/corners, wrapped dialogue
reply, and rights-cleared task 128 art remain open. PR #310 remains draft.
Next: keep the task open until those acceptance items are implemented and
captured; this merge only refreshes the base and verifies the existing code.
Check: `tools/configure-build.ps1 Release`, CTest and scene-export tests on
the merged branch.
- 2026-11-08 claude: owner find (open): in the message window the left edge is ragged, as in the original. Wanted: bullet, a gap, then the text; a wrapped line starts level with the text, not with the bullet; the bullet does not count toward the text's left edge, so every text line of a block shares one straight edge. Crop: `runs/193-text-indent-and-line-height/owner-bullets-ragged-edge.jpg` (his shot `20261108-153908-kladwtwn-e0`).
- 2026-11-08 claude: owner find 2 (open): after part A the alignment went the wrong way. The text lines now start left of the message window edge and the first letters are clipped. Shift the whole block right so the bullet sits inside the window, then the gap, then the text; wrapped lines level with the text. Crop: `runs/193-text-indent-and-line-height/owner-status-text-clipped.jpg` (his shot `20261108-181342-kladwtwn-e0`).
- 2026-11-08 claude: priority high - the owner sees the clipped message log text on every screen.
- 2026-11-10 claude: back to todo: no open PR and no work since the last log line; the card leaves «В работе».

- 2026-11-11 glm (part A, stopped at the night boundary)
  Checked: the owner's two crops (owner-status-text-clipped.jpg,
  owner-bullets-ragged-edge.jpg) and the sharp text pass. The clipped block
  and its fix live in the sharp text renderer itself:
  ce/src/scene_renderer/text_draw.cc holds the message log's bullet lead
  (lines 414-447), the per-row hudLogXCorrection (352-364) and the glyph
  placement from run.x/characterX. The runs are only captured on the scene
  side (ce/src/scene/text_capture.cc:682 builds characterX). Shifting the
  block right and re-leveling the wrapped lines has to happen where the
  glyphs are placed, in the renderer file; a capture-side shift would
  misalign the erased legacy ink from the sharp glyphs.
  Per the night queue's boundary (a needed file in ce/src/scene_renderer/
  means stop), I stop here: the renderer is 157's territory this night.
  Next: shift the block right and level the wrapped lines at the text edge
  in renderSharpText (a renderer change), then the EN and RU message window
  snapshots at x1/x2.

### 2026-11-12 glm: the message window's left edge is straight; part A continues

Done: the message log's sharp rows share one left edge and no letter is
clipped, in English and in Russian. Two halves:
- ce/src/display_monitor.cc: the monitor's per-row one-pixel stair (the
  vanilla depth illusion, buf++) now applies only with the sharp text pass
  off. With the sharp pass on the monitor draws every row at the same x, so
  the captured ink sits where the sharp glyphs land. The pixel view keeps
  the vanilla stair (the acceptance's pixel-for-pixel rule).
- ce/src/scene_renderer/text_draw.cc: the old per-row -row correction (it
  assumed the stair in the captured x) is gone; the captured x is already
  the straight edge. A wrapped continuation row (no bullet) now starts at
  the block's text column, one bullet advance right of the bullet column,
  so every text line of a block shares one edge (the owner's 2026-11-08
  find). The bullet keeps its ink on the pen (151).
- ce/tests/embedded_fonts/embedded_fonts_test.cc: the HUD log case now pins
  the flush contract (both rows at the same x from flush captures) and adds
  a wrapped-continuation case (the ink at the text column).

Measured (the run's own screenshots, the log's leftmost green ink, physical
px at 1920x1080): before 206/208/210/219 (the stair; the fourth row's
bullet clipped off the window edge), after 206/206/206/206 in English and
206/206/206/206 in Russian. Evidence: the before/after sheets in
runs/193-text-indent-and-line-height/ (message-window-en-before-after.jpg,
message-window-ru-before-after.jpg). ctest 27/27 with the updated
embedded_fonts case. The pixel regression runs with the sharp pass off, so
its points show the legacy view and must stay 0 changed pixels (run logged
below). Not done yet in part A: the dialogue answer options' hanging indent
(the same renderer) and the line-pitch check against the dialogue.

Note: the log fixture's date row shows two bullets because the fixture
feeds the captured date line (which already carries the bullet) back as a
new message; the real log never does that. Fixture artifact, recorded here.
- 2026-11-12 glm (bookkeeping): status doing: the log edge half is merged
  (#410); part A's dialogue answers' hanging indent and parts B and C
  remain.
