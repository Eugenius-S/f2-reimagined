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

### 2026-11-27 gpt (implementation pending acceptance)
Done: prepared a run-local Kcsulik message overlay for the plain dialogue fixture, with one long EN/RU answer and the other source messages preserved. The fixture captures the GL back buffer immediately before presentation and collects layouts at that boundary. The existing page, cursor, and layout checks keep their previous paths. Static staging against the isolated RP copy succeeded for both languages; this does not prove the game frames.
Next: after the shared Windows slot is free, build Release and run CTest and the four native captures below. Inspect each NPC line and every answer, including the wrapped lines; verify the seven logical sidecar rectangles against the image, executable hash, commit, map, and scale. Run the three existing diagnostic modes and compare a matched ordinary view from the base and candidate pixel for pixel. Independent review and CI remain pending; status stays blocked.
Check (after build, from this worktree; use only the isolated RP copy):

```powershell
$qa = '<path to an isolated RP game copy>'
foreach ($lang in 'english','russian') {
  foreach ($scale in 1,2) {
    powershell -NoProfile -ExecutionPolicy Bypass -File tools/dev-snapshot.ps1 -DialogFixture -GameRoot $qa -Language $lang -Scale $scale -HeroTile 25075 -CaptureConsole -OutputDirectory "build/268-$lang-x$scale"
  }
}
```

The historical Kcsulik first page has NPC message 796 and seven answers (797, 126, 798, 799, 800, 801, 803). Its unmodified 797 is 30 bytes in English and does not wrap, so repeatedly selecting its first option could not guarantee this test case. The overlay replaces 797 only inside the run's redirected state; no game source file is changed.
