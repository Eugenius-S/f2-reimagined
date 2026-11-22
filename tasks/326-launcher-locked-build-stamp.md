---
id: 326
title: Refuse an already locked build stamp before installing
milestone: M1
status: done
owner: gpt
priority: high
tier: 1
claude-only: no
user-check: no
depends-on: []
reviewed: yes
---

## Problem

A Windows update could replace the game executable, ce.dat, exporter and
wide art before version.txt failed to rename. A stamp already locked by
another process left the original version beside new files.

## Fix

The launcher checks the destination stamp's replacement access before
saving backups or installing payload files. The handle is opened only to
check access and is closed without changing the file. Missing stamps on
manual or fresh installs are accepted. The existing final rename remains
atomic and reports its own failure.

This is not a complete update transaction or a rollback for a lock that
appears after preflight, a failed write elsewhere or a power loss.

## Verification

An isolated Windows loopback update fixture with a read handle that
forbids replacement reproduced the mixed installation. The stricter
regression check now compares the entire game's disposable file tree
before and after, including optional art and backups. A separate test
confirms that an unstamped manual install can still update and preserve
its configuration.
