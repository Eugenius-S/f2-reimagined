---
id: 211
title: The 3D mode toggle responds when it can, and says why when it cannot
milestone: M1
status: done
owner: glm
suggested: glm
priority: high
tier: 2
claude-only: no
user-check: yes
depends-on: []
reviewed: yes
---

## Goal

The owner, 2026-11-07: "the 3D mode button does not work." Toggling the 3D
mode in play must either turn the scene on or say plainly why it did not.

## Context

- The only in-game 3D toggle is the settings row "Enable 3D scene"
  (Включить 3D-сцену); there is no interface-bar button. The row flips
  [3d] enabled, and scene_view syncSettings enables the scene on the next
  frame only when a real map is loaded (not the main menu, not a
  transition).
- enable() refuses with "3D scene is disabled in settings" when enabled is
  off, and showsCurrentMap() gates the enable: a map without a 3D scene
  (no exported scene data) stays 2D silently. A refusal on a sceneless map
  reads to the owner as "the button does not work".
- The owner played on their current save; which map they toggled on is
  unknown, so the exact refusal is unconfirmed.

## Fix

- When the toggle cannot enable the scene, surface the reason in the
  game's status line in Russian (which map has no scene, what failed).
- Check whether the owner's failing case is a sceneless map and whether
  the map list of sceneless locations is worth showing in the toggle's
  description.

## Acceptance

- On a map with a scene the toggle turns the 3D view on and off.
- On a map without one the toggle says so, in Russian, in the game.

## Log

- 2026-11-07: owner report; the toggle path and its silent gates found the
  same day; the reason surfacing is pending.
- 2026-11-08 claude: owner order 2026-11-08: glm, third after 239 and 172.
- 2026-11-09, glm. Done: every refusal of the 3D toggle now lands in the
  game's status line (displayMonitorAddMessage), in the game's language:
  "3D scene is disabled in settings", "no map is loaded", "this map has no
  3D scene: MAP (elevation N)" - a map that was never exported has no
  scene cache directory at all - and "the 3D scene failed to load" for a
  cache that exists but did not load. The wording lives in the scene
  catalog (menu_text.cc toggleRefusalText), covered by settings_3d_tests.
  Both toggle paths go through the same enable(), so the settings row and
  Ctrl+F10 both speak. Evidence in runs/211-3d-mode-toggle-no-response/:
  toggle-scene (Ctrl+F10, sidecars scene_view full -> off -> full),
  row-scene5 (the row toggled on kladwtwn, the scene loads, sidecar
  scene_view=full), row-sceneless10 (the row toggled on sceneless
  vctyctyd: the status line reads "y etoy karty net 3D-sceny: vctyctyd
  (etazh 0)" in the game's CP1251 font). Tooling that made the proof
  possible: the preferences probe's toggle3d mode (Space on the enabled
  row, then Done; the panel gained selectRowForProbe because the display
  rows moved the General group's first slot to the scale row - the old
  probe flow's Space-on-first-row assert is stale on main), the probe
  holds a few rendered frames after the panel closes so the line is on
  screen for the shot, dev-snapshot collects the probe's shots and gained
  -Set3d, and the scripted input gained hold/release for chords.
- For the owner (user-check): the failing case was almost certainly a
  sceneless map - the toggle silently stayed 2D. Which locations have
  scenes depends on the owner's own mod-cache exports, so the toggle's
  description does not try to list them; say the word if it should.
- Find, not fixed here: a devshot startup on a sceneless map never
  settles on its own - the demo map's entry dialog holds the game state
  off Normal, so syncSettings (and the toggle with it) never runs until
  -SkipScriptBlocking. Same family as 111's live-capture defect.
