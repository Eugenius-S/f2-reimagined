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
reviewed: yes
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
- 2026-11-27 gpt: fixed the second SHA-256 padding block by clearing its
  first 56 bytes after the first block is transformed. No capture format or
  algorithm change. `py -3 runs/334-devshot-exe-hash-padding/check_sha256.py <CI-exe>`
  compiles the actual source routine with the local MSVC toolchain and compares
  it to Python hashlib: 0, 1, 55-64, 119-121, 127, 128, 184 and 1024 bytes
  pass. Baseline `facbdb671` fails each 56-63 byte case. The read-only
  `f2r-291-evidence-images-leave-git/build/rule1-9f54f368/ci/fallout2-ce.exe`
  (11387576 bytes, remainder 56) changes from the reported `b5d667ebe0812c27a9d37978fab57ad3ca4cb198b734815cad51e2f311f50cd9`
  to the standard `87a6e21031a9c3825b6fd99ecdd7498cf6ac92c1ad5e19aeb670e88dae5d8a0f`.
  The script requires that executable and the local MSVC 14.40 / SDK 22621
  paths. Next: native Windows devshot with an exact boundary-length executable,
  Release build, capture checks and CI on the candidate; independent review.
  Those checks remain pending and this task remains doing.

- 2026-11-27 gpt: independent Terra High accepted the padding change at
  `af3a9710` and reproduced the standard vectors and the exact CI file hash.
  Take the checked component through Windows CI. The local Release build
  did not start because the shared Windows slot was occupied. Task 334
  stays open for the native boundary devshot on the merged Release. This
  pending test also stays ahead of publication of that build.
