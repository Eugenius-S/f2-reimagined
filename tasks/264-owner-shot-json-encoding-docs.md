---
id: 264
title: Owner shot docs describe current JSON encoding
milestone: M1
status: todo
owner: any
suggested: any
priority: normal
tier: 1
claude-only: no
user-check: no
depends-on: []
reviewed: no
---

## Goal

Make the owner-shot guide tell agents how to read the JSON files produced by
the current game build.

## Context

`ce/src/devtools/owner_shot.cc` converts game strings from CP1251 to UTF-8
before writing JSON. The current `docs/owner-shots.md` still says those JSON
strings keep CP1251 and should be read as game data. That advice is wrong for
current shots. The committed sample sidecars in task 183 predate the UTF-8
fix in PR #334, so document their legacy encoding separately if they remain
part of the guide.

## Acceptance

- The guide says current owner-shot JSON is UTF-8 and Russian map, city and
  object names are decoded normally by JSON readers.
- Any retained pre-#334 sample is clearly labeled as a legacy CP1251 example.
- A small check or documented command successfully reads a current-format
  UTF-8 sidecar and its Russian string fields.

## Out of scope

Changing runtime encoding or the screenshot image format.

## Log

- 2026-11-12, gpt: created from review of task 183. Current source converts
  game strings to UTF-8; the guide's CP1251 instruction is stale.
