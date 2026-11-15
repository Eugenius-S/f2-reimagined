---
id: 297
title: The packaged exporter exports one minimal synthetic map in CI
milestone: M1
status: todo
owner: any
suggested: glm
priority: normal
tier: 2
claude-only: no
user-check: no
depends-on: []
reviewed: no
---

## Goal

Before publishing, run the frozen exporter through a real minimal export outside the checkout.

## Context

`.github/workflows/verify.yml:102-103` smoke-tests the packaged exporter with `--help` only. `tools/scene_export/standalone.py` declares imports manually and `build_standalone.ps1` bundles the exporter tree as data. Help does not exercise map decoding, art loading, image output or sidecar writing. This belongs to the [player export batch](220-launcher-builds-every-map.md).

## Acceptance

- Use one small synthetic map with only the resources needed to export it, created with existing fixture helpers where possible. No copyrighted game data is committed or downloaded.
- Run the source exporter and packaged executable against isolated copies of that fixture, with the executable outside the checkout. Both complete with the expected scene identity and decodable sidecars/images; compare outputs that are independent of absolute paths and packaging provenance.
- The publish workflow runs the fixture export after packaging and before upload; a deliberately missing bundled resource fails the smoke check in a local negative test.
- Existing exporter tests pass; record source and frozen commands and output paths.

## Out of scope

A synthetic full game, a general asset generator, booting CE in CI, or reproducing all maps.

## Log

- 2026-11-15 gpt: cut from the tooling and verification follow-up to the existing batches; source observations above are scoped gaps, not unverified game bug claims.
