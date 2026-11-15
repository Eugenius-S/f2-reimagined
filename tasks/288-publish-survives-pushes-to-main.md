---
id: 288
title: A merge to main does not cancel the publish of the players' build
milestone: M1
status: doing
owner: gpt
suggested: glm
priority: high
tier: 0
claude-only: no
user-check: no
depends-on: []
reviewed: no
---

## Goal

The manual publish of the `latest` release always runs to its end, and the release never holds a version number that does not match its zip.

## Context

- `.github/workflows/verify.yml`: the publish dispatch shares the concurrency group `verify-refs/heads/main` with the push runs, with `cancel-in-progress: true`. 4 of the last 12 publish runs were cancelled (2026-11-11 to 2026-11-13).
- `gh release upload` sends the zip and `version.txt` as two assets: a cancel between them leaves the release mixed. The launcher writes the remote `version.txt` after install and ignores the one inside the zip.

## Acceptance

- A dispatch run has its own concurrency group and is never cancelled by a push; two dispatches queue, the later one wins.
- The launcher records the version from the `version.txt` inside the zip it installed.
- The AGENTS.md Rule 1 text names how to see that the publish ended (one command).
- One real dispatch after the merge ends green with a push to main landing while it runs, or the Log says why that could not be staged.

## Out of scope

Signing the release.

## Log

- 2026-11-15 claude: cut from the repo audit of 2026-11-15 (find 2).

### 2026-11-15 gpt

Done: isolated manual publication in the fixed `verify-publish-latest` concurrency group across refs; automatic checks keep their separate `verify-check-` prefix and cancel stale runs. The launcher validates the package's regular, nonempty, single-line `version.txt` before backups or copies, records that ZIP identity through the existing atomic stamp after successful installation, and preserves the previous rollback backup when the ZIP already matches the installed version. Rule 1 names `gh run watch <dispatch run ID> --exit-status`.

Check: Release build and full CTest 29/29 passed, including 13 real HTTP installer tests (10 invalid-version subcases) and the existing 26 RP integration tests. Added remote A/package B, same-installed-version backup, missing/empty/multiline/control/dot/directory version coverage. The existing empty-version subcase also asserts the Russian error. Exporter Python tests 148/148 and regression Python tests 12/12 passed. Standard pixel regression passed all 154 views with zero changed pixels; references and thresholds were unchanged. A local incremental build initially missed the translation header; the actual launcher consumers were then rebuilt, the live Russian HTTP assertion passed, and full CTest passed again. No build-script changes are included.

Next: keep this task doing until independent review, the protected merge and post-merge Rule 1 handover finish. Rebuild fresh main, confirm runtime commit and executable hash, run offscreen and clean four-map performance gates, then publish that same full commit and watch it to success. Before the first dispatch using the new key, wait for all legacy-key manual runs to become terminal. Stage a genuine main push during publication if safe, otherwise record the exact reason; confirm `latest/version.txt` matches. No post-merge dispatch has been run yet.
