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

## 2026-11-23 Windows evidence

Published with the first integration. The isolated settings fixture saved 1600x1200 and retained 2x. A hidden run does not prove physical resize, monitor mode changes or pointer alignment on a real display.

[Native acceptance checks](../runs/325-night-checks/README.md).

Follow-up: display-only Done now triggers the live resize. Native Windows isolated proof measured the actual window and drawable transition to 1600x1200, retained the logical canvas and OpenGL, and mapped one physical point to the same ground hex. The black band does not capture the point. [Evidence](../runs/325-live-resize/README.md). Visible display mode tests are separate.
