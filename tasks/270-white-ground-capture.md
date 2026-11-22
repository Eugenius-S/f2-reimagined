---
id: 270
title: A 3D capture never comes out with white ground
milestone: M1
status: done
owner: gpt
suggested: gpt
priority: normal
tier: 3
claude-only: no
user-check: no
depends-on: [306]
reviewed: yes
---

## Goal

Find why `tools/dev-snapshot.ps1` on KLAMALL e0 at view tile 28096 gave a 3D frame with white ground (256, PR #455), and fix the cause or make the run fail.

## Context

- 2026-11-15: PR #504 isolates the white floor to the bake-enabled renderer path. Task 306 supplies the renderer fix excluded below; resume this evidence check after it lands.

Both 3D attempts in 256 (`-Still -Shadows`, then `-NoPreview`) show white ground where the floor should be. Suspects: a stale or half-built scene cache for the map, a missing `tiles.png`, the capture taken before the floor texture is uploaded.

## Acceptance

- The cause is named in the Log with the evidence.
- The same command gives a frame with the floor, saved in `runs/270-white-ground-capture/`; or, if the cache is at fault, the harness rebuilds it or stops with a message.
- With that frame, say in 256's Log whether the notched shadow edge shows.

## Out of scope

Renderer and map data changes.

## Log

- 2026-11-13 claude: cut from the review of gpt's run of 2026-11-12: the blocked evidence tasks share a few harness gaps.

- 2026-11-23 gpt: PR #573 restored the atlas and draw framebuffer in the
  bake-target allocator. On exact release `7667d33e`, KLAMALL e0 at view
  28096, hour 8 and zoom 1 rendered a textured floor with the normal
  bake enabled. The exact release had 0 near-white samples in 13,502
  samples. The diagnostic baseline had about 60% near-white in the same
  sampled view. The [paired native frames](../runs/306-ground-bake-current/before-after.jpg)
  record the restored floor. The old notched vine-wall report belongs to
  KLADWTWN, a separate map at the same view tile; 256 records that check.
