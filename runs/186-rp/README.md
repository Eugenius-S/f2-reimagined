# 186: what RP asks of sfall and CE lacks

RPU 2.4.34 in a scratch copy outside the repository (copied from an installed
RPU game; no RP file is committed). Build: this branch on `origin/main`
`79ee16efe`.

## How it was counted

- Static: `tools/dev-snapshot.ps1 -ScanUnimplemented` over all 2961 scripts
  (every DAT and `data/`), same result as 185's scan.
- Runtime: a new `sfall gap:` console line (`ce/src/sfall_gaps.cc`) printed the
  first time a script hits an opcode CE has no handler for, an unknown
  metarule, a stub opcode, or registers a hook CE never runs; game exit prints
  `sfall gap summary:` with the counts. Routes: a new game at ARTEMPLE, then
  ARVILLAG, KLADWTWN, DENBUS1, DENRES1, ABBEY, ABBASEM, ABUNDGR, ENCDOCK,
  EPAMAIN1, EPAMAIN2, EPASL, EPAX, SAFEHS1-3, SFSHENG, SFSUB, SLVCAMP,
  TRIBE01, TRIBE02, VILMERCH, HUBSTASH (four tour runs; every map loaded).
- Ini keys: RP's own `ddraw.ini` settings (its `extra/package/ddraw.ini`, the
  keys it sets on top of stock sfall) against what CE reads.
- Test runs now read the scratch copy's `ddraw.ini`: `dev-snapshot.ps1
  -GameRoot` copies it beside the test exe (CE reads `ddraw.ini` next to its
  exe; before this, RP test runs ran without RP's sfall settings).

## After part 2: 7 gaps, 0 blocking

Before: 12 gaps, 1 blocking. Part 2 implements five, following sfall's
`Modules/Perks.cpp` and `Modules/FileSystem.cpp`:

- `set_fake_perk` (the blocking one), with `set_fake_trait` (was a stub),
  `has_fake_perk` and `has_fake_trait` (`ce/src/sfall_perks.cc`); kept in
  `sfallgv.sav` in sfall's record layout. The character screen does not
  list them yet.
- `fs_seek`, `fs_read_short`, `fs_write_short`, and with them
  `UseFileSystemOverride`: `fs_copy` now holds the file's bytes, and an
  edited file is served to the game from a temp file. On a route the two
  global scripts run to the end and patch 56 walking FRMs; no `sfall gap`
  line fires.

Left (none blocks): `set_dude_obj`, `HOOK_SETGLOBALVAR`, `WorldMapSlots`,
`ExtraSaveSlots`, `FemaleDialogMsgs`, `EnableHeroAppearanceMod`,
`BoxBarColours`. The static scan now lists only `set_dude_obj` and
`HOOK_SETGLOBALVAR`.

Checks: ctest 25/25 (with the new `sfall_perks_tests`). The original view
regression passed 153 spots; `den-residential` timed out with no snapshot,
and it fails the same way with main's exe, so it is not from this change.
Not seen live: the `epac3` reply, an edited FRM being drawn in play.

## The gap list before (12 gaps, 1 blocking)

Count: static references (scripts), then runtime fires on the routes.

| Gap | Static | Runtime | Where | Blocks play? |
| --- | --- | --- | --- | --- |
| opcode `set_fake_perk` (0x81bb) | 1 (1) | 0 | `epac3` (EPA, Toaster repair dialog, Node015) | **Yes**: the undefined opcode kills the script mid-node, so the reply never shows and `GVAR_TOASTER_SLOTMACHINES` is never set |
| opcode `fs_seek` (0x8202) | 4 (2) | 2 per game load | `gl_k_walking_speed`, `gl_k_goris_derobing`, at game start | No: the two optional global scripts die; slow walking frames and Goris's derobing speed stay vanilla |
| opcode `fs_read_short` (0x820a) | 2 (2) | 0 (the scripts die at `fs_seek` first) | same two scripts | No, as above |
| opcode `fs_write_short` (0x81fb) | 2 (2) | 0 (same) | same two scripts | No, as above |
| metarule `set_dude_obj` | 1 (1) | 0 | sfall's `gl_partycontrol` | No: party control is off in RP (`sfall-mods.ini` `[CombatControl] Mode=0`) |
| hook `HOOK_SETGLOBALVAR` (29) | 1 (1) | 0 (not registered while party control is off) | sfall's `gl_partycontrol` | No, as above |
| ini `[Misc] UseFileSystemOverride=1` | | | `ddraw.ini` | No: CE's `fs_copy` only aliases a path, so files a script edits are not served (the `fs_*` rows) |
| ini `[Misc] WorldMapSlots=21` | | | `ddraw.ini` | No: the world map town list shows CE's default slots; towns stay reachable on the map |
| ini `[Misc] ExtraSaveSlots=1` | | | `ddraw.ini` | No: fewer save slots |
| ini `[Misc] FemaleDialogMsgs=2` | | | `ddraw.ini` | No: a female hero gets the default dialogue text |
| ini `[Misc] EnableHeroAppearanceMod=1` | | | `ddraw.ini` | No: the hero keeps the default look (`set_hero_style`/`set_hero_race` are stubs) |
| ini `[Misc] BoxBarColours=11111` | | | `ddraw.ini` | No: interface box colours |

Not gaps: `ElevatorsFile`, `IniConfigFolder` (CE reads them), `KarmaFRMs`,
`KarmaPoints` (migrated into `game.cfg`), `BoostScriptDialogLimit` (always on
in CE), `HiResMode`, `OverrideArtCacheSize`, `ProcessorIdle` and the
`[Debugging]` keys (sfall's own engine and logging switches, nothing for CE).
RP ships no `hs_*` hook scripts; the RP global scripts not listed ran without
a gap line.

## EPAMAIN1

EPAMAIN1's map-entry script (`epa1.ssl`: start hex, the first-visit message,
the car and lighting) reaches a presentable scene on the route from SLVCAMP:
`scene_view=full`, the hero on the map, the first-visit message in the log
(`epamain1-route.jpg`). 185's stall there did not come back on current main
in four route runs.

## Seen on the way, not sfall gaps

- HUBSTASH: the tour stalls after the map loads (no gap line, scene
  available, no snapshot within 30 s), as in 188's sweep.
- SFSHENG: the map-entry script requests a dialogue; the tour passes it only
  with `-SkipScriptBlocking` (as 187 found).
