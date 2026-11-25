---
id: 281
title: Geometry re-walk: NCR, the Vaults, San Francisco, Navarro, the Enclave
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

Every spot of the listed maps is looked at with fresh eyes in the game camera and in a turned view, and each flaw is fixed or handed to its batch.

## Context

- A slice of 224 (the geometry second pass), cut by region so the walk has an end. Maps: NCR, Vault 13, Vault 15, the Military Base, San Francisco, Navarro, the Enclave.
- Spots: `tools/regression/spots.json` and `tools/tour/spots.json` for those maps. A find is pinned by spot, hex and frame before any fix.
- The owner's rule (2026-11-04): a fix is one rule for a kind of object across every map. A wall find goes to 189, a furniture or prop find to 191, each as a numbered line with its crop; a find of another kind gets its own PR here.

## Acceptance

- `runs/281-rewalk-ncr-vaults-sf-enclave/README.md`: one line per spot (clean, or the find: hex, what is wrong, frame).
- Every find fixed here, or written as a numbered line in 189, 191 or the inbox with the evidence link.
- No reference refresh without the owner's word; ctest and the pixel regression pass on each fix.

## Out of scope

Speed, lighting levels and the UI.

## Log

- 2026-11-13 claude: cut for glm's weekend queue (the owner: more tasks for glm).
- 2026-11-25 glm: walk complete at main cf4c2f5d (21 maps, 60 spots, game camera and +45 fly turn each). One defect: sfshutl2 e0 around tile 20985 shows a field of red CLOSED ART! placeholder cards in 3D where the classic 2D draws an empty black picture; evidence and repro in runs/281-rewalk-ncr-vaults-sf-enclave/, the find is cut as task 329. Everything else clean.
