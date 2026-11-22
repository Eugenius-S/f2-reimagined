---
id: 318
title: Take owner screenshots from every menu
milestone: M1
status: doing
owner: gpt
priority: high
tier: 1
claude-only: no
user-check: no
depends-on: []
reviewed: no
---

# 318: Take owner screenshots from every menu

Status: in review. Parent queue: #317 / PR #554.

Route plain F12 through shared input without changing F9, Ctrl+F12, or the active mark's keys.

Windows acceptance: one PNG/JSON pair per press and no screen or renderer change in main menu, options, character sheet, Pip-Boy, dialogue, local 3D/combat and world map. Separately test saving a region with a fresh F12 during combat without ending the fight. Graphical verification pending.
