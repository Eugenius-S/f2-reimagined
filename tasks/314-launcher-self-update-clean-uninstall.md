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

### 2026-11-20 gpt checkpoint
Draft #545 now publishes launcher hash metadata with the package, detects a stale installed launcher even with the same game version, stages replacement beside the destination, and refuses externally locked launchers before touching the game build. It reports deferred launcher removal as pending and handles a locked orphan `.old` on retry. Matching settings are removed without deleting an INI configured for another game root. No live installation or release was changed.

Windows checks: 26/26 install tests, including a SHA-verified copy of published `Install-F2R.exe` migrating through the console to the fixture build and starting its replacement; 23/23 console RP tests. A separate unmerged trial tree `48fd4726d686e3cb29d81c3e013bc58f73f4ed79` combines this work with #309 at `215cf0da` (which includes #307 at `8e8ea2df`), preserves recovery and concurrency gates, and passes 9/9 non-GUI launcher CTest targets plus 23/23 selected console RP cases.

Still open before release:

- Review and integrate #307 and #309. Make `launcher_update.h::copyLauncherFile` reuse the staged launcher replacement contract instead of falling back to copying over a locked executable. Prove recovery of a post-replacement interruption while the prior launcher image is live, and repeat with a real denied handle and a later retry.
- Replace broad cleanup of `user-assets/fullscreen-20261028` with positively identified file ownership. Use exact installed paths and digests from a validated package inventory and record whether each path was introduced or replaced, saving originals for replaced paths. For older installs without a receipt, use only demonstrated package identities; retain unmatched, user-modified or unprovable files. Refuse traversal through reparse points and remove only empty parents. An unbound legacy AppData INI also needs ownership proof before deletion. Test mixed files inside the same asset subtree, not just a different sibling folder.
- Verify worker/GUI behavior without disturbing the live game setup: auto-update on and off, stale asynchronous release replies after changing roots, and confirmed delayed cleanup after the GUI and helper exit. The published migration test exercised console mode, not the original GUI sharing-denial scenario. Keep the replacement bootstrap path documented for old launchers that cannot update themselves.
- Confirm the exact reported UI backplate states before touching rendering. No visual acceptance or design change has been made.
