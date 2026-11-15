---
id: 299
title: The release checks every shipped wide-art group before upload
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

A release with missing or unreadable required menu, death or splash art fails before upload.

## Context

`.github/workflows/verify.yml:110-115` unpacks `wide-art.zip` and checks only one menu PNG, while the step promises menu, death and loading art. The [wide-art batch](128-wide-hud-bar-source.md) and [player install batch](242-installer-matches-main.md) require more than that one file. Inspect the actual consumers before choosing the required files; do not infer that currently published art is missing.

## Acceptance

- List the exact required relative paths from the menu/death/splash consumers and the existing permitted release payload, including English and Russian splash variants already shipped. Validate those paths and decode their images before release upload. If a required source is absent or has no redistribution permission, report it under 128 instead of generating, copying or inventing it here.
- A complete minimal fixture passes; deleting one required file from each group or truncating an image fails with its relative path.
- Wire the check into the current publish step without changing art or install paths. Its small tests run in CI and pass.

## Out of scope

Generating or redesigning art, changing the launcher copy/update transaction (287), release concurrency (288), or committing owner art.

## Log

- 2026-11-15 gpt: cut from the tooling and verification follow-up to the existing batches; source observations above are scoped gaps, not unverified game bug claims.
