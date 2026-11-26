---
id: 321
title: Enable vertical sync for player builds
milestone: M1
status: doing
owner: gpt
priority: high
tier: 1
claude-only: no
user-check: no
depends-on: []
reviewed: no
---

From the overnight queue #317: add a display preference for frame synchronization. Default on for player settings on both OpenGL and SDL fallback; unsupported sync cannot prevent startup. The isolated performance fixture stays uncapped.

Windows acceptance: verify Screen UI in English/Russian, toggling and persistence across restart, and verify active synchronization on a physical display while panning. Compare two display modes and both supported/unsupported renderer initialization. Local source and config checks pass; physical display verification is pending.

## 2026-11-23 Windows evidence

Published with the first integration. The setting is routed to both video paths and the Windows builds pass. Physical synchronization with the monitor, toggling on a visible display, and the full unsupported-driver matrix remain unverified.

[Native acceptance checks](../runs/325-night-checks/README.md).

The Windows Display panel saved VSync using its real key and Done paths. The isolated restarted build kept the setting and the native OpenGL driver returned interval 1 instead of 0. This does not certify physical monitor behavior, fullscreen, or unsupported hardware.

### 2026-11-23 release reconciliation

The setting shipped in `039d25da3`, and the native restart test confirmed OpenGL interval 1. Remaining: physical-display tearing while panning, exclusive and borderless checks, Russian UI and unsupported driver fallback.
### 2026-11-26 gpt

Check the remaining English and Russian menu paths and note the safe limits of off-screen display-driver verification.
