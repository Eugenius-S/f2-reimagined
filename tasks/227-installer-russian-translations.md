---
id: 227
title: The installer offers every major Russian translation, not only RPU
milestone: M1
status: todo
owner: any
suggested: gpt
priority: medium
tier: 1
claude-only: no
user-check: yes
depends-on: []
reviewed: no
---

## Goal

A Russian player picks the translation they grew up with in the installer:
1C (with Russian speech), Fargus, Levy Corporation (it also replaces part of
the sound and the music), or the RPU translation the installer has today.

## Context

199 added one Russian option: the launcher fetches the RPU translation from
its official release at the player's request. The other translations are
not on GitHub and must never be committed or attached to our releases
(the owner, 2026-11-08): the launcher downloads them at install time from a
Google Drive the owner hosts or from the sites that publish them.

## Acceptance

- The installer's Russian option becomes a choice of four: RPU (today's),
  1C with speech, Fargus, Levy Corporation. The UI says in one line what
  each one changes (text, speech, sound, music).
- Each is downloaded from its URL at the player's request, checked by size
  and SHA-256 before use, and installed so the game loads it (dat files and
  `mods_order.txt`, or what that translation needs). A failed or wrong
  download leaves the game as it was.
- Switching between translations, or back to English, leaves no file of the
  previous one in effect.
- Nothing of these translations is in the repo, its history or a release.
- One launch check per translation on the demo copy: the main menu and one
  dialogue are in Russian; for 1C one spoken line; for Levy Corporation its
  music plays.
- Both READMEs list the four options.

## How

1. Find each translation's archive and what is in it (text, fonts, speech,
   sound, music; dat or loose files), and how it must load with our engine.
   Write the findings in the Log.
2. Ask the owner (in the PR or the task Log) to put the archives on their
   Google Drive, or name a stable public mirror. The URLs and hashes live in
   the launcher's code, not in the repo as files.
3. Extend the 199 Russian option in the launcher with the choice and the
   download, check and install.

## Out of scope

Rehosting on GitHub; translations of the Restoration Project; editing a
translation's content.

## Log

- 2026-11-08 claude: cut at the owner's request.
