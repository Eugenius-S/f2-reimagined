---
id: 227
title: The installer offers every major Russian translation, not only RPU
milestone: M1
status: doing
owner: gpt
suggested: claude
priority: high
tier: 0
claude-only: no
user-check: yes
depends-on: [186]
reviewed: no
---

## Goal

A Russian player picks the translation they grew up with in the installer:
1C, Fargus, Levy Corporation, or the official RPU text. Text and audio are
selected separately. The original audio is available with any supported text.

## Context

199 added one Russian option: the launcher fetches the RPU translation from
its official release at the player's request. The other translations are
not on GitHub and must never be committed or attached to our releases
(the owner, 2026-11-08): the launcher downloads them at install time from a
Google Drive the owner hosts or from the sites that publish them.

## Acceptance

- The installer has two dropdowns, Text and Voice. Both show only packages
  verified for the current choice of base game or RP. A player can keep the
  original audio or choose an available track independently. Label matching
  tracks and verified alternatives clearly; never replace a voice silently.
- Normalize all text to the project's CP1251 layout and preserve the built-in
  IBM Plex Mono sharp-text renderer. The classic UI uses one shared Russian
  font set matching the existing official Russian install. Individual
  translations must not ship their source fonts or override the shared set.
- Include the supplied RP-specific archives in the inventory and test their
  language assets against the RPU release the launcher uses. A package for
  older RP is not selectable for the current release until verified there.
  Keep the official RPU text option available under its existing name.
- Each is downloaded from its URL at the player's request, checked by size
  and SHA-256 before use, and installed so the game loads it (dat files and
  `mods_order.txt`, or what that translation needs). A failed or wrong
  download leaves the game as it was.
- Switching text or sound, or back to English, leaves no file of the
  previous choice in effect. If a text and a sound do not fit together
  (speech lines that do not match the dialogue), the Log says so and the
  installer warns.
- Nothing of these translations is in the repo, its history or a release.
- On disposable base-game and RP copies, verify the main menu and a dialogue
  for every offered text package. For every offered voice package, verify a
  spoken line and the matching lip sync. Verify a mixed choice where a
  compatible voice is available, and verify that switching back to original
  audio drops the installed voice files without removing the text.
- Both READMEs list the text and sound options.

## How

1. Find each translation's archive and what is in it (text, fonts, speech,
   sound, music; dat or loose files), and how it must load with our engine.
   Write the findings in the Log.
2. Ask the owner (in the PR or the task Log) to put the archives on their
   Google Drive, or name a stable public mirror. The URLs and hashes live in
   the launcher's code, not in the repo as files.
3. Split each archive into its text part and its sound part, so each
   installs on its own.
4. Extend the 199 Russian option in the launcher with the two choices and
   the download, check and install.

## Out of scope

Rehosting on GitHub, modifying a translation's wording, or shipping game
binaries, scripts or settings from third-party installers as language assets.

## Log

- 2026-11-22 gpt: launcher implementation adds separate text and voice
  selectors, verified downloads, transaction recovery and RP filtering.
  The four v4 DATs remain on the owner's Drive, not in git. Local tests cover
  the base/RP round trip, a locked-config rollback, concurrent workers, both
  UI languages and foreign same-name files. Independent review and final
  acceptance remain pending.
- 2026-11-22 gpt: the owner supplied the archives and asked for separate text
  and voice selectors, with supported choices filtered by RP. Work started
  in an isolated branch from `ccd8c0b7`; outputs will await the owner's Drive
  upload and tested download links before they become available to players.
- 2026-11-22 gpt: the official RPU v2.4.34 Russian DAT matches the existing
  project reference byte for byte (SHA-256
  `f32d9d0b2696a5066a05a13e431c8dfc9d0772c058932121209049c374d8a8c7`).
  Its classic font resources provide the common CP1251 font set. The former
  RP 2.3.3 Levy overlay, before filtering, lacks 367 current message IDs
  across 23 shared MSG files. Installing it as-is would discard those IDs.
- 2026-11-22 gpt: independent Sol review accepted local preparation v4.
  The common font DAT keeps the 10 official font resources at their root
  names. Variant text DATs contain no fonts. On current build `ccd8c0b7`,
  the three vanilla text variants reached the menu and passed the Klamath
  Cassidy dialogue fixture with the built-in sharp text active. Six tests
  passed. Local upload set and proof are outside git. The old RP Levy overlay
  still lacks 363 IDs in 22 files after filtering. Its `elder/aeld1` replacement
  audio is shorter than the inherited lip timing. Old RP and voice candidates
  stay out of the active catalog. Drive upload and installer wiring are pending.
- 2026-11-22 gpt: the owner uploaded the four v4 text/font DATs. All four
  Drive links returned the exact pinned bytes without signing in. The Windows
  launcher now shows Text and Voice separately, filters choices by the selected
  game root and RP, and applies the locale through a recoverable transaction.
  Accepted Release launcher SHA-256 `5cff17f5735b94bc94634ffeea9fcd597156f66438b500e6745aec4a72d09d55`.
  Launcher CTest 10/10 passed. Independently verified the bilingual GUI,
  typed-root and recovery paths, live Drive switching across all three
  translations, rollback with a locked cfg and rejection of a same-size file
  with the wrong SHA-256. Fargus installed from Drive and rendered in Cassidy's
  dialogue on build `ccd8c0b7` with IBM Plex Mono active.
  Sol accepted the final CLI guard for pending or malformed RP state.
- 2026-11-08 claude: cut at the owner's request.
- 2026-11-08 claude: goes right after 186 (the owner): depends on it, priority high.
- 2026-11-08 claude: owner order 2026-11-08: claude takes critical bugs, performance, RP and the Russian translations.
- 2026-11-11 claude (audit, folded in): the launcher takes a cut-off download for a whole one. `ce/src/launcher_main.cc:451` sets `ok = total > 0`; the Content-Length read at line 433 is never compared. `rpu_russian.dat` comes from the mirror as a bare file, so no unpack step catches a stump, and the next run keeps it (`kept the existing`, line 990). Fix here: a download succeeds only when the length is unknown or equals the bytes received; one case in `launcher_routes_test`.
- 2026-11-12 claude: the audit find above is fixed (#473): a download succeeds only when the server named no length or every byte came, and a failed one removes its stump. Checked: ctest 27/27 (a case in `launcher_routes_test`); the launcher against a local server that names 5 MB and sends 2: "the download was cut short", nothing installed, no work folder left. The translations themselves still wait for step 2 (the owner's archives or a mirror).

- 2026-11-22 gpt: added independent text and voice switching, including
  rollback across RP changes and reversible handling of a manually installed
  BGforge sound pack. Voice compatibility for RP is checked against the
  tested `rpu.dat` from v2.4.34. Windows Release CTest 33/33 passed. A local
  HTTP run installed the exact 1C and Levy voice packages and exercised
  independent changes and a return to original audio. Four silent in-game
  proof runs started Sulik speech and lip sync with Fargus text in base and
  official Russian text in RPU, each with both available voice packs.
- The two voice packages are ready outside git. The 1C recording matches
  the BGforge Fallout2_Sound v1.2 track. The Levy DAT contains 706 playable
  ACM recordings and 629 lip tracks. Twenty unreadable old Hakunin files
  fall back to the game. The owner accepts Fargus text with another voice;
  Fargus voice remains unavailable.
- 2026-11-22 gpt: configured the 1C and LeftCorp voice package metadata for
  base and RP from the owner's Drive links. Publication and public download
  verification remain pending; owner visual check follows the merge.
