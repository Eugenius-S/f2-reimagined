# Per-frame image generation prompts

Use one request per frame (14 total), always attach that frame's `source/*-original.png` as the image reference. For a request, concatenate the shared prompt below and that frame's pose line. Do not attach a sheet. The reference frame is the exact blockout for its pose, silhouette, pixel footprint, proportions, colors, and light direction.

## Shared prompt

Create a realistic HD redraw of the exact adult male character shown in the reference: Fallout 2's default male `hmjmps` character in the blue Vault jumpsuit. Keep the same recognizable person, face, hair, skin tone, body build, outfit construction, pose, viewing direction, silhouette, and light direction as the reference. Treat the reference as a strict colored clay blockout: preserve the exact pose, outer contour, limb placement, frame proportions, relative sizes, color regions, and transparent canvas dimensions. Add believable material detail only inside the existing opaque shape. The fitted blue suit is thick, slightly glossy synthetic cloth with stitched padded panels, ribbed seams, and narrow yellow trim, like the material language of a Fallout 4 vault suit; do not copy any Fallout 4 image or redesign the suit. Preserve the source's yellow Vault 13 mark in its existing location and shape. Render natural skin, short dark hair, subtle cloth folds, and grounded highlights/shadows consistent with the source light. Keep the same character design and facial features as every other frame in this set. Keep the background transparent. Add nothing outside the source alpha silhouette and do not change the canvas size, crop, pose, expression, lighting direction, costume details, or framing. No props, text, borders, new marks, dramatic lighting, extra fingers, blur, or painterly background.

## Per-frame pose lines

| # | Frame | Request-specific line |
|---:|---|---|
| 01 | `stand-dir0` | Standing idle, direction 0 (NE), three-quarter front view. |
| 02 | `stand-dir1` | Standing idle, direction 1 (E), side-facing view. |
| 03 | `stand-dir2` | Standing idle, direction 2 (SE), three-quarter view from the opposite front angle. |
| 04 | `stand-dir3` | Standing idle, direction 3 (SW), rear three-quarter view. |
| 05 | `stand-dir4` | Standing idle, direction 4 (W), side-facing view from the opposite side. |
| 06 | `stand-dir5` | Standing idle, direction 5 (NW), rear three-quarter view. |
| 07 | `walk-dir0-frame00` | Unarmed walking cycle, direction 0 (NE), frame 0: first distinct stride pose. |
| 08 | `walk-dir0-frame01` | Unarmed walking cycle, direction 0 (NE), frame 1: continue the exact source limb positions. |
| 09 | `walk-dir0-frame02` | Unarmed walking cycle, direction 0 (NE), frame 2: continue the exact source limb positions. |
| 10 | `walk-dir0-frame03` | Unarmed walking cycle, direction 0 (NE), frame 3: continue the exact source limb positions. |
| 11 | `walk-dir0-frame04` | Unarmed walking cycle, direction 0 (NE), frame 4: continue the exact source limb positions. |
| 12 | `walk-dir0-frame05` | Unarmed walking cycle, direction 0 (NE), frame 5: continue the exact source limb positions. |
| 13 | `walk-dir0-frame06` | Unarmed walking cycle, direction 0 (NE), frame 6: continue the exact source limb positions. |
| 14 | `walk-dir0-frame07` | Unarmed walking cycle, direction 0 (NE), frame 7: final source stride pose before the loop returns to frame 0. |

Every row is a separate generation request with the shared prompt, the row's pose line, and its matching original PNG. Record each request start/end time and acceptance or regeneration in the task Log. No generated frame has been accepted yet.
