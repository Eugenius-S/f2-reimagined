---
id: 317
title: Restore 3D exit cues and predictable edge scrolling
milestone: M1
status: doing
owner: gpt
priority: high
tier: 0
claude-only: no
user-check: no
depends-on: []
reviewed: no
---

## Goal

Keep local maps navigable and their exits visible without losing the corrected map edge.

## Context

The November 22 owner reports are grouped and ordered in the [inbox](inbox.md#november-22-night-queue). This first packet extends task 316, after the ac73dbb release. The second Arroyo area and a random encounter were affected. Screenshot timing and the edge fix are clues, not established causes. Old 3D policy intentionally omitted translucent exit sprites while keeping their pick proxies, so distinguish visibility from navigation before assigning blame.

## Acceptance

- Compare known-good and current builds on the same disposable saves: 3D exit marker, hover/picking and actual route; check Arroyo and a random encounter. Compare with a classic 2D control and check both before and after F12 and Ctrl+F12 where possible.
- Keep local 3D active and the world map in 2D. Check the Temple route with and without combat when a walking fixture exists.
- Pan in both directions along the edge of the map and diagonally. Preserve the wider camera travel and straight authored clipping; verify the round Temple door separately.
- Keep change-specific checks bounded. Send each tested packet to CI and obtain an independent read-only Terra High review before the release decision.

## Out of scope

Do not change the owner's installation or saves. Follow-on input, lighting and visual defects remain on the linked ordered queue rather than expanding this renderer packet.

## Log

### 2026-11-22 gpt

Claimed the first packet from the owner's overnight batch. Owner-visible symptoms are recorded separately from verified source behavior.

## 2026-11-23 Windows evidence

The published Windows candidate shows the orange random bridge and green Arroyo bridge exit cues after actual F12, with matching before and after scene-gl metadata. The sampled left edge is identical to the prior release. The Temple corner moves along the authored limit and away from it at 3440x1440 / 2x. The ordinary ARCAVES-to-ARTEMPLE click passes; additional reverse and world-map routes, full edge coverage, and the owner-specific door click remain open.

[Native acceptance checks](../runs/325-night-checks/README.md).

Follow-up: both normal-click Temple/foyer directions passed with real local 3D arrivals. The old failure was a probe-input timing issue, not a change to product walking or clip behavior. [Evidence](../runs/325-combat-and-exit/README.md).

Follow-up: ARBRIDGE tile 26734 also opened the world map by a normal click, after one legal movement step. The world map did not enable local 3D. The same normal-click check passed on the registered random encounter RNDWHALE, from 12715 to exit 12716 and then the 2D world map. The separate RNDBRIDG diagnostic had no reachable visible exit from the direct fixture spawn; do not close the RNDBRIDG or exact owner save acceptance on this evidence.

Temple sanctuary door: at ARCAVES e2 view 22307, 3440x1440 at 2x and zoom 1.28, a native game-handler click on the selected visible door (19928) started the hero walking. After arrival the door advanced from frame 0 to 5 and set its open flag. Both snapshots retained local 3D and the same camera. This was an offscreen test at the original screen coordinate, not physical focus or the owner save.

A separate exact release `65d374e8` check started the outer Temple
fixture next to the native exit; a normal click on ARTEMPLE 21917 moved
the hero from 21716 and reached ARVILLAG e0 at 11683. The watched first
local frame presented in 3D. A separate exact build ARTEMPLE combat
fixture captured targeting, attack, enemy turn and combat end in
scene-gl at tile 17288. It used a spawned fixture critter, not the story
challenge NPC. These independent checks do not prove the continuous
post-challenge route from ARCAVES e2 on the owner's save.

At exact Windows build cc7ea07d, four separate normal clicks on authored
ARCAVES connections reached e0 to e1, e1 to e0, e1 to e2, and e2 to e1.
The transition watcher saw full 3D at the first destination frame in each
case. These start beside the exits and do not prove the continuous story
challenge path. The snapshot wrapper also now requires the destination
floor and tile and copies the matching first-3D PNG and sidecar before
marking a watched exit run complete. Tested this export for ARCAVES e1 to
e0 and ARTEMPLE to ARCAVES e0 using the native Windows game.

### 2026-11-23 release reconciliation

Release `039d25da3` includes the exit-marker fix and the native destination checks above. Remaining: a continuous story-duel route, Arroyo and random-encounter transitions around screenshot capture, and matched diagonal scrolling and map clipping near the round Temple door.

### 2026-11-26 Windows controls

The new worktree is based on `f1f1d61e3`. The isolated test used the
published `a414e39d7` Windows Release executable (SHA256
`75caf366deea13d6a412abf1e89175cd9a1c77f2ba42b6568886f7ffe5fa05ac`).
The runtime and tools trees are identical across those commits. The test
installation contains disposable copies of stock data. No owner saves
were used.

- ARCAVES e2, hero 21125, camera 22307, zoom 1.28, an off-screen
  3440x1440 window at 2x. The script drove the game's hover path to
  the left edge, the upper-left corner and back. Camera positions were
  22307, 22108, 21708 and 21907, matching the saved fd16a60e7 control.
  All four frames remained 3D and the four clip planes matched the
  reference exactly. This was a floor-envelope clip fixture. The separate
  classic snapshot reported view off at the same starting camera.
  [Edge captures](../runs/317-native-acceptance/temple-edge.jpg).
- A separate injected press and release reached the input logger at the
  round-door starting view. The camera moved and the hero stayed on
  21125. The run did not establish a click on door 19928 or an open flag.
- Normal click probes started beside two exits, with separate processes.
  ARCAVES e2 exit 28511 reached e1 at 14722 after 20 frames; destination
  frame 346 was 3D. ARTEMPLE exit 21916 reached ARVILLAG e0 at 11683
  after 31 frames. The watcher recorded loading frames with iso disabled
  and the first available local 3D frame at 742.
  [First local 3D frames](../runs/317-native-acceptance/first-local-3d.jpg).

Still open: one continuous route after the real story duel, the round
door action combined with edge scroll against the authored clip, and the
independent Terra High review. The local catalog identifies Priority as
the optional Fast tier. Continue on the Standard route. Preserve the
isolated raw logs under `build/317-*`. This packet changes no application
code and has no release candidate.

### 2026-11-26 authored edge and story fixture follow-up

A temporary diagnostic build based on `ecb7e6138` used the normal game
mouse handler to select the scenery door, with stock data plus only
`maps/arcaves.edg` extracted unchanged from RPU `rpu.dat` (SHA256
`27db830e0fb67d02b60e166e2acb772a92292f5f4dd88f733b740b406a0864ee`).
No other RPU content was active. At ARCAVES e2, 3440x1440 / 2x, zoom
1.28, hero 21125 and starting view 22307, input moved diagonally to 21907
and back to 22307; the four authored clip planes stayed identical. The
normal door handler then picked tile 19928 at logical 27,240, walked the
hero to 20128 and changed the door open flag from 0 to 1 (frame 0 to 9).
All captured states were local 3D. The reverse order also succeeded in a
separate process. This is injected engine input, not physical Windows input,
and does not prove all corners or a complete RP installation. The diagnostic
instrumentation was removed. [Captures](../runs/317-native-acceptance/door-edg.jpg).
Raw scripts, snapshots, sidecars and console logs are in
`build/317-edg-scroll-then-door` and `build/317-edg-only-door-edge`.

Loading the full RPU DAT with its mods order produced a real MAP_EDGE, but
both the door-first and scroll-first probes stopped after their first input;
the watchdog could not capture the engine. The stock-plus-EDG-only run
above is a different fixture and does not clear the full-RP hang. A separate
classic 2D scripted-scroll attempt did not start its scene-input script and
is not a completed 2D control.

The story probe's normal scripted fight request is suppressed by
`skipScriptCombatRequest()` during background captures. A temporary, narrow
exception removed that suppression for the challenge, but the run
still did not reach observed combat: keys 2, 1, 1 were consumed and the
snapshot remained in dialogue. This is a fixture blocker, not proof of a game
regression. Next: trace the actual dialogue/script state and input lifecycle,
then finish the real challenge and walk the entire route in one process.
The earlier TransitionWatch numbers 346 and 742 count real presents, but
`first-local-3d.jpg` comes from `takeSnapshot` with `static-wall-plus-hero-preview`;
it is not necessarily a back-buffer readback. Capture actual present buffers
for the continuous route. Do not modify the shared watcher from PR #588.
Status remains doing, reviewed no; no game fix or release decision is claimed.
The clean Windows Release build after removing instrumentation passed, and
CTest passed 35/35 (`build/317-final-build.log`, `build/317-ctest.log`).

### 2026-11-26 story-duel diagnosis and narrow fix

The previous report that the fight never started is superseded. The direct
fixture called `actionTalk` just as the NPC also requested the real scripted
dialogue, producing a second dialogue. With the extra fixture dispatch removed,
the NPC naturally opened its dialogue: `Sure, let's party`, `I'm ready`,
`[Done]`; the script then requested combat. A temporary background-only
exception let that genuine combat request run. Native mouse-handler clicks
and turn input fought the real challenge NPC without spawning an opponent or
forcing combat completion. One run ended with the NPC's actual victory text,
`You have defeated me. You are worthy, Chosen One.`

Before the fix, `DialogFinished` persisted through the real fight: present
callbacks logged state 2, scene view full but `shouldPresent=0` from frame 90
at tick 14021 to at least frame 3510 at tick 75159, and after combat. The
post-duel capture was `vanilla-framebuffer`, availability `game-state`.
`runtimeState` rejected `DialogFinished`, so local 3D fell back to 2D even
though the scene remained enabled. The production change permits precisely
`DialogFinished`; game-mode, iso, movie and transition gates remain in place.
After the fix, combat callbacks at frames 90 through 540 and the post-combat
callback recorded state 2 and `shouldPresent=1`. A post-victory `takeSnapshot`
shows the 3D preview. [Before and after](../runs/317-native-acceptance/duel-before-after.jpg).
The QA binaries contained temporary observers and input drivers, removed from
production; raw output is under `build/317-story-present-before`,
`build/317-story-present-after`, and `build/317-story-route6`. The screenshot
on the right is the preview capture, not an asserted back-buffer readback.

For the continuous route, the victory line was floating text, with no response
prompt. Immediately after combat no path to e1 existed; after waiting 12 seconds
for the NPC's real scripted sequence, paths opened without forcing a door or
changing game state. Normal mouse-handler clicks moved the hero 13527 -> 15725
-> 17125 on ARCAVES e2, still in 3D. The automated driver then found no visible
waypoint because the next segment was outside its current view. It did not
reach e1 or capture any arrival; the full route and its present buffers remain
open. Next, extend the disposable driver's normal edge-scroll input to follow
the path, then capture each first actual present. This is a fixture limitation,
not a claim that the player cannot walk further. Full RP and 2D controls remain
open as stated above.
