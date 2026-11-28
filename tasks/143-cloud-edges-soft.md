---
id: 143
title: Cloud shadow edges stay soft on the ground
milestone: M1
status: blocked
owner: any
suggested: any
priority: normal
tier: 3
claude-only: no
user-check: yes
depends-on: []
reviewed: no
---

## Goal

A cloud shadow fades into the lit ground over a wide soft band. No hard
line or one-step contour at its edge, on dark ground as well as on light.

## Context

- The owner, 2026-11-01, after 138 (#199) went live: "the clouds give hard
  edges again" (`runs/inbox/2026-11-01-cloud-hard-edge.png`, the downtown
  street in kladwtwn, a curved hard line across the dark asphalt).
- 138 changed `ce/src/scene_renderer/scene_shader.cc`: the cover now runs
  through `smoothstep(kCloudCoverLow, kCloudCoverHigh, cover)`, and for the
  ground and walls (`uLightMode` 1 and 2) the cloud loss is taken in the
  5-bit colour steps (`cloudLoss5 = baseColor5 - cloudColor5`, then
  subtracted from `sunColor5`). On dark ground a 22% loss is one or two
  5-bit steps, so the whole soft band collapses into a single step: a hard
  line. That is the first suspect; confirm it before fixing.
- Also rule out a lamp's light edge: the shot has a lit patch top right.
  Take the same spot with cloud shadows off; if the line stays, it is not
  this task, write that in the Log.
- The fix is general: the cloud fade must not be quantised into a step
  where the 5-bit look is kept (dither the cloud loss, apply it before the
  step, or another general rule). No per-map or per-spot case.

## Acceptance

- At the owner's spot and on light sand, the cloud edge reads soft: no
  line, no single step, at 08:00, 12:00 and 17:00.
- Building shadows keep their contrast inside and outside a cloud (138).
- Evidence in `runs/143-cloud-edges-soft/`: before and after crops at the
  owner's spot and on light ground, and the same spot with clouds off.
- The scene tests pass.

## Log

- 2026-11-01 Cut from the owner's live find after #199.
- 2026-11-01 The owner asked gpt directly to make cloud shadows an option that is off by default for now; this task still makes the edges soft for when they are on.

### 2026-11-01 gpt
Blocked: The owner's PNG is 432x505 and has no camera or time sidecar. I used
the prior KLADWTWN/tile 24678 fixed camera as a candidate, but the best
half-scale template correlation is only 0.386. The reported hard edge did not
reproduce in that candidate, so the paired cloud-off capture cannot attribute
the edge to clouds or the lamp. No shader change was made.

Evidence: `runs/143-cloud-edges-soft/owner-vs-candidate-0825.jpg` and
`runs/143-cloud-edges-soft/diagnostic.json`. The matched 08:25 captures use
game time 303000, sun swing 44, cloud phase 505, 1920x1080, and the same
executable. Cloud-off keeps sun shadows enabled. In the best candidate crop,
on/off grayscale mean absolute difference is 1.57/255, with no pixels over
10; this is not an exact match to the owner input. A second candidate pair at
08:00 and cloud phase 0 had the same camera-mapping limitation.

Check: Release build completed (763/763); CTest passed 18/18. The exporter
read 1467 resources into `build/scene-cache`. Pixel regression passed 16/16
with zero changed pixels (`runs/regression/20261101-231614-494-54244`).
`game/mod-cache` was not used.

Next: Obtain a capture sidecar or exact camera, zoom, and time for the owner
frame, then repeat the matched cloud-on/cloud-off comparison before changing
the shader.

### 2026-11-02 gpt
Blocked: The owner image still has no sidecar; the candidate camera match remains
0.386. No shader change was made.

Failed attempts: At KLATRAP tile 13913, 08:00, the supposed clouds-off command
included `--dev-cloud-phase`; that option itself enables cloud shadows, so the
zero-pixel delta was not an off comparison. A 12:00 cloud-only capture at the
same tile was mostly black and had no matched sun-only baseline, so it cannot
measure cloud-edge softness or sun-shadow contrast.

Evidence: The valid 08:00 light-ground pair is
`runs/143-cloud-edges-soft/sand-08-cloud-vs-sun.jpg` with details in
`runs/143-cloud-edges-soft/diagnostic-followup-2026-11-02.json`. At matched
camera, game time, and executable, cloud plus sun versus sun-only changed
1,373,362 pixels, mean absolute RGB 2.648, with 136,063 pixels above 10. This
outdoor KLATRAP surface has no lamps; a hard cloud edge did not reproduce there.
This is not the owner's dark-street camera. Captures at 12:00 and 17:00 on the
light ground and the owner's spot remain unverified.

Check: Release build passed; CTest passed 18/18; scene-export tests passed
88/88; regression-tool tests passed 12/12; export read 1,467 resources into
`build/scene-cache`. Full pixel regression was stopped on controller direction
after three captures; the script reports no snapshot for the fourth. No shader
change was made. PR #208 remains draft.

Next: Get the owner's exact camera, zoom, and game time, then repeat matched
cloud-on and cloud-off captures before changing the shader.

### 2026-11-04 gpt: release without a reproduction
Done: rebased the existing branch on current `origin/main` (`46f69787`). Inspected the valid light-ground comparison: the hard edge from the owner's dark-street screenshot is not present there, and the owner image still has no sidecar or reliable camera/time match. Only the 08:00 light-ground comparison is valid; required 12:00/17:00 and owner-spot evidence is absent. No shader change is justified without reproducing the defect. The task remains incomplete, the claim is released, and PR #208 stays draft.
Next: resume when the owner's spot can be matched from a sidecar or a reliable camera, zoom and time reconstruction; then collect the required matched cloud/sun evidence.
Check: prior Release build, CTest, exporter tests and regression-tool tests passed as recorded above. No new runtime attempt was made; the branch only rebases the existing evidence and blocker log.

- 2026-11-08 glm: notes merged as PR #208; still blocked on the owner's exact view (no camera or time sidecar).
- 2026-11-12 task 248 audit: missing evidence to determine whether a shader fix is needed: the owner's exact camera/time or a sidecar, followed by matched cloud-on/cloud-off captures at that view.

### 2026-11-12 glm: the edge did not reproduce in three hour frames; the hunt needs a controlled pair

Attempted on fresh main b3820c487: the owner's spot (kladwtwn e0, view
tile 24678, the downtown road) captured at hours 8, 12 and 17 with clouds
on (build/probe-143-h8,h12,h17/). The road region shows dimming that
moves between the hours, but no hard curved line stands out in the crops,
and a sharp-step measurement of the road region is inconclusive: the
road's own curb and art edges dominate (sharpest step 170 in every frame,
the same as the owner crop's 143-style edge). The clouds drift, so a
single frame cannot pin the cover's edge; the acceptance's evidence needs
a controlled pair (the same camera and moment with clouds off and on) and
the shader's 5-bit quantization suspicion (138's cloudLoss5 on dark
ground) stays the first suspect to confirm once the pair exists. Task
stays open; nothing changed.
- 2026-11-12 glm (bookkeeping): status blocked: three hour frames did not
  reproduce the edge (#450); the hunt waits for a controlled clouds-off
  and clouds-on pair at one camera and moment.


### 2026-11-12 gpt: controlled toggle evidence still unavailable
Blocked: Two controlled capture passes at KLADWTWN e0, view tile 24678, did not reproduce the owner's curved road edge. The owner image is a crop without camera/time metadata; the candidate full frame does not establish a reliable match. No shader change was made.

Attempt 1 captured cloud-only, sun-only, and combined using `-CloudShadows`, `-SunShadows`, and `-Shadows`. Attempt 2 repeated the three modes with `-OwnerShot -OwnerShotDelay 2`; it still emitted no owner-shot sidecar. The devshot JSON omits effective `scene3d.shadows` and `scene3d.cloud_shadows`, and process logs do not report those settings. `sun-only` and `combined` emitted `SHADOW PASS`; `cloud-only` did not, as expected with sun shadows disabled, but this does not independently confirm the cloud toggle. Do not treat these as validated toggle-matched evidence.

The devshot sidecars do confirm equal `game_time=288000` (08:00), view tile, fixed camera, 1920x1080, scale 2, and executable SHA-256 across the second pass. No `-CloudPhase` override was used; `shadow.cc` derives cloud offset from game time when no override is supplied. Requested modes, metadata, image comparisons, and limitations are recorded in `runs/143-cloud-edges-soft/controlled-capture-audit.json`; JPEGs are in the `attempt-08-*` folders. These names identify requested parameters only; effective lighting toggles are unconfirmed, so the captures are not validated mode evidence.

Check: `powershell -NoProfile -ExecutionPolicy Bypass -File tools/configure-build.ps1 Release` passed; scene export read 1,468 resources to isolated `build/scene-cache`; six background captures completed. No tests or perf-check were run because renderer code did not change. No files under `game/` or `build/` were committed.
Next: Obtain an owner camera/time sidecar or independent runtime metadata for effective cloud-shadow settings, then repeat the controlled comparison before considering a general shader change.
