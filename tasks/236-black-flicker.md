---
id: 236
title: The 3D view no longer flickers black; the fly camera moves in normal play
milestone: M1
status: doing
owner: claude
suggested: claude
priority: high
tier: 0
claude-only: no
user-check: yes
depends-on: []
reviewed: no
---

## Goal

Current main is playable again: the 3D world is on screen in every frame, the
fly camera (Ctrl+F11) moves and turns when the dev tools are off, and the
dialogue text stays sharp. Anything that cannot be fixed in one PR is
described here with its evidence.

## Context

The owner, 2026-11-08, build 0f0f1823 (main with the 233 fix), windowed, scale
2, sharp text on, camera rotation on: "unplayable". Three symptoms.

### 1. The screen flickers black

Evidence: his video (2700x1440, 5.5 s, 165 frames at 30 fps). The interface
bar and its sharp text are identical in every frame; the 3D map area (the
whole iso window) is black in 134 of 165 frames (81%) and shows the world in
31. Runs of black are 3 to 49 frames long (5, 13, 4, 21, 6, 49, 19, 3, 14), the
world shows for 1 to 8 frames at a time. The hex cursor is drawn in the black frames, so the overlay is
presented. `runs/236-black-flicker/owner-video-frames-4-5.jpg`: frame 4 black,
frame 5 world, nothing else differs.

His `logs/devperf.log` counts `3d fps` only on frames the 3D scene presents;
the windows around the video read 4.2 and 21.2 against 80+ elsewhere.

Cause: `drawSceneOverlayCached` (219, PR 336), the sharp-text cache hit,
redraws the game overlay texture with `drawTextureQuad` and no blending.
`drawSceneOverlay`, the cache-miss path, enables `GL_ONE, ONE_MINUS_SRC_ALPHA`
for the same quad. The overlay is transparent over the map (the mask rule of
`maskOverlay`), so without blending the quad writes its transparent black over
the scene already drawn: every cache-hit frame is black over the map, every
miss frame is fine. A still world is almost all hits, so black is most of the
time. The dev snapshots never showed it: `presentStaticPreview` draws the
overlay with blending on its own.

Proof, a live probe (a temporary `glReadPixels` of one map pixel after the
overlay of each presented frame, not kept) on the same build, 3440x1440, scale
2, sharp text, glow, shadows, save slot 1, the cache-hit blend switched by an
environment variable:

| overlay quad | map pixel black |
| --- | --- |
| as on main (no blend) | 51 of 60 frames, then 50 of 60 |
| blended | 0 of 60, then 0 of 60 |

`runs/236-black-flicker/live-frames-before-after.jpg`: four consecutive
presented frames from the live back buffer, before (black, world, black,
world) and after (world in all four).

### 2. The dialogue text sometimes looks bold

Not the cache bug (the dialogue is a 2D frame: iso disabled, no cache hit).
Measured on the dialogue fixture (`-DialogFixture -SharpText -Language
russian`, 3440x1440, scale 2), with a temporary counter in the present: of 600
presented frames the sharp pass was not ready in 487; every one of those was
`prepareSharpTextForPresent` finding no text records on the game screen
buffer (`textCaptureSharpRecords(buffer).empty()`), while the window buffers
of the dialogue held 7 records (`records total=7 screen=0`); in the other
frames the screen held 4. A frame with no screen records presents the game's
own pixel text, a frame with them the sharp text: the answer list alternates
between the thin and the heavy face. No "sharp text: ... using pixel text" line
is written, because this is not a failure of the pass, it is a frame without
input. This is the screen-buffer text capture losing the dialogue window's
records after a window refresh (the same state task 140 found: "the answer
window records existed separately and had no matching screen-buffer owner
records"). Not fixed here: it lives in `scene/text_capture.cc` (the copy and
the validation of records between window buffers and the screen), is shared
with 140 and 193, and a patch needs the owner's exact hover sequence to be
checked against. Next: log, per frame, which record is dropped and by which
`copy` or `overwrite` call, with the hovered reply, then fix the capture
rule.

### 3. In 3D camera mode the movement keys and the mouse do nothing

Cause: 215 (PR 330) added Ctrl+F11 in normal play: `game.cc` calls
`devtools::toggleFly()`, which takes off and captures the mouse
(`mouseDeviceSetCaptured(true)`, relative mode). The flight itself steps in
`fly()`, called from `devtools::update()`, which returns at the top when the
dev tools are off (`gEnabled` is set only by `--dev-tools`, as `dev-launch.ps1`
passes it). The owner's game runs without them: the camera froze where it took
off, WASD and the mouse did nothing, the mouse cursor stopped (relative mode)
and the keys still went to the game, since `handleKey` also returns first when
the tools are off. Found by reading the code; not run live (every
`dev-snapshot.ps1` run has the dev tools on, so a normal-play flight cannot be
scripted yet).

## Acceptance

- `drawSceneOverlayCached` blends like `drawSceneOverlay`: the live probe
  reads 0 black map pixels in 60 frames with the cache hit on (done).
- `fly()` runs every frame the game loop runs, and a flying camera owns the
  keyboard (Esc lands) with the dev tools off (code change; needs the owner's
  Ctrl+F11 check).
- Release build and ctest pass.

## Out of scope

Symptom 2 (see above).

## Log

### 2026-11-08 claude
Done: found the cause of symptom 1 (video frames, then a live read-back probe
on the same build, before 51 of 60 black, after 0 of 60) and fixed it in
`ce/src/scene_renderer/context.cc`; found symptom 3 in `devtools.cc` and fixed
it (`update()` calls `fly()` before the dev-tools gate; `handleKey` lets a
flying camera own the keyboard when the tools are off, Ctrl+F11 still lands
it). Symptom 2 measured and described above, not fixed.
Check: Release build, ctest 21/21, one off-screen launch check at 3440x1440
scale 2 with sharp text.
Next: the owner checks black flicker and Ctrl+F11 flight in the build from this
PR; then symptom 2 in `text_capture.cc` (joins 140 and 193).
