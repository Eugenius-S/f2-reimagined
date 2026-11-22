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

## 2026-11-23 Windows evidence

Verified one actual F12 screenshot in Preferences with the open panel and on rndbridg and arbridge with live 3D. Tested the region files separately outside combat. Two offscreen combat confirmation fixtures were inconclusive; do not claim the real input route is fully certified there or in every menu.

[Native acceptance checks](../runs/325-night-checks/README.md).

Follow-up: the dev flight binding used to consume Ctrl+F12 during combat. The patch passes it to the normal owner handler. The two decoded shortcut events saved a region in actual Windows combat without ending the fight. Physical keyboard and window focus are separate. [Evidence](../runs/325-combat-and-exit/README.md).

Additional plain F12 checks covered visible Inventory, Pip-Boy, Character and NPC dialogue via the standard SDL queue. The normal frame request succeeded for each; this fixture discarded the temporary player output before file inspection. The menu and world map still need their own plain F12 checks.
