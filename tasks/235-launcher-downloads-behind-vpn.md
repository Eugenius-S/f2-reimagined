---
id: 235
title: The launcher downloads work with and without a VPN
milestone: M1
status: done
owner: any
suggested: claude
priority: high
tier: 0
claude-only: no
user-check: yes
depends-on: []
reviewed: no
---

## Goal

Install, update, the release check and the Russian translation work for a
player whose traffic goes through a VPN, the normal case for the owner and
for most Russian players, and keep working without one.

## What was seen (2026-11-08)

Through a datacenter VPN exit (Datacamp, Houston), for more than an hour:

- `github.com/<owner>/<repo>/releases/download/...` answers 504 for every
  asset, ours and RPU's. HTTP/1.1 and a browser user agent change nothing.
- `github.com/.../releases/latest` (the 302 page), `releases/expanded_assets/<tag>`,
  `raw.githubusercontent.com` and the signed CDN `release-assets.githubusercontent.com`
  answer 200.
- `api.github.com` asset downloads (`Accept: application/octet-stream`) redirect
  to that CDN, but the unauthenticated limit (60 per hour per IP) is used up on
  a shared VPN address.
- The launcher showed "no network" and "the Russian translation could not be
  installed".

## Plan

Every download tries the routes in order and moves on when one fails:

1. `github.com/.../releases/download/...` (today's route).
2. The API asset route, `api.github.com/repos/.../releases/assets/<id>`, to the CDN.
3. A mirror on `raw.githubusercontent.com`: CI force-pushes an orphan branch
   (for example `dist`) holding `version.txt` and the release zip after each
   release upload. One commit, so the repo does not grow. Check the 100 MB file
   limit and say what happens to the RPU translation (its release zip is 140 MB;
   the launcher needs only `rpu_russian.dat`, 6.4 MB). Check RPU's license
   before mirroring anything of theirs; if it does not allow it, RPU keeps
   routes 1 and 2 only and the launcher says so plainly.

The log names the route that worked. When all fail, the GUI says "GitHub does
not answer" with the error, not "no network".

## Acceptance

- With the owner's VPN on: the release check shows the version, an update runs,
  and the Russian translation installs.
- Without a VPN the same works (route 1).
- A test covers the fallback order with a fake failing route.

## Log

- 2026-11-08 claude: cut from the owner's report; measurements above.
- 2026-11-08 claude: done in the PR. Every launcher download (version.txt, the release zip, the RPU zip) tries github.com, then the API asset route (`Accept: application/octet-stream`), then, for our release only, the `dist` mirror on raw.githubusercontent.com (CI force-pushes an orphan `dist` branch with version.txt and the zip, about 34 MB, under the 100 MB limit). RPU has no license file and no stated terms, so it is not mirrored: it keeps routes 1 and 2 and the error says it has no mirror. The log names the route that worked; all routes failing shows "GitHub does not answer" plus the errors. Test: `launcher_routes_tests`. On this VPN machine route 1 gave 504, the API gave 403 (limit used up) and `dist` does not exist until CI runs on main, so routes 2 and 3 were not seen working end to end here; user-check stays yes (after the merge, check the release check with the VPN on and off).
