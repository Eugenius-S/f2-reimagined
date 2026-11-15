---
id: 287
title: A failed update is never written down as done
milestone: M1
status: todo
owner: any
suggested: gpt
priority: high
tier: 0
claude-only: no
user-check: no
depends-on: []
reviewed: no
---

## Goal

When the launcher cannot unpack or copy a part of the new build, it stops with a clear message and leaves `version.txt` at the old build, so the next start tries again.

## Context

- `ce/src/launcher_main.cc`, the block after `installing into the game folder:`: the results of `copyIfPresent` and `copyLauncher` are ignored and `version.txt` gets the new version whatever happened. With the game running, the exe copy fails, the version is written, and every later start says the build matches the release.
- `unpackZip` in the same file treats the archive as unpacked once `tar.exe` started: `runTool` returns the exit code, nobody reads it, and the Expand-Archive fallback runs only when tar never started. A full disk gives a half-unpacked stage that installs as `not in the package, skipped`.
- The pure decisions already have a test home: `ce/tests/launcher_progress/`.

## Acceptance

- An update with the game exe locked (a test holds the file open) ends with an error naming the file; `version.txt` still holds the old build; the next run with the file free installs and writes the new one.
- A corrupt zip from a local test server (`--update-url`) fails on the unpack with its message, and the fallback is tried when tar exits non-zero.
- The required files of the package (the game exe, `ce.dat`, the exporter) are checked before the first copy; a missing one stops the install with nothing changed.
- ctest passes; the launcher's Russian lines cover the new messages.

## Out of scope

A rollback of files already copied (the backup made before the update covers it).

## Log

- 2026-11-15 claude: cut from the repo audit of 2026-11-15 (find 1 and 3).
