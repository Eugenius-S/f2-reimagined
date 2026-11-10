---
id: 252
title: CI runs every test the repository has, and the rules say what the gate does not cover
milestone: M1
status: done
owner: glm
suggested: glm
priority: normal
tier: 1
claude-only: no
user-check: no
depends-on: []
reviewed: yes
---

## Goal

Green CI means what the rules say it means.

## Context

- Found by the 2026-11-11 audit (`runs/audit-2026-11-11.md`, problem 12).
  Stands alone: no open task touches the workflow.
- `.github/workflows/verify.yml:182-183` runs only the `tools/scene_export`
  unit tests. The tests in `tools/regression` (the snapshot comparer) and
  `tools/test_check_public_content.py` never run in CI.
- `AGENTS.md`, Review: "The one merge gate is green CI (build, tests,
  regression)". The pixel regression cannot run in CI (it needs the game
  files) and does not.

## Acceptance

- The `exporter` job also runs `python -m unittest discover -s
  tools/regression -p 'test*.py'` and the public-content test; both are
  green on the PR.
- A deliberately broken assertion in one of them, pushed to the PR branch
  and then reverted, shows a red job (the run's link is in the Log).
- `AGENTS.md` says in one sentence that the pixel regression
  (`tools/regression.ps1`) is run by the author before the merge and is not
  part of CI.

## Out of scope

Running the game in CI; the release job (253).

## Log

- 2026-11-11 claude: cut from the audit.
- 2026-11-11 glm
  Done: the exporter job in .github/workflows/verify.yml also runs
  python -m unittest discover -s tools/regression -p test*.py (12 tests)
  and python -m unittest discover -s tools -p test_check_public_content.py
  (6 tests). AGENTS.md Review now says the pixel regression
  (tools/regression.ps1) needs the game files, so CI does not run it: the
  author runs it by hand before the merge.
  Check: the deliberately broken assertion in test_check_public_content.py
  (expecting a fake finding) gave a red exporter job:
  https://github.com/Eugenius-S/f2-reimagined/actions/runs/42878708747
  (conclusion failure); after the revert the same job is green:
  https://github.com/Eugenius-S/f2-reimagined/actions/runs/42880103335.
  Locally both suites pass (6 + 12 tests).
  Next: 146.
