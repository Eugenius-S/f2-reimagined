---
id: 266
title: Test the all-map shape sweep without game data
milestone: M1
status: done
owner: gpt
suggested: any
priority: normal
tier: 3
claude-only: no
user-check: no
depends-on: []
reviewed: yes
---

## Goal

Cover the map and elevation loop, per-elevation failures, and resource cleanup
in `tools/shapes/sweep_all.py` with isolated unit tests.

## Context

This stands alone: the sweep is a tools script with no direct unit coverage.
The tests use fakes and do not read the game's files or launch it. CI already
discovers `tools/shapes/test*.py`.

## Acceptance

- Tests verify sorted map handling, all three elevations, successful builds,
  reported `Exception` and `SystemExit` failures, and game-file closure.
- A named output stream is closed after the sweep.
- The shared shapes test discovery runs the tests.

## Out of scope

Runtime or sweep implementation changes.

## Log

- 2026-11-12, gpt: added isolated tests using fake maps/builders. The tests
  cover sorting, elevations, errors, and closure of game and output resources.
  Checks: `py -3 -m unittest discover -s tools/shapes -p 'test*.py' -v`,
  `py -3 tools/check_public_content.py origin/main`, and `git diff --check`.
  Draft PR: #484.

- 2026-11-13, gpt: review found the fake builder did not prove the sweep called
  either build method. The test now asserts `build_tiles()` then
  `build_objects()` for every successful map elevation. Checks: the same shapes
  discovery (11 tests), public-content check, and `git diff --check` pass.
  Draft follow-up PR: pending.
