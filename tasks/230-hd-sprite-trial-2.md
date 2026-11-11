---
id: 230
title: A second HD sprite trial, realistic, every frame generated on its own
milestone: M2
status: blocked
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

In `runs/230-hd-sprite-trial-2/`, never in game data, each file under 1 MB:

- One image request per frame (14 requests), each given its original frame
  as the reference. The F2 frame is the base, used like a coloured clay
  blockout: it fixes the shape, pose, silhouette, proportions and where
  each colour goes, and the prompt adds all the realism on top (real
  materials, fabric folds, skin, believable light from the original's
  direction). Nothing outside the F2 shape is invented.
- The frames are consistent with each other: the same face, build,
  suit details and light in all 14, so the walk does not flicker. Terra,
  the main agent, checks every new frame against the ones already accepted
  and regenerates any that drifts.
- The vault jumpsuit's fabric reads like the Fallout 4 vault suit (the
  owner, 2026-11-08): a fitted blue suit of thick, slightly glossy
  synthetic cloth with stitched, padded panels, ribbed seams and yellow
  trim. The prompt describes the material in these words; it copies no
  Fallout 4 image.
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

### 2026-11-11 gpt
Tried: two individual image-generation requests for `stand-dir0`, each with its original frame as the reference and the prepared realistic-material prompt. The controller stopped the trial after these two failures.
Saw: attempt 1 turned the character around and spread the legs widely. Attempt 2 changed the foot positions and the silhouette proportions. Neither met the pose and shape constraints.
Next: blocked after two failed attempts. No other frames were generated or accepted. Resumption needs a generation setup that preserves the reference pose, foot placement, silhouette, and frame proportions.
Check: controller visual review reported both failures. Controller wait times were about 10 s for attempt 1 and 19 s for attempt 2; these are approximate waits, not exact generation timings.

### 2026-11-12 gpt
Tried: eight more individual `stand-dir0` generations, each using the source frame and a revised description. Together with the two attempts logged above, the controller reviewed 10/10 attempts for this frame.
Saw: all eight new images were rejected. They changed the turn, stance or foot placement, proportions, and silhouette; some also failed to look photorealistic. None was accepted, and there is no agreed set. The eight rejected generated images were copied to ignored `build/task230/rejected/`; originals remain in the generated-images folder. No game files were changed.
Next: blocked. No more frames or image requests until a setup can preserve the source pose, feet, proportions, silhouette, and realism.
Check: controller visual review of all eight new images. Copy verification: eight files in `build/task230/rejected/`; directory is ignored by Git. Generation timings and image-size/alignment metrics were not measured. No accepted frame, comparison sheet, or GIF exists.
