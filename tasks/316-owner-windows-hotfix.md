---
id: 316
title: Cumulative Windows hotfix for the owner's Temple reports
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

Diagnose the player-release reports and verify the fixes together on an isolated Windows install.

## Context

The owner requested one urgent batch. [Inbox PR #552](https://github.com/Eugenius-S/f2-reimagined/pull/552) records the exact Temple viewpoints and the inventory recording. Reproduce against the installed `d01e850` release and implement on current main, including the Russian text choices from #551.

## Acceptance

- Region capture writes PNG and JSON with a second Ctrl+F12 during and outside combat, gives the same instruction in its prompt, and cancels with Esc without affecting the turn.
- Check pointer movement and cursor changes in inventory and combat. Investigate stale cursor pixels independently of the recorded stall.
- Identify the visible outline layer and compare large crops in ordinary and turned local views. The outline follows the visible silhouette.
- Identify the Temple pot and verify the depth-order rule in the original and turned views and on another prop of the same class.
- Reproduce the sanctuary door camera limit and allow roughly one third of the visible viewport beyond the local edge. Apply manual 3D panning to local maps with and without EDG. Check the clean cut in the new travel area, other edges and a corner in ordinary and turned views at standard and ultrawide sizes.
- Check entry and exit of fullscreen UI screens and identify any confirmed extra or lingering black frame. Preserve intentional fades.
- Verify local 3D across combat end, Temple exit after combat and without combat, and Temple reentry, recording renderer and camera state separately. Keep the world map 2D.
- Hold the action-menu button over a target, move to a visible row and release to select it. Check fast motion and the bottom-shifted menu. Keep the local view in 3D throughout the gesture.
- Run project checks, CI and an independent review. Include matched before and after evidence for visual fixes and leave unconfirmed reports open.

## Out of scope

Changes to the owner's installation, localization, movement, door clickability, rendered map boundaries or the 2D world map.

## Log

### 2026-11-22 gpt

The draft contains region capture, cursor cache, manual camera travel and held action menu candidates. Release is blocked pending the open visual and gameplay checks. [The investigation](../runs/316-windows-hotfix/README.md) records each report, the copied Windows fixtures, the observations and the open checks.
