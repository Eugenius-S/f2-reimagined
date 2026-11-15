---
id: 099
title: Point lights next to cut-out pixels no longer get a broken wall normal
milestone: M1
status: done
owner: gpt
suggested: gpt
priority: normal
claude-only: no
user-check: yes
depends-on: [098]
reviewed: yes
---

## Goal

The point lights (barrels, slime, lamps) light walls the same way next to a
cut-out pixel as anywhere else, as the sun already does since 090.

## Context

- Found in 090 (inbox, 2026-10-26): the point lights' wall normal in
  `sceneLighting` (`uLightMode` 2) takes `dFdx/dFdy(vWorld)` after the alpha
  `discard`. Next to a discarded pixel NVIDIA gives zero derivatives and the
  normal turns NaN.
- 090 fixed the same thing for the sun only; read its fix in the scene
  shader and do the same for the point lights.
- In 090 the same change moved 179 to 1155 px in the caves (klaratcv,
  klatoxcv), so this needs a look check there.

## Acceptance

- before | after sheets of klaratcv and klatoxcv near their lights, and one
  lit Klamath spot (barrels or slime), in `runs/099-point-light-normal-nan/`,
  with the changed pixel count; no new dark or bright specks in the after.
- The original view regression stays pixel for pixel; ctest passes.

## Out of scope

The sun, the light colours and their strength.

## Log

### 2026-10-27: point-light face normal

- Reused `gFaceCross`, computed before alpha discard, for scene point lights; no sun, color, or strength changes.
- Targeted 3D before/after: KLARATCV e0 tile 17541: 1,179 changed RGBA pixels; KLATOXCV e0 tile 17941: 929; KLATRAP e0 tile 19721: 4 only with dev test ambient override 16384 (normal ambient=1.0 gives 0). Captures were 1920x1080, scale 2, matching scene/camera metadata. Sheets and enlarged changed-edge crop are in `runs/099-point-light-normal-nan/`; full captures are in ignored `build/099-raw/`.
- Direct final 3D RGBA regression against clean 098 baseline: 10/16 exact. Six Rat/Toxic cave points changed (204, 1,175, 676, 764, 300, 861 px respectively), within intended point-light scenes. Classic `-Still` framebuffer control: 16/16 exact 0 px with matching metadata, `scene_view=off`. Twelve used direct startup targets; four maps that emitted no screenshots on direct startup were captured using the existing test `-Tour` route from KLADWTWN.
- CTest 18/18 and regression Python unit tests 12/12 pass. After rebasing onto merged 098, updated exactly six cave references from isolated captures; their sidecars retain camera/map/elevation/tile/resolution/scale/CE commit/EXE SHA. Full final regression: 15/16 pass 0 px; KLAGRAZ e0 retains its baseline 132 px reference difference and is logged in `tasks/inbox.md`. Refresh details and raw logs are in the evidence report and `build/099-raw/reference-refresh-6/`.
- Clean 098 baseline EXE SHA256: `23a382f0d445f924bba10acdd4e58e6839f112b637803d0a79398d532ad1984c`; pre-rebase targeted fixed EXE: `4f2ce48a9aa7dedfd0f12e0dce07bd226f8ece6b3e56f7f5e4b7edf415c3ca9f`; final rebased EXE: `e8bb1bbff72f582d9d14f4c407920a06ad4e83c94c7d4d9d06952a49c184c730`.

### 2026-11-12 gpt: review of PR #140

Reviewed merged PR #140 and its complete diff. The shader change is limited to point-light wall mode 2: it normalizes the face cross product computed before alpha discard; the sun branch and point-light colors/strength are unchanged. All PR #140 CI checks passed. Inspected the Rat cave, Toxic cave, Klamath barrels before/after sheets and the 3x changed-edge crop in this folder. The cave changes follow cutout contours; no isolated new dark or bright speck is visible. The spot counts are 1,179, 929, and 4 pixels respectively.

Acceptance coverage: KLARATCV and KLATOXCV have matched 1920x1080 scale-2 before/after frames and changed-pixel counts. KLATRAP contains a barrel spot, but its four-pixel change is visible only with the dev ambient override 16384; at normal ambient 1.0 the evidence report says there is no measurable change. This is supporting evidence, not proof of a visible normal-settings Klamath effect. Follow-up task 257 asks for a naturally visible Klamath point-light pair at normal settings.

Regression limits: the six changed 3D cave views are expected lighting differences, so the 3D suite is not pixel-identical. The updated references include KLATOXCV e1, whose old reference already differed from clean task 098 baseline by 492,039 pixels. Classic mode is correctly reported as unchanged: 16/16 exact, scene_view=off. CTest 18/18 and regression Python tests 12/12 pass per the committed evidence. Full regression is 15/16 because KLAGRAZ e0 retains the same 132-pixel baseline deviation. I did not rerun these historical captures or test the shader on another GPU.

Decision: reviewed yes. The shader fix is supported by the before/after evidence and the Classic control. The normal-settings Klamath visibility gap is recorded as a separate follow-up, not treated as a demonstrated defect in this PR.

- Owner review pending.
