---
id: 324
title: Apply physical display resolution from preferences
milestone: M1
status: doing
owner: gpt
priority: normal
tier: 2
claude-only: no
user-check: no
depends-on: []
reviewed: no
---

From overnight queue #317. Resolution changes apply on closing the preferences when windowed or exclusive fullscreen. Borderless remains desktop-sized. Keep the current logical canvas and camera in place to avoid restarting local 3D. The interface scale still needs a restart.

Windows acceptance: toggle 1920x1080 and 3440x1440 in windowed or exclusive fullscreen, verify actual output, mouse alignment, local 3D and global 2D, and restart persistence. Test Cancel, unsupported exclusive modes, and borderless. Source routing tests pass; physical display checks are pending.
