---
id: 247
title: Switch the bottom HUD between Original and CE
milestone: M1
status: doing
owner: gpt
suggested: any
priority: medium
tier: 2
claude-only: no
user-check: no
depends-on: []
reviewed: no
---

## Goal

Offer Original and CE HUD side panels in game preferences. Preserve the
existing CE appearance and player layout settings by default.

## Context

- The owner chose the standard game-mounted HR_IFACELFT1/HR_IFACERHT1
  metallic pair; do not add game art to the repository.
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
