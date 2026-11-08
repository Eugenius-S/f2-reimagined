---
id: 229
title: A second HD sprite trial, realistic, every frame generated on its own
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

One more go at redrawing a character's sprites with an image model, after
197: more realism in the prompt, and each frame generated on its own instead
of all frames on one sheet. A trial; nothing goes into the game.

## Context

- 197 (runs/197-hd-sprite-trial/) redrew the default jumpsuit (`hmjmps`)
  as two sheets of 14 frames each. The owner saw it (2026-11-08): everything
  on one sheet, not realistic enough. Its findings: the look drifted and the
  bottom anchors shifted by up to 7 px between frames.
- Same character and frames as 197, so the two compare directly: the
  standing frame in all 6 directions and the 8 unarmed walk frames in
  direction 0.
- Only `gpt`: image generation is gpt-only. Terra writes and checks the
  prompts, Luna low generates the images (the owner, 2026-11-08). Lower
  priority than the rest of the board.

## Acceptance

In `runs/229-hd-sprite-trial-2/`, never in game data, each file under 1 MB:

- One image request per frame (14 requests), each given its original frame
  as the reference. The prompt asks for a realistic look (real materials,
  fabric folds, skin, believable light from the original's direction) while
  keeping the pose, silhouette, proportions and colours of the frame.
- Each output is cut to its frame's original size times x2 and x4 and
  aligned to the original frame offsets; the Log gives the anchor shift per
  frame, as 197's `metrics.json` did.
- A comparison sheet: original, 197's result, this result, at x1, x2, x4.
- The walk cycle as a looping GIF next to the original's, to judge flicker.
- The prompts in `prompts.md`, and time per frame in the Log.
- Terra rejected and regenerated frames that broke the pose or drifted from
  the others; the Log says how many.

## How

1. Write the per-frame prompt from 197's prompts and findings; try it on
   two frames and check they match each other before running all 14.
2. Generate, cut, align, build the sheet and the GIF.

## Out of scope

Putting any of it into the game; other characters; the full animation set.

## Log

- 2026-11-08 claude: cut at the owner's request.
