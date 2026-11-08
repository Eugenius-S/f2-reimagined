---
id: 228
title: Front, back, left and right reference views of every landmark object we will model
milestone: M2
status: todo
owner: gpt
suggested: gpt
priority: low
tier: 4
claude-only: no
user-check: yes
depends-on: []
reviewed: no
---

## Goal

Every landmark object we will hand-model has four reference views generated
in GPT Image: front, back, left and right. The views are made to make
modeling easy, so a modeler builds the object from them without guessing.

## Context

- The roadmap (docs/2026-10-19-roadmap-design.md, M2): ordinary
  surroundings convert automatically, landmarks get hand-made models (GPT
  Image textures + modeling). 173 puts what cannot be built yet (landmarks,
  vehicles, large signs) on a plane; those are the candidates.
- Only `gpt` does this: image generation is gpt-only. The owner's split
  (2026-11-08): Terra writes and checks the prompts, Luna low generates the
  images. Lower priority than the rest of the board.
- The views come from the game's own sprite of the object as the reference:
  same shape, proportions, colours and wear, only seen from four sides.

## Acceptance

- A list of the landmark objects, one line each: name, the map(s) it stands
  on, its sprite (FRM name), and why it is a landmark. The list is in
  `runs/228-landmark-views/list.md`; the owner can strike lines.
- For each object, four views: `front.png`, `back.png`, `left.png`,
  `right.png` in `runs/228-landmark-views/<object>/`, each under 1 MB
  (the runs/ limit).
- The views are made for modeling: orthographic, no perspective, the same
  scale and ground line in all four, a plain flat background, even light
  without hard shadows, the whole object in frame, nothing else in it.
- The four views of one object agree with each other and with the game
  sprite: details on one side do not contradict another.
- The prompt for each image sits next to it in `prompts.md`, so a view can
  be regenerated. Terra checked each image against the sprite and the rules
  above; a rejected image is regenerated, not kept.

## How

1. Build the list from 173's planes and the city maps; cars, the vertibird,
   the tanker, large signs and unique buildings first.
2. Write one prompt template for the four views, try it on one object, show
   the owner, then run the rest.
3. One PR per batch of objects, so main gets them as they land.

## Out of scope

The 3D models themselves; textures; objects the automatic conversion
already builds well.

## Log

- 2026-11-08 claude: cut at the owner's request.
