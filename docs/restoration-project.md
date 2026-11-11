# Restoration Project Updated

As checked on 2026-11-05, the latest official release is RPU **v2.4.34** (published 2025-10-18). The official
release page is <https://github.com/BGforgeNet/Fallout2_Restoration_Project/releases/latest>;
the Windows installer is
<https://github.com/BGforgeNet/Fallout2_Restoration_Project/releases/download/v2.4.34/rpu_v2.4.34.exe>.
Its published SHA-256 is `226b2252840a9230bb4ced1550becb2bd69d3580c15ee5c2148d5dc0a750f898`.
The upstream installation notes are in the repository README:
<https://github.com/BGforgeNet/Fallout2_Restoration_Project>.

## Through the launcher

The launcher's **Restoration Project** option (console: `--restoration-project`)
fetches `rpu_<tag>.zip`, RPU's manual package, from the latest official release
at the player's request; the project never rehosts it. It copies the package
over the game folder and applies RPU's `rpu-install.bat` steps: `patch000.dat`,
`f2_res.dat` and the loose `data/` files move to `backup/rpu` (the saves,
`data/*.txt`, `data/*.gcd` and the music stay), `mods_order.txt` goes into
`mods/` and `gl_ammomod.int` into `data/scripts/`. Every file the package
replaces is kept in `backup/rpu` first. Then the scene export runs with the
exporter's `rp` map group added to `all`.

## Install into a separate game copy

RPU's Windows installer handles a large set of files, archives and options, so
use its installer instead of copying a hand-picked subset.

1. Make a full copy of the existing CE game directory and keep the original
   intact. Upstream says RPU must be installed on vanilla Fallout 2. If the
   existing folder already contains another mod or edited game data, make the
   copy from an unmodified Fallout 2 installation instead.
2. Before installing, back up the copy's `fallout2.cfg`, `ddraw.ini`,
   `mods/`, `data/`, and any existing `SAVEGAME/` directory. Preserve a copy
   of the entire directory if you may want to restore every file exactly.
3. Download `rpu_v2.4.34.exe` from the official release above and run it.
   Point it at the copied game directory, select the language and options, and
   let it finish.
4. Start a new game after installing RPU. Upstream documents updating from an
   existing RP/RPU install as an exception; ordinary saves should not be
   reused for a new installation.
5. Start the copied game with `fallout2-ce.exe`. Keep the original
   installation separate while checking engine compatibility and scripts.

## Roll back

Close the game. Restore the backed-up files and directories, or remove the
RPU copy and make a fresh copy from the untouched original. Restoring only
`ddraw.ini` is not a complete rollback because the installer also installs
archives and supporting files.

## Compatibility status

The task's scratch-copy run log records the tested CE build, game copy,
language, new-game startup and map loads. A successful boot does not imply
that every RPU script or sfall feature works; missing sfall features are
tracked separately in tasks 186 and later.
