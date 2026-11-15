---
id: 294
title: An interrupted export preserves a recoverable complete scene cache
milestone: M1
status: todo
owner: any
suggested: glm
priority: normal
tier: 2
claude-only: no
user-check: no
depends-on: [293]
reviewed: no
---

## Goal

A failed rebuild leaves the previous complete scene usable, or recoverable on the next export, instead of mixing old and new artifacts.

## Context

`tools/scene_export/export.py:552-632` builds directly into the live directory: sidecars and images change before `provenance.json` is written. The [player cache batch](220-launcher-builds-every-map.md) and [273](273-missing-cache-builds-on-entry.md) need a complete scene while an export fails or is retried. The observed write order is a risk, not proof of the white-floor cause in [270](270-white-ground-capture.md).

## Acceptance

- Build a map/elevation in a validated sibling staging folder; validate it with 293 before publishing. Keep the previous complete cache until publication succeeds. On Windows use a recoverable rename/restore sequence; do not assume an atomic replacement of a nonempty directory.
- A second exporter targeting the same scene waits or exits clearly without modifying the first one's files. Different cache roots remain independent.
- Fault-injection fixtures stop after an artifact write and during publication, then restart. The old cache hashes remain available, recovery restores or completes one valid scene, and staging/backup cleanup stays within that scene's cache root.
- Exporter tests pass, including a successful rebuild and recovery; no game files or unrelated cache directories change.

## Out of scope

Renderer changes, background engine export (273), general filesystem transaction libraries, or an atomic-directory guarantee that Windows cannot provide.

## Log

- 2026-11-15 gpt: cut from the tooling and verification follow-up to the existing batches; source observations above are scoped gaps, not unverified game bug claims.
