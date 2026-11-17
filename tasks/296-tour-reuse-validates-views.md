---
id: 296
title: A reused fly tour passes the same capture checks as a fresh tour
milestone: M1
status: todo
owner: any
suggested: glm
priority: normal
tier: 1
claude-only: no
user-check: no
depends-on: []
reviewed: no
---

## Goal

`tour.py --reuse` refuses unusable captures instead of treating the stamp and PNG count as sufficient evidence.

## Context

`tools/tour/tour.py:145-155` reuses captures on stamp equality and PNG count, before `check_views` at line 181. `check_views` itself iterates partial slices with `zip`, so explicit count validation belongs with this check. This supports the [fly-tour evidence batch](225-fly-tour-evidence-flight-view-changes.md), not new renderer behavior.

After task 225 (PR 508), also check the shared validation at the sheet-building boundary with `--skip-failed`: a capture group can have the full PNG count while failing `check_views`. Add a fixture ensuring such a group cannot count as valid sheet evidence. This is a validation gap to verify, not an observed visual defect.

## Acceptance

- Reuse runs the shared view validation before accepting saved captures. Validate exact turn counts, matching map/elevation/tile sidecars, full 3D renderer state and decodable PNGs.
- Tests cover missing sidecars, wrong tile, a corrupt PNG, incomplete turns and a valid saved tour. Invalid captures fail clearly or are recaptured; they never produce successful evidence from the invalid set.
- A fresh valid tour and an unchanged valid reused tour produce equivalent sheets. `py -3 -m unittest discover -s tools/tour -p 'test*.py'` passes.

## Out of scope

New tour spots (303), changing screenshots or renderer settings, and replacing the current stamp with an evidence database.

## Log

- 2026-11-15 gpt: cut from the tooling and verification follow-up to the existing batches; source observations above are scoped gaps, not unverified game bug claims.
- 2026-11-18 claude: claimed. Small synthetic fixtures and unit tests only, no game, tour, export or perf runs.
- 2026-11-18 claude: done in PR 530, code and synthetic tests only.
  - `check_views` is now the one shared check for fresh, reused and sheet shots. After the start shot it wants exactly one PNG per spot and turn, so an incomplete or extra set fails with `<map> eN: X views for Y stops`. Every view needs a sidecar (`no sidecar for <png>`; damaged JSON counts as missing). The sidecar must show this map, elevation and tile with the full 3D scene. Each PNG must decode (`<png> is not a readable PNG`), and a spot's turns must differ.
  - **`--reuse`.** When the stamp matches, the saved shots still go through `check_views`. A failure prints `saved views unusable, shooting again: <reason>` and the group is shot again. A reshoot that fails stops the group, as before, and leaves no stamp.
  - **Sheets.** They run `check_views` on both sides of each group. Without `--skip-failed` a failure stops with the reason. With it, the group is skipped, so a set that is full by count but invalid no longer counts as evidence.
  - **Tests** (`test_tour.py`, 12 pass). Each fixture first passes as a valid saved tour, then is spoiled one way: a missing sidecar, a wrong tile, a corrupt PNG, an incomplete turn. Also covered: a valid tour is reused without a reshoot; a spoiled one is shot again; a bad reshoot stops with no stamp; sheets refuse or skip a full but invalid group. A fresh and a reused run of the same synthetic set give byte-identical sheets. dev-snapshot is a fake in all of these.
  - **Mutations.** All 6 mutants of the fix were killed: no count check, missing sidecar accepted, unreadable PNG skipped, reuse without checks, sheets without checks, a bad reshoot stamped.
  - **Not verified:** a real tour run, since no game was allowed. Spots, the renderer and the stamp are unchanged. Next to 301's `snapshot-output.json` the check still works, because a JSON with no `image` key matches no view; that pairing is left to the independent review.
