---
id: 296
title: A reused fly tour passes the same capture checks as a fresh tour
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
