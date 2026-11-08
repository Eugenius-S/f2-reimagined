---
id: 227
title: The installer offers every major Russian translation, not only RPU
milestone: M1
status: todo
owner: any
suggested: gpt
priority: high
tier: 0
claude-only: no
user-check: yes
depends-on: [186]
reviewed: no
---

## Goal

A Russian player picks the translation they grew up with in the installer:
1C and Fargus (both with Russian speech), Levy Corporation (it also replaces part of
the sound and the music), or the RPU translation the installer has today.

## Context

199 added one Russian option: the launcher fetches the RPU translation from
its official release at the player's request. The other translations are
not on GitHub and must never be committed or attached to our releases
(the owner, 2026-11-08): the launcher downloads them at install time from a
Google Drive the owner hosts or from the sites that publish them.

## Acceptance

- The installer's Russian option becomes two independent choices: text
  from one translation (RPU as today, 1C, Fargus, Levy Corporation), and
  sound from one (original English, 1C speech, Fargus speech, Levy
  Corporation's sound and music, or what else a translation carries). Any text goes with any sound,
  e.g. RPU text with 1C speech. The UI says in one line what each choice
  changes.
- Each is downloaded from its URL at the player's request, checked by size
  and SHA-256 before use, and installed so the game loads it (dat files and
  `mods_order.txt`, or what that translation needs). A failed or wrong
  download leaves the game as it was.
- Switching text or sound, or back to English, leaves no file of the
  previous choice in effect. If a text and a sound do not fit together
  (speech lines that do not match the dialogue), the Log says so and the
  installer warns.
- Nothing of these translations is in the repo, its history or a release.
- One launch check per translation on the demo copy: the main menu and one
  dialogue are in Russian; for 1C and Fargus one spoken line; for Levy
  Corporation its music plays.
- One mixed check: RPU text with 1C speech.
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

Rehosting on GitHub; translations of the Restoration Project; editing a
translation's content.

## Log

- 2026-11-08 claude: cut at the owner's request.
- 2026-11-08 claude: goes right after 186 (the owner): depends on it, priority high.
