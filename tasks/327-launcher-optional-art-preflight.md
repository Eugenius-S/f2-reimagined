---
id: 327
title: Refuse known optional art path conflicts before updating
milestone: M1
status: done
owner: gpt
priority: high
tier: 1
claude-only: no
user-check: no
depends-on: [326]
reviewed: yes
---

## Problem

An optional art folder in the package can be blocked by an existing
ordinary file at the destination or one of its parent paths. The previous
installer did not notice until after writing core files. The version
stamp stayed old even though the game and exporter were already new.

## Fix

Before creating any backup or copying install files, check the directory
roots for the optional art trees actually present in this package. Share
the same fixed path list between preflight and copy. Do not remove a
conflicting file, and do not object to unrelated files when the package
does not include their optional directory.

This checks known directory layouts, not permissions on every leaf or a
new filesystem change occurring after preflight.

## Verification

The real Windows launcher reproduced the mixed tree on a disposable
loopback fixture. The revised regression checks the complete file set
and backup directory. Additional fixtures verify a conflicting splash
ancestor and a legitimate update that lacks optional wide art.
