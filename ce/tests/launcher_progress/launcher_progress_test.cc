#include "launcher_progress.h"

#include <cstdio>
#include <cstdlib>

using namespace launcher_progress;

static void check(bool ok, const char* what)
{
    if (!ok) {
        std::printf("FAILED: %s\n", what);
        std::exit(1);
    }
}

int main()
{
    Tracker t;
    t.feed("  ... 5 / 20 MB");
    check(t.value == 2, "download 5/20 maps to 2");
    t.feed("  ... 20 / 20 MB");
    check(t.value == 10, "download ends at 10");
    t.feed("  ... 3 / 20 MB");
    check(t.value == 10, "the bar never goes back");
    t.feed("{\"maps_total\": 2, \"scenes_total\": 4}");
    check(t.hideLine && t.mapsTotal == 2 && t.scenesTotal == 4, "totals read");
    t.feed("{\"fingerprint_sha256\": \"x\", \"output\": \"g\\\\cache\\\\klamall\\\\0\"}");
    check(t.hideLine && t.finishedMap.empty() && t.value == 34, "first scene: 12 + 88/4");
    t.feed("{\"current\": true, \"output\": \"g\\\\cache\\\\klamall\\\\1\"}");
    check(t.finishedMap.empty(), "map is open until another starts");
    t.feed("FAILED denbus1 e0: boom");
    check(t.finishedMap == "klamall" && t.finishedResult == MapResult::Built && t.mapsDone == 1, "klamall built");
    t.feed("{\"current\": true, \"output\": \"C:/g/cache/denbus1/1\"}");
    check(t.finishedMap == "denbus1" && t.finishedResult == MapResult::Failed && t.mapsDone == 2, "last map closes at the total");
    check(t.value == 99, "stays under 100 until the worker is done");
    t.feed("FAILED nomap: no such map");
    check(t.scenesDone == 4, "a map that never loaded is not a scene");
    t.feed("some other line");
    check(!t.hideLine, "ordinary lines stay in the log");
    std::printf("launcher_progress_tests: ok\n");
    return 0;
}
