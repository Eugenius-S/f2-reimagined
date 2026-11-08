---
id: 226
title: The exporter builds coast11 (a forced solid whose first piece has no card)
milestone: M1
status: done
owner: claude
suggested: claude
priority: high
tier: 0
claude-only: no
user-check: no
depends-on: []
reviewed: yes
---

## Goal

`export.py --maps all` builds every map; coast11 failed with
`'<' not supported between instances of 'int' and 'NoneType'`, so it stayed
2D and `--check` never answered 0.

## Context

Found while timing 220's full export (stands alone: an exporter crash, not
part of the launcher change). A forced solid marked `first.board` in
`forced_boards` even when that piece is a sheet without a card (board None);
every other place that fills the set skips None. The rule: a piece without
a card adds no board mark, on every map.

## Acceptance

- coast11 elevation 0 exports and starts in 3D (`dev-snapshot.ps1 -Map coast11`).
- Exporter tests pass.

## Out of scope

Other maps: the branch only ran when a board was None, which always crashed,
so no other map's output changes.

## Log

### 2026-11-08 claude
Done: `tools/scene_export/vendor/fp/scene.py` skips the None board.
Next: nothing.
Check: `py -3 tools/scene_export/export.py --game-root <game> --map coast11 --cache-root build/scene-cache`, then `tools/dev-snapshot.ps1 -Map coast11`.
