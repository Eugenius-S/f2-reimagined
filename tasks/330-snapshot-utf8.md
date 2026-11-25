---
id: 330
title: Developer snapshots write valid UTF-8 JSON
milestone: M1
status: doing
owner: gpt
suggested: gpt
priority: normal
tier: 1
claude-only: no
user-check: no
depends-on: [183]
reviewed: no
---

## Goal

Developer screenshot sidecars remain readable as UTF-8 JSON with localized game data.

## Context

Follow-up to finished task 183. During PR #588's Russian Klamath world-map run, two sidecars per run contained the game-encoded no-map placeholder. Strict UTF-8 decoding rejected those files.

## Acceptance

- A Russian world-map and a local-map developer snapshot parse as strict UTF-8 JSON with their intended values.
- Any variable game text in the affected writer is encoded and escaped. Existing keys, numeric fields and paths used by capture tools stay compatible.
- The checks cover CP1251 and JSON escaping of quotes, backslashes and control characters.

## Log

### 2026-11-25 gpt

Done: The legacy sidecar now escapes and converts game text from CP1251 to UTF-8 using the same helper as owner shots. The undefined CP1251 byte becomes U+FFFD. The tools commit argument is escaped too. The JSON keys and paths are unchanged.

Check: Windows Release and CTest 34/34 passed; snapshot_rows_tests covers Russian, CP1251 extensions, undefined byte, quotes, backslashes and controls. Strict Python UTF-8 decoding rejects `build/147/final1/devshot-20261124-214655.json` in the read-only 147 worktree at byte 76. On an isolated game copy, the Russian local-map sidecar parses with map `KLADWTWN.MAP`. A separate temporary test build (exe SHA256 `f5d25becdd57d94e303787a0d46b5dad1545be5bd8ee8d0ceb386a498ac3ba3c`) used PR 588's scripted input and world-map pump only for the Klamath exit and return: the actual world-map sidecar parses strictly and its map is `(нет)`; three route snapshots parsed. The fixture changes were removed from the source. Test captures are in ignored `build/330-world` and `build/330-local`. The initial 3D launch without a scene cache timed out; the route used a private copy of the Klamath scene cache.

Next: Independent review and CI; no merge or installer publication by this worker.
