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
