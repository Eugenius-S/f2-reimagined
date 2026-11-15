---
id: 292
title: The launcher loses its duplicate download code and the save hash check
milestone: M1
status: todo
owner: any
suggested: any
priority: low
tier: 3
claude-only: no
user-check: no
depends-on: [287]
reviewed: no
---

## Goal

The launcher's three HTTP functions share one request opener, the before and after hash of the saves is gone, and `fallout2.cfg` is rewritten through a temporary file.

## Context

- `httpGet`, `httpRedirectLocation` and `httpDownload` in `ce/src/launcher_main.cc` repeat the session, connect and request setup. The cut-short fix (#473) reached only `httpDownload`: `httpGet` still takes a dropped answer for the whole text.
- The `sha256Tree` check around the game run is a leftover of 091's acceptance: it prints CHANGED whenever the player saved.
- `setLanguage` truncates the player's `fallout2.cfg` and then writes it; the file header and `uninstallMod` say the cfg is never touched.

## Acceptance

- One opener used by the three functions; `httpGet` rejects an answer shorter than its Content-Length.
- The hash lines and `sha256Tree` removed; nothing else reads them (grep in the Log).
- The cfg is written to a temporary file and renamed over the old one; the comments say what is changed.
- ctest passes; an install, an update and a Russian install from a local test server still pass.

## Out of scope

New launcher features.

## Log

- 2026-11-15 claude: cut from the repo audit of 2026-11-15 (find 8, 9 and 10).
