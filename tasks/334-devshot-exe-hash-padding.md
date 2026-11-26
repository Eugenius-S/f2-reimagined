---
id: 334
title: Devshot reports the exact executable SHA-256 at every file length
milestone: M1
status: doing
owner: gpt
suggested: gpt
priority: high
tier: 1
claude-only: no
user-check: no
depends-on: [330]
reviewed: no
---

## Goal

A Windows devshot's executable hash matches the executable that ran, including
sizes that leave 56 to 63 bytes in the final SHA-256 block.

## Context

Follow-up to screenshot metadata 183 and 330. Exact main `9f54f368` Windows
CI artifact `45868058942` reached Klamath e0 in full 3D. Its executable and
the staged test copy both hash to
`87a6e21031a9c3825b6fd99ecdd7498cf6ac92c1ad5e19aeb670e88dae5d8a0f`,
but the normal devshot reports
`b5d667ebe0812c27a9d37978fab57ad3ca4cb198b734815cad51e2f311f50cd9`.
Size 11387576 leaves 56 bytes. Replaying the engine's stale second padding
block independently yields the reported hash. Zeroing its first 56 bytes
before writing the length yields the file hash.

## Acceptance

- Standard SHA-256 matches for a short final block and for lengths 56 to 63
  bytes modulo 64, including a multi-block input and the exact CI executable.
- A native isolated Windows devshot agrees with the actual copied executable's
  SHA-256 for the padding-boundary case. Preserve the field and capture format.
- Capture and targeted hash checks plus CI pass. A successful scene load is
  separate from checking the hash in its metadata.

## Out of scope

Changing the hash algorithm or adding dependencies, changing snapshots or
game presentation, changing release or performance gates.

## Log

- 2026-11-27 gpt: grounded on the captured Klamath e0 sidecar under
  `build/rule1-9f54f368/smoke` in the isolated 291 verification worktree.
