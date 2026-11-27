---
id: 247
title: Switch the bottom HUD between Original and CE
milestone: M1
status: done
owner: gpt
suggested: any
priority: medium
tier: 2
claude-only: no
user-check: no
depends-on: []
reviewed: yes
---

## Goal

Offer Original and CE HUD side panels in game preferences. Preserve the
existing CE appearance and player layout settings by default.

## Context

- The standard game-mounted HR_IFACELFT1/HR_IFACERHT1 metallic pair matches
  the owner's reference. Do not add game art to the repository.
- The center, buttons, layout and existing CE side art configuration remain
  unchanged. Missing Original art falls back to CE with an explicit menu label.
- The former RP/Original proposal is superseded; task 268 is separate.

## Acceptance

- Original and CE are selectable in English and Russian, in Classic and 3D;
  Save, Cancel, defaults and restart work.
- Both sides show the full logos at ordinary and wide widths; compact mode,
  user side selection and orientation, hitboxes and states remain intact.
- Missing art has an honest CE fallback. Paired screenshots show both styles,
  both views and widths; Windows build, tests and independent review pass.

## Out of scope

Bundled game art, changes to the owner's installation and saves, and release.

## Log

- 2026-11-10 claude: cut from the owner's ask in chat.

- 2026-11-27 gpt: implementation candidate. Added a persisted HUD style,
  mounted-art pair detection and in-game side repaint; the original pair is
  cropped toward the center and fitted for narrow sides. Existing CE layout
  settings and center are retained. Next: Windows build, real Preferences
  Save/Cancel/restart, absent-art fallback, Classic and 3D paired screenshots,
  normal and wide layouts, clicks and independent review. Check: static diff
  only so far; runtime and visual acceptance remain open.

- 2026-11-27 gpt: review found the narrow Original logo was horizontally
  squashed. The candidate now scales the logo band in both axes and fills the
  remaining height from its outer bands; very wide sides extend the outermost
  art without enlarging the emblem. The CE rendering path stays unchanged.
  Check: a small arithmetic check covered both sides, widths 1..2000 at
  representative boundaries, heights 1..200, source bounds, the final 80px
  column and pixel-identical 99px-high crops when the logo fits. This is not
  an image or game check; Windows build and paired runtime views remain open.

### 2026-11-27 gpt

Done: The Original / CE selector saves and applies in game. The metallic
pair shows both emblems in Classic and 3D at 1920x1080 and 3440x1440. In all
four matched checks CE equals main, and the center strip equals CE in Original.
Next: No further work on this card. This PR does not publish a release.
Check: Windows Release `962f829ef`, CTest 35/35 and independent Terra review.
In English Classic and Russian 3D, used the Preferences menu to save both
styles, restarted into each, canceled a change and restored defaults. Checked
the inventory button press, decorative side click and the stock CE fallback.
[Paired game frames and raw capture metadata](../runs/247-hud-bar-style-switch/README.md).
