---
id: 303
title: The fly tour reports which selected views it covers and misses
milestone: M1
status: doing
owner: claude
suggested: glm
priority: normal
tier: 1
claude-only: no
user-check: no
depends-on: []
reviewed: no
---

## Goal

Before using a tour as geometry evidence, print its selected map/elevation, spot and turn coverage, including requested maps with no configured spots.

## Context

`tools/tour/tour.py:60-69` filters configured spots and accepts the remaining groups; a multi-map request can omit a map with no spots without an explicit omission report. The default lists in `tools/tour/spots.json` and `rp_spots.json` are curated samples. This supports the [fly-tour evidence batch](225-fly-tour-evidence-flight-view-changes.md) and geometry re-walks [279](279-rewalk-arroyo-klamath-den-modoc.md), [280](280-rewalk-vault-city-gecko-redding-reno.md), [281](281-rewalk-ncr-vaults-sf-enclave.md), [282](282-rewalk-encounters-and-rp-maps.md).

## Acceptance

- Add a small no-capture coverage report using the existing spot lists: requested maps, configured map/elevations, named object/landmark spots and exact turns. Explicitly list requested maps with no spots; do not silently claim them covered.
- For a real geometry re-walk selection, save the report as text in its evidence Log and list the omitted views that still need configured spots or owner captures. A named spot proves only that view, not every object of its class.
- Fixtures cover a partially configured two-map request, unknown map, an explicit elevation gap in a supplied expected selection and a fully covered selection. Existing tour tests pass.

## Out of scope

271's map load/FPS/black-frame measurements, automatic spot generation, declaring every object covered, filling every town's spot list, or making a new scene inventory framework.

## Log

- 2026-11-15 gpt: cut from the tooling and verification follow-up to the existing batches; source observations above are scoped gaps, not unverified game bug claims.
- 2026-11-18 claude: claimed. No-capture report and synthetic fixtures only, no game, tour, export or perf runs.
