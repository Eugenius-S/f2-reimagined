---
id: 321
title: Enable vertical sync for player builds
milestone: M1
status: blocked
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


### 2026-11-26 gpt: isolated menu text check

On Windows Release `5e8738eab` (executable SHA-256 `c1cc0353f123be55b82edd207265bf9315e51f59692dcbc5a5f990aa68334ad7`), the actual off-screen options panel showed readable [English](../runs/321-vsync-native/display-en.png) and [Russian](../runs/321-vsync-native/display-ru.png) VSync labels and restart notices at 1920x1080, 2x. The resolution dropdown obscured the toggle value; these crops prove the text only, not toggling, cancel, or restart. The Russian run used copies of translated text and fonts from the existing isolated `f2r-227-translation-packs/work/steam1c/app/data` donor, with its `text/english` copied to the scratch `text/russian`; it did not touch a player install.

Check: `powershell -NoProfile -ExecutionPolicy Bypass -File tools/dev-snapshot.ps1 -Preferences -PreferencesDropdown resolution -Language <english|russian> -GameRoot build/isolated-game -BuildExecutable build/test-bin/fallout2-ce.exe -TestDirectory build/test-run-321 -SceneCache build/scene-cache -OutputDirectory build/evidence-321-display-<en|ru> -CaptureConsole -TimeoutSeconds 90`. The executable and base DAT files were copied from `_archive/worktrees/f2r-317-native-acceptance`; an empty private scene cache was used. Snapshot metadata identifies the same executable commit and a window at (-20000,-20000), not fullscreen. `py -3 -m unittest tools.test_vsync_settings -v` passed 4/4 source checks.

The previously recorded native Done/restart and OpenGL interval proof remains separate, as it did not identify this exact executable. Cancel and a visible control state were not established by this capture. Physical panning/tearing, two display modes and unsupported-driver startup still require a safe interactive monitor/driver session or a suitable isolated fixture. Status stays blocked; do not infer physical synchronization from the hidden window.
