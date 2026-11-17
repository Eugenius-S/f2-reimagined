# GLM night report, 2026-11-17

Queue: `runs/glm-night-2026-11-17.md`. Heavy-operation slot shared with Opus306
(PR #517): no local export, build, game run or perf ran tonight; existing
results, light unit tests and remote CI only. Nothing merged: the night rules
ask for an independent Sol review before merge, so each PR carries its frozen
SHA and green CI, ready for review.

## 271: the all-map health table (PR #516, ready, CI green)

Reconciled, not re-swept. Coverage exactly 223 of 223 map elevations on one
build (exe sha256 `de20f410dc47a6a9`, main `de6d5267`): 184 valid rows, 106
clean of every threshold, 23 noisy and 16 never-measured rows listed and
clean of every threshold, 23 noisy and 16 never-measured rows listed and
counted nowhere. Worst valid rows: dnslvrun:e2 1% low 12.8 fps; cave2:e1 20.9
fps and 21.3 percent black; modbrah:e0 26.8 fps; sfchina2:e0 28.6 fps;
redwan1:e1 28.7 fps and 48 percent black; artemple:e0 and arvill2:e0 over the
2000 ms load gate. Table: `runs/271-all-map-health-table/README.md`, head
bfe8f01a. The 16 missing rows stalled their warmup on the uncached-map entry
watchdog (their caches are pre-exported; the rows wait for a free slot, the
stall itself is data for 272 and 273). The 23 noisy rows need one rerun on a
free machine; coast2:e1 at 99.8 percent black is a hint, not a result.

## 292: launcher HTTP and cfg cleanup (PR #518, CI green, 555e72aa)

One HttpGet opener serves httpGet, httpRedirectLocation and httpDownload;
httpGet applies the same cut-short check #473 gave httpDownload; the
sha256Tree check and the hash lines around the game run are gone; setLanguage
writes the cfg through launcher_uninstall::writeFileAtomic (temporary file,
rename) and the file header and uninstall comments now say what is actually
touched. New writeFileAtomic cases in launcher_uninstall_tests.

## 307: a failed update restores the previous build (PR #519, CI green, 430dcd95)

New `ce/src/launcher_update.h`: the update writes a recovery marker before
its first copy and journals every file it touches; the next start restores
the named backup (files, asset trees, journal-introduced files removed)
before anything launches or mutates the half-installed build. The backup now
also carries user-assets and both splash trees; the player's own files there
survive byte for byte; a retry cannot back the mixed state over the good
backup; explicit --rollback repairs an interrupted update even when the
backup's version equals the installed stamp; a blocked restore names the file
and keeps the material. launcher_update_tests covers the marker, the journal,
the interrupted restore, the player's files and the blocked path. Branch cut
from task/292-launcher-cleanup (depends on 292).

## Not done and why

- 309 (per-physical-root writer lock) not started: two unmerged PRs stand
  below it, its acceptance needs a two-process fixture, and the window kept
  its reserve for acceptance instead.
- Local install/update/Russian-install and the 307 fixture passes still owe
  their runs: the heavy slot stayed reserved all night. CI's ctest (including
  the new suites) is green on every PR.
- No sweeps, no noisy-row reruns, no renderer work tonight (slot and queue
  rules).

## Next

1. Sol review on #516, #518, #519 in that order; merge the stack in order
   (516, then 518, then 519).
2. Free slot: finish the 39 incomplete 271 rows (resume only), rerun the 23
   noisy rows, then hand 272 its causes table.
3. 309 after 307 merges, with the two-process fixture in its own worktree.
