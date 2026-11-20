---
id: 314
title: The launcher self-updates safely and removes only its own files
milestone: M1
status: doing
owner: gpt
suggested: gpt
priority: high
tier: 0
claude-only: no
user-check: yes
depends-on: [292, 307, 309]
reviewed: no
---

# Goal

With automatic updates enabled, the installed Windows launcher updates itself and the mod from the official GitHub release on a later start. A failed update remains recoverable without reinstalling through Steam. Full mod removal deletes every mod-owned file and launcher setting, while leaving vanilla Fallout 2 files, other mods and saves untouched. Launcher text, menu labels and entries have no unwanted rectangles or backplates.

# Context

Folded into the existing launcher recovery batch 292, 307 and 309 rather than duplicating their HTTP, rollback or per-game writer work. Existing drafts are not yet accepted, so reuse their implementations only after verifying interfaces and behavior. The user's recent failure was a Windows permission-denied replacement of `Fallout 2 Reimagined.exe`; do not touch the live Steam folder or launch the user's game during this task.

# Acceptance

- A previous published-like launcher build with auto-update on can obtain and stage the official update, replace itself safely, and start the updated version on the next launch; auto-update off does not replace it.
- Interrupted or failed update keeps the installation launchable or restores the previous complete version without Steam repair or reinstall; no partial files are presented as success.
- Full uninstall removes all positively identified mod-owned install files, update/recovery artifacts and launcher settings, including settings recreated on window close, without deleting vanilla files, other mods, saves, junction targets or arbitrary user files.
- Existing install/settings UI text and menu entries render without the reported rectangular backplates. Keep the current intended design; no broad UX redesign.
- Add or extend bounded Windows fixture tests for prior-build self-update, restart into the replacement, interruption/recovery, exact install/uninstall inventory, settings cleanup, and relevant backplate rendering if fixture capture supports it.
- Build and focused launcher tests pass. No interaction with the live Steam game folder, no release workflow, and no merge to main.

# Out of scope

- Steam, vanilla Fallout 2, other mods or save migration.
- Publishing or republishing any release, changing global installer UX, or replacing the launcher's visual design.
- Reimplementing the already-open 292, 307 or 309 draft work without first checking and integrating its exact behavior.

# Log

### 2026-11-20 gpt
Started: isolated worktree from origin/main at de6d5267; reviewed repository rules and confirmed no task 314 branch or PR exists. Existing drafts 292/307/309 cover adjacent recovery and locking work and are dependencies, not accepted behavior. Next: trace the Windows launcher path and add focused regression fixtures before changing implementation.
Check: focused launcher test command to be recorded with implementation.
