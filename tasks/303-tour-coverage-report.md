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
- 2026-11-18 claude: done in PR 532, code and synthetic fixtures only.
  - **Report.** `tools/tour/tour.py --coverage [--maps ...] [--expect map:elevation,...]` shoots and needs no sides or game. It prints:
    - the exact turns and the requested maps;
    - each covered map elevation, with its count of spots, turns and views and every named spot (tile and name);
    - then `missing <map>: no configured spots` for each requested map without a spot, and `missing <map> eN: expected, no configured spots` for each `--expect` elevation without one, or `missing: none`.
  - Without `--expect` it says elevations were not checked. It always adds that a named spot proves only its own view. The same report prints before every tour. The spot selection is now one shared `spot_groups` for the report and the tour.
  - **Tests.** `test_coverage.py` (6) covers a partly configured two-map request, an unknown map, an elevation gap in an expected selection, a fully covered selection, every map, and the command line without `--before`. All 12 tour tests pass, and all 6 report mutants were killed.
  - **Real selections.** `runs/303-tour-coverage-report/` holds the reports for re-walks 279-282, with the maps taken from the exporter's region groups. Maps still without spots:
    - 279: arvill2;
    - 280: bhrndmtn, newr1a, newr2a;
    - 281: none at map level;
    - 282: coast3, 5-9, 11, 12, desert1-4, 6, 8, 9, desrt11, desrt12, cowbomb; RP easter, gammovie, gcenclen, merchant, v13_orig, v15_orig, vill.
  - **Not verified:** elevations, which need the map headers from the game data and so were not read. Spots, the renderer and the scene set are unchanged. The re-walk cards belong to GLM and were not edited; when a re-walk runs, it can copy its report into its Log.
