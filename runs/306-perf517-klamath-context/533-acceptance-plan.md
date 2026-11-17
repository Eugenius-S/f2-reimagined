# PR 533 (task 308): acceptance plan for the heavy slot

PR 533 is head `4aa817e8`. Its code is done and CI is green. What is still owed is the heavy-slot acceptance below. Main runs it. Nothing in this plan was run yet.

## Setup

1. **Build** `4aa817e8` with `tools/configure-build.ps1 Release` in a clean checkout. You need `fallout2-ce-3d-launcher.exe` and the tests.
2. **Run the CTest suite:**

   ```
   ctest -R "launcher_(ui_state|progress|routes|ru|rp|uninstall|install|rp_install)_tests"
   ```

   Every test must pass. `launcher_ui_state_tests` prints `launcher ui state: ok`.
3. **Make a throwaway folder** under the temp directory. Put a fake `master.dat` and `critter.dat` in it, as `launcher_install_test.py` does. Make a second throwaway folder the same way, but with no game exe in it.
4. **Start a local HTTP server on 127.0.0.1.** It serves the release info and the package from `launcher_install_test.py`, with two handlers:
   - the release-info reply waits 20 seconds before it answers;
   - the package goes out in small chunks for about 30 seconds, so the install is slow.
5. **Start the launcher**, all inside the throwaway folder, with no real game folder and no network outside the local server:

   ```
   fallout2-ce-3d-launcher.exe --ui --game-root <scratch> --release-info-url <local> --update-url <local> --settings-file <scratch>\launcher.ini --ui-language en
   ```

   For the RU sequences, start it again with `--ui-language ru`.

## Sequences and expected results

1. **Delayed reply during a slow install.**
   - **Steps:** press Install at once, before the delayed release reply arrives. The reply then lands in the middle of the install.
   - **Expected:**
     - Play, Install, Remove, Browse and the three boxes stay off;
     - the folder field cannot be edited;
     - Install sent by keyboard does nothing;
     - when the install ends, the controls come back by the normal rules and the status shows the install result, not "up to date" from the late reply.
2. **Idle folder change makes the old reply stale.**
   - **Steps:**
     - with the window idle, restart it with the 20-second reply delay;
     - before the reply arrives, type the second throwaway folder into the folder field.
   - **Expected:**
     - Play and Remove go off at once, because that folder has no game;
     - the RP and translation boxes reload from the new folder;
     - when the old reply arrives, no update starts and the status does not say the new folder is up to date.
3. **Busy close.**
   - **Steps:** during a slow install, press the window's close button twice.
   - **Expected:**
     - the window stays open;
     - the status and the log say it will close when the operation finishes;
     - the install finishes, its files are complete (same check as `launcher_install_test.py`), and then the window closes on its own;
     - no launcher worker process is left running.
4. **Idle close.** With nothing running, the close button closes the window at once.

## Images (at most four)

1. EN: sequence 1, the late reply during the install, with every control off.
2. RU: the same moment.
3. EN: sequence 3, the deferred close message in the status and the log.
4. RU: the same message.

## Accepted when

- Every CTest test passes.
- Sequences 1 to 4 give the expected results, in both EN and RU.
- The four images match the descriptions above.
- No worker process outlives its window.
- Nothing outside the throwaway folders changed.

## Bound

About 25 minutes of the slot: about 10 for the build, a few for CTest, the rest for the sequences and the images.

## Out of scope

- 307 rollback;
- 309 exclusion between separate launcher processes;
- window redesign;
- stopping a write halfway.
