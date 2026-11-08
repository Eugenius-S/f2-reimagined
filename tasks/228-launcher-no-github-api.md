---
id: 228
title: The launcher works when GitHub's API rate limit is spent
milestone: M1
status: done
owner: claude
suggested: claude
priority: high
tier: 0
claude-only: no
user-check: no
depends-on: []
reviewed: yes
---

## Goal

`--russian` and `--check-release` keep working when api.github.com has
answered 403 "rate limit exceeded" (60 unauthenticated requests per hour per IP).

## Context

The launcher read the RPU translation's version from the releases API, so
`--russian` failed with "cannot reach the official RPU release (http status
is not 200)" once the limit was spent. The release notes for `--check-release`
and the GUI use the same API.

## Acceptance

- The RPU tag comes from the Location header of the 302 that
  `https://github.com/BGforgeNet/Fallout2_Restoration_Project/releases/latest`
  answers (not rate limited); the API is not used for it.
- When the notes cannot be fetched the launcher logs one line saying so.
- The update check itself depends only on `version.txt` from the release
  download URL.

## Log

- 2026-11-08 claude: `installRussianTranslation` now calls the new
  `httpRedirectLocation` (WinHTTP with `WINHTTP_DISABLE_REDIRECTS`, then
  `WINHTTP_QUERY_LOCATION`) and takes the text after `/releases/tag/`. The API
  fallback is dropped.
- 2026-11-08 claude: the version check already used `version.txt` from
  `kDefaultUpdateBase` (github.com download URL), not the API; only the notes
  use the API. A failed notes call now logs "release notes are unavailable".
- 2026-11-08 claude: checked with a Release build, ctest, `--check-release`,
  and `curl -sI` on the release page (see the PR).
