# Voice package configuration

The launcher shows a voice package only when its URL, exact byte length and
SHA-256 are configured, and only for its matching game variant. Package names
are `VANILLA_1C`, `RP_1C`, `RP_LEVY`, `VANILLA_FARGUS` and `VANILLA_LEVY`.
For each name set CMake cache values `F2R_VOICE_<name>_URL`,
`F2R_VOICE_<name>_SIZE` and `F2R_VOICE_<name>_SHA256`. The hash is 64
lowercase hexadecimal characters. URLs must use HTTPS, except loopback HTTP
for tests. A package must be a DAT containing only the compatible voice files;
it must be validated before setting release defaults. Empty defaults mean that
no extra voice packages are published.

The launcher stores its own DAT and receipt under `mods`. It checks the receipt
and installed hash before removing or replacing the file. Existing unrelated
DATs, including manually installed voice packages, are left in place. Text-only
CLI operations do not change voice. `--voice-translation <id>` changes voice;
`--text-translation <id> --voice-translation <id>` changes both in one
transaction. Add `--text-with-install` to apply these choices after installation.
Original audio is `original`; the other IDs are the lowercase package names with
underscores changed to hyphens.

The known BGforge 1.0.1, 1.1 and 1.2 manual voice DATs are kept on disk but their mod-order entry
is disabled when selecting a voice. Full uninstall restores the saved entry.
RP voice packages require the verified RP 2.4.34 `mods/rpu.dat`.

For the loopback integration fixture, configure `VANILLA_1C` with URL
`http://127.0.0.1:18751/voice`, length `22`, and SHA-256
`16b874f8ebf84d286c4684a0a7a78c0cd00c84c35515a0eee9d889746537f2c1`, then
run `launcher_voice_test.py` with `F2R_VOICE_FIXTURE=1` and the fixture launcher
path. This is test data, not a playable voice package or a release default.
