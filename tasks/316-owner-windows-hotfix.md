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

The owner narrowed the immediate Windows hotfix to local 3D continuity, cursor trails, and manual camera travel and clipping.

- Walk Temple to Arroyo and back on the isolated game after combat and without combat. Record renderer and camera state separately. Check the agreed local map overlays and keep the world map 2D.
- Verify cursor movement and cursor shape changes in inventory and combat on Windows. Identify any stale pointer pixels in the presented frame.
- Preserve the additional manual 3D travel on local maps with and without EDG. Check the Temple sanctuary round door, the original and new stops, the lower corners, the return to 2D, and the New Reno floor notch. Compare matched ordinary and turned views and exercise the 3440x1440 borderless layout at 2x.
- Keep the existing region capture and held action menu fixes. Run the Windows Release build, project tests and CI at the final code revision. Submit the draft for Marvin's review before release.

## Next batch

- Isolate the reported silhouette and compare the fixed outline in ordinary and turned views.
- Correct Temple pot PID 243 at both angles and check another prop of the same class.
- Measure entry and exit fades for Pip-Boy, dialogue, inventory and character screens.
- Finish physical input checks for Ctrl+F12 and the held action menu.
- Investigate the reported performance stall independently of cursor trails.

## Out of scope

Changes to the owner's installation, localization, movement, door clickability, authored clipping planes or the 2D world map.

## Log

### 2026-11-22 gpt

The draft contains region capture, cursor cache, manual camera travel and held action menu candidates. Release is blocked pending the open visual and gameplay checks. [The investigation](../runs/316-windows-hotfix/README.md) records each report, the copied Windows fixtures, the observations and the open checks.

### 2026-11-22 gpt, narrowed hotfix

The owner restricted the release review to 3D continuity, cursor trails and camera travel and clipping. The latest code keeps 3D visible in Pip-Boy and continues the dry desert floor at the reported New Reno left edge. The original camera limits and the tile data used by movement and picking are preserved. The full walking route and live borderless door check still need evidence. The next-batch items above are outside this review.

### 2026-11-23 release reconciliation

The Windows hotfix shipped through the cumulative release `039d25da3`. Local 3D under overlays, native combat region capture, cursor trails, the Temple pot and outlines have separate landed fixes. Remaining: the continuous post-duel Temple to Arroyo route on a suitable save, and the round-door/edge sweep in 3440x1440 borderless at 2x. Do not rerun the shipped work as an unreleased batch.

### 2026-11-26 gpt

Reconcile the shipped packet with the remaining acceptance in task 317.
