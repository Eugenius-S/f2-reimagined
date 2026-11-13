---
id: 268
title: The dialogue fixture captures the reply list as the player sees it
milestone: M1
status: doing
owner: gpt
suggested: gpt
priority: high
tier: 2
claude-only: no
user-check: no
depends-on: []
reviewed: no
---

## Goal

`tools/dev-snapshot.ps1 -DialogFixture` saves a frame with the NPC line and the full list of answers, in English and in Russian, at x1 and x2.

## Context

140 (letters cut in the answers) and 193 part A (the answers' hanging indent) are blocked on this: the fixture opens the dialogue, but the answer list is not in the presented frame, and three cursor probes failed (PR #424). `gameDialogGetOptionWindow()` from #424 may help. Find why the options window is missing from the capture (refresh order, a window drawn after the capture) and fix the fixture, not the dialogue code.

## Acceptance

- Four frames in `runs/268-dialogue-answers-in-capture/`: English and Russian at x1 and x2, each with at least one wrapped answer visible.
- The sidecar lists each answer's rectangle from `gameDialogGetOptionLayout`.
- ctest passes; the original view regression stays pixel for pixel.

## Out of scope

Renderer and map data changes.

## Log

- 2026-11-13 claude: cut from the review of gpt's run of 2026-11-12: the blocked evidence tasks share a few harness gaps.
