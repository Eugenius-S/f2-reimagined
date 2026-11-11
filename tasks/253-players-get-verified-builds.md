---
id: 253
title: Players get only a build that passed the launch check
milestone: M1
status: done
owner: glm
suggested: any
priority: high
tier: 2
claude-only: no
user-check: no
depends-on: []
reviewed: yes
---

## Goal

The `latest` release that the launcher installs for players is the same
build the owner got through Rule 1, never a newer unchecked one.

## Context

- Found by the 2026-11-11 audit (`runs/audit-2026-11-11.md`, problem 8).
  Stands alone: 186 and 227 change what the launcher installs, this one
  changes when CI publishes.
- `.github/workflows/verify.yml:56-62`: `publish-latest` runs on every push
  to main. CI has no game files, so nothing there starts the game.
- The launch check of Rule 1 (`AGENTS.md`) runs after the merge on an
  agent's machine. Between the merge and that check players can already
  download the build; a merge that breaks the start reaches them first.

## Acceptance

- `publish-latest` no longer runs on a plain push to main. It runs on
  `workflow_dispatch` with the commit as its input and publishes exactly
  that commit's build.
- `AGENTS.md`, Rule 1: the step that copies the exe to `f2r-verified` is
  followed by `gh workflow run` for the same commit.
- A docs-only merge publishes nothing.
- One real run after the change: `version.txt` of the `latest` release
  equals the last line of `f2r-verified/version.txt` (both in the Log).

## Out of scope

The launcher's code; signing; a second release channel.

## Log

- 2026-11-11 claude: cut from the audit.
- 2026-11-11 claude: glm merged the change in #411 but left the header at
  doing and no Log entry; closed here after checking the result.
  Done (glm): publish-latest runs only on workflow_dispatch with the commit
  as input; AGENTS.md Rule 1 ends with gh workflow run for that commit.
  Check: dispatch run 42897800793 on 96416cf8 is green (the first try,
  42896039108, failed at checkout). version.txt of the latest release:
  96416cf847279fecf6fe07b509948bb1cb9d3123. Last line of
  f2r-verified/version.txt: 96416cf847279fecf6fe07b509948bb1cb9d3123
  2026-11-11 Release ctest 27/27 devshot OK. The docs-only merge #412
  published nothing (release assets still dated 2026-11-10 20:50 UTC).
