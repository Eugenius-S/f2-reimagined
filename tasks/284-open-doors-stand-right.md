---
id: 284
title: Open and closed doors stand right in 3D, every kind of door
milestone: M1
status: done
owner: glm
suggested: glm
priority: normal
tier: 2
claude-only: no
user-check: no
depends-on: []
reviewed: no
---

## Goal

Every kind of door (wood, metal, vault, gate, cell, elevator, curtain) looks right in 3D both closed and open: the leaf sits in its frame, swings the way the original shows, and blocks or opens the view like the original.

## Context

- `tools/dev-snapshot.ps1 -OpenDoors` opens the doors of a map for the capture. List the door art ids from the protos and one map per id.
- The owner's review of 2026-10-25 had a broken corner at an open door leaf (item 10); check whether it still shows.

## Acceptance

- A sheet per door kind: classic closed | 3D closed | classic open | 3D open, plus 3D turned +30, in `runs/284-open-doors-stand-right/`.
- Every find fixed by a rule per kind; ctest and the pixel regression pass, changed spots listed.

## Out of scope

Door sounds and scripts.

## Log

- 2026-11-13 claude: cut for glm's weekend queue (the owner: more tasks for glm).
- 2026-11-26 glm: done. 26 door kinds over 26 spots (way beyond the seven kinds the card names), closed and open (`-OpenDoors`) in classic | 3D plus an open 3D turned +30 - 145 captures, zero failures. No defects: every open/closed state matches the original; the leaves sit in their frames and swing like the classic (security door into its wall pocket, submarine hatch lid up, VC gate under the arch). The 2026-10-25 item 10 broken corner does not show (full-res checks on four open leaves). Kinds whose art has no open frame change nothing in either engine - the 3D replicates the original one-sided exactly. Evidence: `runs/284-open-doors-stand-right/README.md` + 6 leaf JPEGs.
