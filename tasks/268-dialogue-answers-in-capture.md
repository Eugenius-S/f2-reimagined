---
id: 268
title: The dialogue fixture captures the reply list as the player sees it
milestone: M1
status: blocked
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

### 2026-11-13 gpt
Done: removed the pre-dialogue baseline image from `-DialogFixture`; added logical answer rectangles from `gameDialogGetOptionLayout` to the snapshot sidecar; required every answer layout before capture; added a natural first-choice route when the current English list has no wrapped answer. The initial unmodified x2 English capture at `build/diag-268-before/scr00000.png` visibly contains Kcsulik's NPC line and all seven answers, but no answer wraps. It also exposed the cause of the misleading output: the tool returned a static preview without dialogue before the valid dialogue frame.
Next: blocked. Kcsulik's first English answer list has seven unwrapped answers. Two modified-source English x2 captures did not find a natural wrapped page: `build/diag-268-after-en-x2/process.stdout.log` ended with `answer-list-did-not-change-after-key-1`; `build/diag-268-after2-en-x2/process.stdout.log` showed six first-choice advances with 7 and 2 options, then `no-wrapped-answer-after-six-natural-first-choice-pages`. No screenshot was produced by either attempt. The latest rectangle-based option-window lookup and sidecar output compile, but they have not been exercised at runtime. Need a reproducible English dialogue page with a naturally wrapped answer before completing the four-frame matrix.
Check: `powershell -NoProfile -ExecutionPolicy Bypass -File tools/configure-build.ps1 Release` passed after the latest source change; `ctest --test-dir build --output-on-failure` passed 27/27. `powershell -NoProfile -ExecutionPolicy Bypass -File tools/regression.ps1` began exporting all maps to `build/scene-cache` but did not reach comparisons or write a summary; stopped only this run's process tree after about four minutes so the next checked build can proceed. Pixel regression remains unverified. No game files or cache outside this worktree were changed.
