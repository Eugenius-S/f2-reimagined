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

## Log

### 2026-11-26 gpt

The 200 ms eased dialogue backdrop is implemented for the local Full 3D
composite, below the interface. The timer has a standalone test including
reversal and tick wrap. This is a draft: the complete runtime acceptance is
not yet met.

On Windows, a temporary fixture opened the real Maida dialogue (KCMaida,
KLADWTWN elevation 0, speaker tile 22678, hero 22676, view 22677) at the
parent 10925abd, 1920x1080 and 3440x1440, scale 2. Presented frames showed
an already black TV opening, so the reported TV defect did not reproduce
there and its code was left unchanged. The surroundings were not black.
An instrumented candidate showed a smooth darkening on entry in a 16:9
presented-frame readback; the standalone timer test passed with MSVC.

An instrumented build of the candidate then selected Maida's Goodbye option
through the normal dialogue input. Both formats showed a fade in and out in
the actual composed framebuffer. The logs in `runs/332-dialogue-tv-backdrop/`
record ticks and RGB sums for an 8x8 scene area and a panel area before swap;
on entry the panel sample stays constant. The 16:9 and 21:9 before/after
sheets show the presented frame; the Sulik sheet shows a talking head with
the backdrop. Sulik's Goodbye option also reached the fade-out completion.
The extra diagnostic capture and input were temporary, not part of the PR.

CI Verify 45667652641 passed for code commit 28245c10, including Windows
Release and CTest 35/35. Repeat opening, barter and classic-view runtime
comparisons remain open. Two earlier temporary exit probes using Escape and
_dialogQuit did not close this dialogue; neither established exit behavior.
