---
id: 255
title: Verify cabinet ordering with the camera turned
milestone: M1
status: done
owner: gpt
suggested: gpt
priority: normal
tier: 3
claude-only: no
user-check: yes
depends-on: [067]
reviewed: yes
---

## Goal

Capture and verify that a hero standing in front of a tall cabinet is drawn in front of it when the game camera is turned 45 degrees.

## Context

PR #121 for task 067 states that the hero remains in front of the shelf at a 45-degree camera turn. Its retained evidence only shows the unturned game view; no turned-view capture is present. The depth-plane shader change is not covered by an independent turned-view artifact.

## Acceptance

- In the Klamath shelf scene from `runs/067-card-order-cabinet/front.jpg`, capture the same hero-before-shelf case at 0 and 45 degrees using the background test instance.
- Store a before/after comparison under `runs/255-cabinet-order-turned/`, with the 45-degree after frame visibly showing the hero in front of the shelf, and attach it to the PR.
- Record map, elevation, hero tile, camera angle, build commit, and capture paths in the Log. Confirm the 0-degree control still matches task 067's expected ordering.

## Out of scope

Changing the card ordering or depth-plane implementation; create a separate fix task if the 45-degree capture fails.

## Log

### 2026-11-12 gpt
Done: queued the missing turned-view evidence identified during review of task 067, PR #121. No runtime capture was performed.
Next: capture the same hero-before-shelf case at 0 and 45 degrees with the background test instance; document metadata and compare the result.
Check: PR #121 evidence contains the unturned `front.jpg`; its Log claims the 45-degree case but no turned capture is retained.

### 2026-11-12 gpt
Done: captured the Klamath cabinet scene at 0 and 45 degrees in the background test instance. The hero remains visibly in front of the shelf at both angles. `runs/255-cabinet-order-turned/comparison.jpg` compares the cabinet and hero; `angle-0.jpg` and `angle-45.jpg` retain the full frames, all below 1 MB. `captures.json` records the run IDs, scene cache fingerprint, executable hash, and source paths.
Next: wait for CI, then merge this evidence-only PR.
Check: Release build via `powershell -NoProfile -ExecutionPolicy Bypass -File tools/configure-build.ps1 Release`; cache export via `py -3 tools/scene_export/export.py --game-root <main-checkout>/game --maps klamall --cache-root build/scene-cache`; background snapshots via `tools/dev-snapshot.ps1` at `-Map klamall -Elevation 0 -HeroTile 13086 -ViewTile 13086 -Still -TurnDegrees 0` and `45`. Both reported `KLAMALL.MAP`, elevation 0, hero tile 13086, 1920x1080 at scale 2, scene view full, and executable SHA-256 `77b61d272e8ca93c5fcce093990570f98001e25800d48c1205613d65c0f4aa7f` from build commit `14feea02a`. Visually inspected the contact sheet and compared the 0-degree order with `runs/067-card-order-cabinet/front.jpg`. No rendering code was changed. Tests and pixel regression were not run for this evidence-only task.
