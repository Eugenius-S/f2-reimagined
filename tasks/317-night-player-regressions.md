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
