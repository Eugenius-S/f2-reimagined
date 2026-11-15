---
id: 288
title: A merge to main does not cancel the publish of the players' build
milestone: M1
status: todo
owner: any
suggested: gpt
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
