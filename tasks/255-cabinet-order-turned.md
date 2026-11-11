---
id: 255
title: Verify cabinet ordering with the camera turned
milestone: M1
status: todo
owner: any
suggested: gpt
priority: normal
tier: 3
claude-only: no
user-check: yes
depends-on: [067]
reviewed: no
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
