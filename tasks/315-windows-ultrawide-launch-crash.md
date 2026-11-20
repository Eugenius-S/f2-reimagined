---
id: 315
title: Fallout 2 starts at 3440 x 1440 after a clean Windows install
milestone: M1
status: todo
owner: any
priority: high
tier: 0
claude-only: no
user-check: no
depends-on: []
reviewed: no
---

## Goal

The game reaches a usable main menu after the player chooses 3440 x 1440 in the Windows launcher on a clean install.

## Context

- The owner reports that after a clean Windows install, choosing 3440 x 1440 in the launcher and launching the game causes it to exit or crash before it is playable. Neither the exit type nor the cause is established.
- This is a startup blocker in the installer/launch batch. Track it separately from 314, which covers launcher self-update, uninstall cleanup and UI presentation. It does not wait for 314.

## Acceptance

- Reproduce the reported sequence with a clean disposable Windows installation, recording the tested build, chosen mode and startup outcome.
- Starting with 3440 x 1440 selected reaches a usable main menu with that supported mode applied, including on the next launch.
- Record an automated regression where practical and keep a standard-resolution startup check. Do not use or change the owner's live installation.

## Out of scope

Launcher self-update, full uninstall and visual cleanup from 314.

## Log

- 2026-11-20 gpt: recorded the owner's report as an urgent tier 0 startup blocker. No diagnosis or implementation started.
