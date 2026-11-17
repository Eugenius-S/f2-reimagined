#include "launcher_ui_state.h"

#include <cstdio>
#include <cstdlib>

using namespace launcher_ui_state;

static void check(bool ok, const char* what)
{
    if (!ok) {
        std::printf("FAILED: %s\n", what);
        std::exit(1);
    }
}

static bool allOff(const Controls& c)
{
    return !c.root && !c.options && !c.install && !c.remove && !c.play;
}

int main()
{
    // Idle: Play needs an installed game with nothing pending; Install needs a folder and the launcher.
    Controls idle = controls(false, true, true, false, true);
    check(idle.root && idle.options && idle.install && idle.remove && idle.play, "an idle installed game offers everything");
    check(!controls(false, true, true, true, true).play, "a pending recovery keeps Play off");
    check(!controls(false, true, false, false, true).play && !controls(false, true, false, false, true).remove,
        "no game, no Play and no Remove");
    check(!controls(false, false, false, false, true).install, "no folder, no Install");
    check(!controls(false, true, true, false, false).install, "the launcher removed itself, no Install");

    // A slow install, and the release reply asked at start-up arrives in the middle of it.
    Lifecycle life;
    unsigned asked = life.rootGeneration;
    check(life.start(), "the install starts");
    check(!life.start(), "a second operation is refused while one runs");
    check(allOff(controls(life.busy, true, true, false, true)), "every control is off while the install runs");
    check(!life.releaseMayAct(asked), "a late release reply does nothing while busy");
    check(!life.changeRoot() && life.rootGeneration == asked, "the worker's folder cannot change while busy");
    check(!life.finish(), "no close was asked, so the window stays");
    check(life.releaseMayAct(asked), "once idle, a reply for the same folder may act");

    // Idle folder change: a reply asked for the old folder no longer acts.
    check(life.changeRoot(), "an idle folder change is allowed");
    check(!life.releaseMayAct(asked), "a reply asked for the old folder cannot act for the new one");
    check(life.releaseMayAct(life.rootGeneration), "a reply asked for the new folder may act");

    // Closing while busy waits for the worker; closing idle is at once.
    check(life.requestClose(), "an idle window closes at once");
    Lifecycle closing;
    check(closing.start(), "an operation starts");
    check(!closing.requestClose(), "a close while busy is deferred");
    check(closing.busy && closing.closeRequested, "the worker keeps running under the open window");
    check(!closing.requestClose(), "a second close is deferred too");
    check(closing.finish(), "the deferred close happens when the worker ends");

    std::printf("launcher ui state: ok\n");
    return 0;
}
