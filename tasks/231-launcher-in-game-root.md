---
id: 231
title: The game folder holds the launcher as Fallout 2 Reimagined.exe with the project icon
milestone: M1
status: done
owner: claude
suggested: claude
priority: high
tier: 0
claude-only: no
user-check: yes
depends-on: []
reviewed: yes
---

## Goal

After the install the game folder root holds our launcher under a clear name,
`Fallout 2 Reimagined.exe`, with the project's radiation icon, and it works
when started from there.

## Context

The owner: the installed folder has `fallout2-ce.exe`, `fallout2.exe`,
`fallout2HR.exe`, `Fallout2Launcher.exe`, `FalloutClient.exe`,
`scene-exporter.exe` and more, and "I don't understand what to run".

## Acceptance

- `ce/src/launcher.ico` (the owner's `f2r-radiation.ico`) is the launcher exe's icon.
- A fresh install and every update copy the launcher from the release zip to
  `Fallout 2 Reimagined.exe`, also when that very file is the running launcher
  (renamed aside to `.old`, deleted on the next run).
- The launcher copy is backed up and rolled back with the other build files.
- Run from a folder holding `master.dat` and `version.txt`, with no
  `--game-root`, the launcher (console and GUI) uses that folder; elsewhere
  registry, stored root and picker work as before.
- Both READMEs and the release notes name `Fallout 2 Reimagined.exe`.

## Out of scope

A desktop or Start menu shortcut; hiding the other exes.

## Log

### 2026-11-08 claude
Done: ctest 20/20. Against a local http update source (zip built like the
workflow's staging): fresh install put `Fallout 2 Reimagined.exe` in the root;
running that exe with no `--game-root` took its own folder, updated v1 to v2
(including itself, `.old` left), and `--rollback` restored v1 and the launcher.
Icon extracted from the installed exe is the radiation icon.
Next: none.

### 2026-11-08 claude
The owner: the launcher must warn that the first install is long. The GUI status says the first install takes about 20 minutes before and during it (short, to fit the one-line status), and the console log says so when it starts building every map. Release build and ctest 20/20.
The owner: the download is named Install F2R with the project icon. The release asset is now `Install-F2R.exe` (GitHub turns a space in an asset name into a dot), the old `fallout2-ce-3d-launcher.exe` asset is deleted on publish, the release notes are refreshed on every publish, and both READMEs link the new name. The zip keeps the launcher as `fallout2-ce-3d-launcher.exe`, which the in-place update installs as `Fallout 2 Reimagined.exe`.
