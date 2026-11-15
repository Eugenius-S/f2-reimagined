---
id: 135
title: The 3D view stays on in combat and whenever the map is on screen
milestone: M1
status: done
owner: glm
suggested: claude
priority: high
tier: 2
claude-only: no
user-check: yes
depends-on: []
reviewed: yes
---

## Goal

The owner, 2026-11-01: "it must always work". Today the 3D view drops to
the original 2D picture in combat, while the interface is disabled (the
enemies' turn, scripted moments), while any game window is open over the
map, and in dialogue. The owner had never seen the toxic caves in 3D,
because the geckos start a fight at once. Wherever the map is on screen,
the player sees it in 3D; a fight in Klamath is played start to finish in
3D.

## Context

- `ce/src/scene/preview_policy.cc` returns the block reason
  (game-state, game-mode, ui-disabled, iso-disabled, combat, dialog, movie)
  and `scene_view.cc` stops presenting on any of them. The policy dates from
  the first preview; nobody decided that combat must stay 2D.
- Remove the blocks that are only caution. Keep a block only where the
  original game does not show the map either (a full screen window, a
  movie), and say in the Log why each kept one stays.
- Combat brings what the 3D view has not drawn before: the hex cursor and
  its to-hit number, the red and green outlines round critters, the attack
  and the projectiles, explosions, floating damage text, the end turn and
  end combat buttons, critters walking during the enemies' turn. Each must
  work and land on the right tile in 3D. What already works in the 3D view
  (picking, the no-path cross, the see-through circle) must keep working.
- Dialogue: 066 hides the street behind black on the dialogue screen. Check
  what the 3D view should show there and match the original's layout.

## Acceptance

- A whole fight in the toxic caves (klatoxcv) and one in Klamath downtown,
  in 3D from the first shot to the end of combat: a short video or a strip
  of shots in `runs/135-3d-always-on/`, with the to-hit cursor, an attack,
  an enemy turn and the end of combat.
- Clicks in combat hit the tile and the critter they hit in the original
  view (a check by script, as the ground pick fixture does).
- The Log lists every block reason: removed, or kept with the reason.
- The original view regression stays pixel for pixel; ctest passes.

## Out of scope

New combat effects, the look of the outlines, the combat interface's layout.

## Log

### 2026-11-01 claude, step 1
Block reasons (`preview_policy.h`):
- game-state: kept. Outside normal play (loading, the main menu) the game
  shows no map.
- game-mode: narrowed to the world map, the only mode that shows no map.
  Combat, the player's turn, inventory, loot, barter, options, the automap
  and the rest keep the 3D view: a window covers the scene as it covers the
  2D map (the overlay mask from `windowShownMask`).
- ui-disabled: removed. The enemies' turn and scripted moments show the map.
- iso-disabled: kept. The game does not draw the map (loads, fades).
- combat: removed.
- dialog: see step 2.
- movie: kept. A movie covers the screen.

Check: ctest; the toxic caves entrance in 3D in the background run.
Not checked yet: a whole fight (no combat fixture). Next: the owner plays a
fight in Klamath and the toxic caves; a combat fixture for the background
run; the to-hit cursor, outlines and floating text in 3D; dialogue.
- 2026-11-02 The owner played a fight in Klamath on `a4d47b26`: it stays in 3D start to finish; the only flaw seen is the unscaled outlines (149).

### 2026-11-02 claude, step 2
Dialogue. The game itself switches the map off while a dialogue runs
(`isoDisable`, game mode 0x4), so the policy already stopped on
`iso-disabled`; the separate `dialog` reason never fired in the dialogue
fixture (`dialog=0` in the log line). It is removed from `preview_policy`
(reason, field, test). `iso-disabled` stays: the original draws no map in a
dialogue either, its 640x480 window covers the screen, and 066's black
backdrop gives the same on a wide screen. So what the player sees is
unchanged: black round the dialogue window, 0 non-black pixels outside it at
1920x1080, the street back after goodbye (066).
Block reasons now: game-state, game-mode (world map), iso-disabled (loads,
fades, dialogue), movie. Check: `dev-snapshot.ps1 -Map kladwtwn
-DialogFixture -CaptureConsole -OutputDirectory ...`; ctest 18/18;
`runs/135-dialogue-3d/dialogue-1080.jpg`.
Next: the combat checks wait for 146 (the combat fixture): the to-hit cursor,
an attack, an enemy turn, the end of combat in klatoxcv and kladwtwn, clicks
in combat by script.

### 2026-11-05 gpt, continuation
Claimed the remaining acceptance on fresh main after merged PRs #211 and #218. No complete combat run or combat-picking evidence is recorded in the existing Log. The new branch preserves both merged steps and does not touch their old branches.
Next: inspect the existing combat test hooks and build a background-only full-fight verification path; keep the owner game and cache untouched.
Check: fresh base `38df715d`; no open PR #135 existed at claim time.

### 2026-11-05 gpt, dependency blocker
The remaining acceptance is the four-phase combat capture and scripted combat picking in two maps. Task 146 supplies the capture fixture and is blocked after two consecutive incomplete runtime attempts; its PR #220 remains draft. No 135 combat code or runtime evidence can be completed until the fixture starts. Keep PR #296 draft and resume after #146 is unblocked. The already merged #211 and #218 steps remain intact.
Next: take the next independent board task.
Check: `runs/146-combat-fixture/klatoxcv-3d-fixed-startup/process.stdout.log` has no `COMBAT_FIXTURE START`; no 135 combat phase captures were generated.

- 2026-11-08 glm: notes merged as PR #296; combat acceptance waits on task 146's fixture.

### 2026-11-12 glm: the combat acceptance closed on the 146 fixture

Done: the last acceptance item (a whole fight in 3D on both maps, with
picking) is proven by task 146's combat fixture, merged in #408 on the
clean main 0aac216e:

- klatoxcv e2 and kladwtwn e0 each give COMBAT_FIXTURE START, four CAPTUREs
  (player-targeting during the player's turn, player-attack, enemy-turn,
  combat-ended) and COMBAT_FIXTURE PASS captures=4 in 3D; the same in
  classic for comparison. Strips: runs/135-3d-always-on/
  (klatoxcv-3d-four-phases.jpg, kladwtwn-3d-four-phases.jpg, copies of the
  fixture's strips; the classic strips live next to them in
  runs/146-combat-fixture/).
- Clicks in combat hit what they hit in the original view: the fixture's
  aim gate requires the engine's own pick at the crosshair to name exactly
  the target critter (gameMouseGetObjectUnderCursor: pointed==target) and
  the shot to read possible (_combat_to_hit accuracy > 0) before every
  phase; the click then fires the attack on that critter and the fight
  reaches the enemy's turn, where the enemy attacks back (the hero takes
  damage in the message log of the captures). The gate held on all four
  view/map combinations, so the pick agrees between 3D and classic.
- The block reasons were listed and resolved in the merged steps 1 and 2
  (game-state, game-mode world map, iso-disabled, movie kept with reasons;
  ui-disabled, combat, dialog removed).
- ctest 27/27 and the pixel regression (150 points, 0 changed pixels) ran
  on the fixture's merge PR; this PR adds evidence only, no code.

Not verified: a human playing the fight (the owner's fly-through, 175 on);
the unscaled outlines stay their own task (149).
