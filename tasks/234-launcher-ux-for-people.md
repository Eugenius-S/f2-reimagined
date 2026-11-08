---
id: 234
title: The launcher speaks to people - remove mod, Russian checkbox, honest errors
milestone: M1
status: open
owner: any
suggested: claude
priority: high
tier: 0
claude-only: no
user-check: yes
depends-on: [232]
reviewed: no
---

## Goal

The owner's launcher review (2026-11-08), all in `ce/src/launcher_ui.cc` and
`launcher_main.cc`:

- "Rollback" is disabled after a fresh install. Replace it with "Remove mod",
  which restores the game as it was before the mod and always asks for
  confirmation first.
- The Russian checkbox is checked when the translation is installed.
  Unchecking it asks "Remove the Russian translation?" and removes it on yes.
- Windows showed "this program might not have installed correctly" after
  `Install-F2R.exe` closed. Add a manifest (`asInvoker`, `supportedOS`) so the
  installer-name detection stops, and check that the prompt is gone.
- Errors say what happened in plain words: "GitHub does not answer" rather than
  "no network" (the route fix itself is task 235).

Starts from main after 232 lands, since both change the launcher window.

## Acceptance

- Each point above is seen in the launcher window, in English and Russian.
- Remove mod returns a clean Fallout 2 that the original game starts from.

## Log

- 2026-11-08 claude: cut from the owner's review.
