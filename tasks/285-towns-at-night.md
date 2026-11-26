---
id: 285
title: Every town at night: lamps and fires light the street, nothing is pitch black or full bright
milestone: M1
status: doing
owner: glm
suggested: glm
priority: normal
tier: 2
claude-only: no
user-check: no
depends-on: []
reviewed: no
---

## Goal

At 22:00 each town in 3D reads like the original at the same hour: lit windows, lamps and fires light their surroundings, dark corners stay dark but readable.

## Context

- 262 and 263 check the ambient level on encounter maps; no task sweeps the towns at night.
- `tools/dev-snapshot.ps1 -Hour 22`, with `-Classic` for the original side. The classic clock ladder is 267's work: if it is not merged, capture the classic side per map by hand and say so.

## Acceptance

- Classic | 3D pairs at 22:00 for one outdoor spot of every town map in `runs/285-towns-at-night/`, with the mean brightness of both sides in the README.
- Each pair whose brightness differs by more than a third, or where a light source the original shows lit is dark, is a find: fixed by a rule, or pinned with its cause.
- ctest and the pixel regression pass.

## Out of scope

Encounter maps (262, 263) and the glow's look (022).

## Log

- 2026-11-13 claude: cut for glm's weekend queue (the owner: more tasks for glm).
- 2026-11-27 glm: took the task (the top free one in the lane; 272 waits on 271, 278 waits on 147). Plan: the town list comes from city.txt through the game-tree reader; one outdoor entry spot per town map; classic and 3D pairs at 22:00 without -Still, mean brightness per side into the README; finds by the card's third-of-brightness rule.
- 2026-11-27 glm: sweep done, 68 town maps (city.txt areas), classic and 3D at 22:00 at each map's spawn tile; raw pairs stay in build/walk-285/, the committed numbers and sheets in runs/285-towns-at-night/. Review round applied: every pair's sidecar game_time checked; ncr4's classic clock resets to 00:00 on every attempt (792000 -> 864003 twice), so its fix pair compares midnight on both sides; brightness re-measured without the HUD strip (the first pass included it on the classic side: sweep-before-the-fix.csv keeps those, sweep-hudfree-22.csv is the honest table).
- 2026-11-27 glm: FIX by a rule. The dark-ground heuristic (commonest floor tile darker than 40 and no roofs -> underground) called dark open-sky maps underground, and their 3D never dimmed at night: cardesrt 2.52x, raiders1 2.47x, ncr4 2.75x the classic's brightness at the same hour. The map script's own set_light_level literals are the engine's proof of the sky: a day level (>= 70) and a night level (<= 60) keep the map outdoor whatever the ground's colour. Change in vendor fp/scene.py (build_ground, plus script_light_levels extracted), exporter 1.13.0 -> 1.14.0, the fingerprint picks it up through the vendor hashes, 5 new unit tests in test_ground_sky_rule.py. Flip set: 37 map elevations by the probe (every dark desert, coast, mountain and random-encounter map; not one cave or vault); the trial export confirms ncr4/cardesrt/raiders1 and adds raiders1 e0 and sfchina2 e1/e2. After: cardesrt 1.45, raiders1 1.39, ncr4 1.53 (00:00) - the outliers are gone, what remains is find B below. Controls hold byte-stable: klatoxcv (interior) 17.52 = 17.52, kladwtwn (street) 29.38 = 29.38, and their scene flags are unchanged.
- 2026-11-27 glm: PINNED with causes. (B) Every outdoor town sits 1.2-1.5x brighter at night in 3D than in the classic (kladwtwn 1.44, klatrap 1.45, ncr1 1.51, tribe01 1.46, modmain 1.48 with its roof the worst): the classic dims the whole screen by the script's set_light_level(49) = 61.75 percent, the 3D night is its own sun model. Candidate renderer rule: apply the map script's night level as the 3D night ambient floor. Renderer lane, not cut here. (C) Dark-3D outliers: sfchina2 0.35 (the bunker room's lamps far weaker at night in 3D), depolva 0.43 / depolvb 0.49 (the Sierra surface's night, script night level 40). (D) ardead and vctycocl classic captures stall at the live-check watchdog step (30 s without progress) on four attempts each, including the walk's own tiles and the 2026-11-25 build; their 3D sides are measured, the classic sides stay missing rows.
- 2026-11-27 glm: gates. Exporter unit tests 182 OK (5 new), ctest 35/35, scoped fly tour (4 flip maps + 4 controls) in runs/285-towns-at-night/tour. Repro of the fix pair: tools/dev-snapshot.ps1 -Map raiders1 -Elevation 0 -ViewTile 23305 -Hour 22 -SceneCache <a cache exported by this branch>; the sweep ran on the shared build/scene-cache (exporter 1.13.0), the after side on build/cache-285trial (1.14.0 trial root, the shared cache untouched until merge). Next step: the tour's changed list lands in the README, PR 609 goes ready for the owner's eyes.
