---
id: 238
title: The Russian translation reaches players behind a VPN through our mirror
milestone: M1
status: done
owner: any
suggested: claude
priority: high
tier: 0
claude-only: no
user-check: yes
depends-on: [235]
reviewed: no
---

## Goal

A player whose VPN blocks github.com and api.github.com installs the game and
the official RPU Russian translation, and sees the launcher log in Russian when
the launcher is in Russian.

## Context

Task 235 gave our own release a `dist` mirror on raw.githubusercontent.com, but
left RPU's translation out (no license file). Through the owner's VPN the game
then installed and the translation failed. The owner decided (2026-11-08): put
the RPU Russian translation into our mirror.

## Acceptance

- CI puts `rpu_russian.dat` and `rpu-version.txt` (RPU's tag) on `dist`, in the
  same single commit. If RPU cannot be reached the previous dist copy is kept;
  with neither, the job fails.
- The Russian install tries RPU on github.com and the api first, then the
  mirror; the log names the route that worked.
- The Russian checkbox is the player's wish (`russian_wish` in launcher.ini);
  every Install / Update and the auto update install a missing translation; a
  failed attempt leaves the box checked and the status says it will be tried
  again. Unchecking an installed translation still asks before removing it.
- The worker prints its user-facing lines in Russian with `--ui-language ru`;
  machine lines and the progress parsing are unchanged.
- Tests for the Russian table and the progress parsing of Russian lines.

## Out of scope

The translation's license (the owner took the decision); GUI layout.

## Log

### 2026-11-08 claude
Done: see the PR. CI step in `publish-latest`; `ce/src/launcher_main.cc` (mirror route, `--ui-language`, `--mirror-url`, exit code 2 when only the translation failed); `ce/src/launcher_ru.h` (the Russian log table); `ce/src/launcher_ui.cc` (the wish, retry, status); `launcher_progress.h` reads the Russian lines.
Next: after the first CI run on main, check that `dist` has the two files and that a VPN player installs the translation.
Check: `ctest --test-dir build`; `fallout2-ce-3d-launcher.exe --game-root <scratch> --no-start --russian --ui-language ru`.
