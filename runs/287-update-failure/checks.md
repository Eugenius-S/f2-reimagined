# 287: Failed updates keep their old build version

## Reproduction

The integration fixture serves synthetic release zips over loopback HTTP with
`--update-url` and installs only into temporary game folders. Its Win32 read
lock allows backup reads but denies overwrites and renames. No game is started.

Before the fix, the locked game exe reported `copy failed` but returned 0,
logged `version.txt: new-build`, and left the old executable installed. A corrupt
zip made tar exit nonzero without trying Expand-Archive. Present optional
launcher/art copy failures and a locked version file also returned success.
The baseline fixture failed all six original test methods (eight assertions).

## Changes

- Check both extractor exit codes, expose tool output, and retry a failed tar
  extraction with Expand-Archive in a clean stage using `-ErrorAction Stop`.
- Require regular files for the game exe, ce.dat and exporter before backing up
  or copying anything into the installation.
- Stop on required and present optional copy failures; keep absent optional
  files optional and preserve the player's config.
- Close and check a temporary version file, then atomically replace version.txt.
  A failed write or replace leaves the old version available for the next run.
- Translate the new user-facing messages into Russian.

## Checks

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools/configure-build.ps1 Release
ctest --test-dir build --output-on-failure
py -3 -m unittest discover -s tools/scene_export -p 'test*.py'
py -3 -m unittest discover -s tools/regression -p 'test*.py'
```

Release build succeeded. CTest passed 28/28, including nine launcher integration
tests: locked exe and retry; corrupt zip and Russian unpack/fallback messages;
each missing required file with no installation writes; a directory instead of
a required file; absent optional files; failed default config copy; failed art
tree copy; locked optional launcher; locked version stamp. The exporter passed
148 unit tests, and regression tools passed 12 unit tests.

To run just the isolated launcher checks:

```powershell
py -3 ce/tests/launcher_progress/launcher_install_test.py build/fallout2-ce-3d-launcher.exe
```

Rollback of already copied files stays in task 307; RP transactions and shared
installer refactoring stay in 289 and 292. No gameplay or rendering code changed.
