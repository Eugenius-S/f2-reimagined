#ifndef LAUNCHER_UI_STATE_H
#define LAUNCHER_UI_STATE_H

// The launcher window's operation lifecycle (task 308), kept apart from
// Win32 so a test can drive every order of events: while a worker changes
// a game folder, the folder stays fixed, no late reply turns a button back
// on, and closing the window waits for the worker instead of leaving it
// running unseen.
namespace launcher_ui_state {

struct Controls {
    bool root = false; // the folder field and Browse
    bool options = false; // the translation, RP and auto-update boxes
    bool install = false;
    bool remove = false;
    bool play = false;
};

// What the window offers now. While an operation runs everything is off,
// whatever reply arrives in the meantime.
inline Controls controls(bool busy, bool rootChosen, bool installed, bool playBlocked, bool launcherLeft)
{
    Controls result;
    result.root = !busy;
    result.options = !busy;
    result.install = !busy && rootChosen && launcherLeft;
    result.remove = !busy && installed;
    result.play = !busy && installed && !playBlocked;
    return result;
}

struct Lifecycle {
    bool busy = false;
    bool closeRequested = false;
    // Bumped by every change of the chosen folder: a reply asked for under
    // an older number belongs to another folder.
    unsigned rootGeneration = 0;

    bool start()
    {
        if (busy) return false;
        busy = true;
        return true;
    }

    // The worker ended; true when a close asked for meanwhile should happen now.
    bool finish()
    {
        busy = false;
        return closeRequested;
    }

    // The worker's folder is fixed while it runs.
    bool changeRoot()
    {
        if (busy) return false;
        rootGeneration++;
        return true;
    }

    // True when the window may close now; while busy it closes once the worker ends.
    bool requestClose()
    {
        if (!busy) return true;
        closeRequested = true;
        return false;
    }

    // A release reply may start an update or say "up to date" only for the
    // folder it was asked for, and only while nothing runs.
    bool releaseMayAct(unsigned askedGeneration) const
    {
        return !busy && askedGeneration == rootGeneration;
    }
};

} // namespace launcher_ui_state

#endif
