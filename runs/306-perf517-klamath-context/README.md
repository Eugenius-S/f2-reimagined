# 306 / PR 517: why Klamath came out slower in the paired perf runs

This is a read-only analysis of the saved files of the independent paired run in the review archive `night-2026-11-17-sol-rereview-5/perf517`. Nothing was run again: no game, no build, no perf. Head 517 (`0ca0d771`) is unchanged.

**Verdict: unresolved. Neither clean nor pass.** The saved files prove that the 306 code did not run in these measurements. So the drop does not come from what 306 executes. They do not prove what caused it. Only a new paired run can settle that, and the plan for it is at the end.

## The measured drop

`tools/perf-check.ps1`, one run per side. The baseline ran first, from 19:32:03 to 19:34:19 UTC. The candidate ran right after, from 19:34:19 to 19:36:36.

| map | avg fps base | avg fps cand | change | 1% low base | 1% low cand | change |
|---|---|---|---|---|---|---|
| kladwtwn e0 | 167.4 | 137.1 | -18.1% | 103.9 | 83.4 | -19.7% |
| newr1 e0 | 158.6 | 140.5 | -11.4% | 90.6 | 97.2 | +7.3% |
| vctyctyd e0 | 935.5 | 897.6 | -4.1% | 613.5 | 597.7 | -2.6% |
| klatoxcv e2 | 288.0 | 287.6 | -0.1% | 194.7 | 190.0 | -2.4% |

Both sides pass the absolute thresholds (1% low >= 60 fps, load <= 2000 ms). That does not make the pair acceptable.

## Proven facts

1. **Source.** The baseline exe reports `ce_commit c1b9afef`, the candidate reports `0ca0d771`. Under `ce/`, the only commit between them is `5b0312e9` (306, `scene_draw.cc`, +11 -1). `ce/src` on main `de6d5267` is the same as in `c1b9afef`.
2. **Toolchain.** Both exes have linker 14.40 and the same Rich-header compiler build ids (30795, 33731, 33821). `.text` differs by 80 bytes and `.rdata` by 1024 bytes. The candidate is a fresh Ninja Release build (`/O2 /Ob2`, MSVC 14.40.33807). The baseline is the `f2r-verified` exe built on 2026-11-15. Its build command is not recorded in the archive.
3. **Context.** These are the same on both sides:
   - the perf script (sha `83804627...`, commit `d52bd3f6`);
   - `tools/test-config/fallout2.cfg` (sha `45b1c45a...`);
   - `dev-snapshot.ps1`;
   - the game folder junction;
   - the scene caches: the prepare script checks every file pair by hash and found them identical;
   - the map order.

   The Klamath `devshot` metadata of both sides are equal field by field: camera, 4600 objects drawn, 48393 wall triangles, sun, light, glow off, 1920x1080 windowed. Only the commit, the exe hash, the time and the run id differ.
4. **306 does not run in this test.**
   - The test config has `[3d] shadows=0`.
   - `bindStaticShadows()` returns false when shadows are off (`shadow.cc:662`). Then `shadowed` is false (`scene_draw.cc:144`), so `bakeGround` is false (`scene_draw.cc:186`).
   - So neither `ensureGroundBake()` nor the new `uLightMode` line runs. These are the only two places 306 changes.
5. **GPU timers are equal on Klamath** (`devperf.log`, median of 5 windows):

   | timer | base | cand |
   |---|---|---|
   | `gpu_ms` | 1.69 | 1.68 |
   | `floor_ms` | 0.74 | 0.73 |
   | `walls_ms` | 0.33 | 0.33 |
   | `shadow_ms` | 0.00 | 0.00 |

   The GPU frame is about 1.7 ms of a 6 to 7 ms frame.
6. **The extra time is on the CPU, in code 306 does not touch** (`process.stdout.log`, Klamath):

   | stage | base | cand | change |
   |---|---|---|---|
   | `loop frame` | 6.78 | 8.35 | +23% |
   | `loop input` | 0.31 | 0.37 | +22% |
   | `objwalk walk` | 1.37 | 1.94 | +42% |
   | `refresh objects` | 2.05 | 2.81 | +37% |
   | `present refresh` | 3.49 | 4.58 | +31% |

   - `objwalk walk` times the CPU walk over the objects (`scene_objects.cc`). In steady state it reuses cached cards: about 700k hits against 120 misses.
   - New Reno shows the same pattern: walk +41%, refresh objects +37%.
7. **Later maps of the same candidate run do not show it.** In the cave (`klatoxcv`, night, no sun), walk is 0.374 vs 0.352 ms and the frame 3.80 vs 3.98 ms. In Vault City the frame is 1.196 vs 1.264 ms.
8. **The candidate's walk is noisier.** In the 14 baseline samples it ranges from 1.12 to 1.66 ms. In the 11 candidate samples it ranges from 1.35 to 2.48 ms. Klamath is slower in all 5 windows of the candidate run, so this is not one short spike.
9. **The `cpu` column is a single reading.** It is a 1-second sample taken before each map starts (`Read-CpuLoadPercent`). It is 23 on both sides for Klamath, and it says nothing about load during the measured seconds.
10. **What players run is not measured.** Players have sun shadows on by default (`settings.h:298`, `shadows = true`). With shadows on, 306 makes the quarter-resolution ground bake compute the sun shadow. perf-check measures neither that path nor the player default.

## Hypotheses (not proven)

- **H1, the machine state while the first two candidate maps ran.** This is the most likely.
  - **For:** the extra time sits in CPU stages that 306 cannot reach (input, object walk, 2D refresh). The candidate samples spread more. The maps measured later in the same run are close to the baseline.
  - **Against:** the start-of-map CPU sample is the same. But that sample does not cover the measured seconds.
  - **Possible sources:** something running in the background, the CPU clock, or the run order. The candidate always ran second, straight after the baseline.
- **H2, code layout.** The 80 extra bytes may shift where the hot functions sit in memory.
  - This is unlikely. A layout effect is stable from window to window, while the candidate's samples are noisier. It also hardly gives +40% in several unrelated functions.
- **H3, how the baseline exe was built.** The baseline was built outside the review harness. The compiler and linker are the same, but its flags are not recorded.
- **Rejected:** that the GPU cost of the 306 bake causes the drop. The bake does not run in this config, and the GPU timers are equal.

## Local error in 306

I found none. The 306 changes run only with sun shadows on:

- `ensureGroundBake()` exits early once the bake exists, so the saving and restoring of the bindings happens only when the bake is created;
- the bake pass sets `uLightMode` to 1.

So there is no fix plan, and head 517 stays as reviewed.

## Plan for the next perf slot (Main runs it; not run here)

1. **Build both sides the same way.** Use the same harness and `tools/configure-build.ps1 Release`: base from main `de6d5267` (same `ce/` as `c1b9afef`), candidate from `0ca0d771`. This removes H3.
2. **Noise floor.** Run one A/A pair with the base exe twice in the same runtime roots.
3. **Interleave three pairs:** A B, B A, A B. A drop counts as real only if the candidate is slower in all three pairs, and by more than the A/A spread. Keep `devperf.log` and `process.stdout.log` of every map. If the drop again sits only in the CPU stages, then H1 or H2 holds, not 306.
4. **The real 306 path.** Use a copy of the test config with `shadows=1`, Klamath and New Reno only, two interleaved pairs. Compare `floor_ms` and `gpu_ms` along with fps. This measures what players run.
5. **Bound.** About 20 minutes of the slot, with no other heavy process running.
