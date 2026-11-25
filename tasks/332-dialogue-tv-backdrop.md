---
id: 332
title: Dialogue has a black TV opening and a smoothly fading backdrop
milestone: M1
status: doing
owner: gpt
suggested: gpt
priority: high
tier: 3
claude-only: no
user-check: yes
depends-on: []
reviewed: no
---

## Goal

When a local 3D dialogue opens, ordinary NPC conversations display a black TV opening. The black backdrop around the panel fades in and out smoothly over the live scene.

## Context

Dialogue-only part of task 193, following the owner's November 24 report. Preserve real talking heads, the dialogue panel, reply text and barter. Leave the classic view and the other full-screen screens alone.

## Acceptance

- Reproduce the old dialogue on Windows. Show the same conversation before and after at 16:9 and 21:9. An ordinary local 3D conversation has a black TV interior, and an actual talking head remains visible.
- Capture intermediate presented frames on opening and closing. A roughly 150 to 250 ms fade darkens the live scene behind the panel, leaves the UI and cursor readable, and completely clears on close without a black flash.
- Local 3D stays active on both sides. No blocking delay, lost input, changed classic scene or state left behind when the dialogue exits. Check repeat open/close and barter if the fixture supports it.
