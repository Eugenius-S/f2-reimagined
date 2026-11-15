#include "launcher_rp.h"

#include <cstdio>
#include <cstdlib>

using namespace launcher_rp;

static void check(bool ok, const char* what)
{
    if (!ok) {
        std::printf("FAILED: %s\n", what);
        std::exit(1);
    }
}

int main()
{
    check(targetOf("rpu-install.bat").empty() && targetOf("RPU-Install.sh").empty()
            && targetOf("rpu-install.command").empty(),
        "RPU's install scripts are not copied");
    check(targetOf("mods_order.txt") == "mods/mods_order.txt", "the mod order goes into mods/");
    check(targetOf("gl_ammomod.int") == "data/scripts/gl_ammomod.int", "the ammo script goes into data/scripts");
    check(targetOf("mods/rpu.dat") == "mods/rpu.dat" && targetOf("ddraw.ini") == "ddraw.ini", "the rest lands as is");

    check(keptInData("SAVEGAME/SLOT01/SAVE.DAT") && keptInData("sound/music/01hub.acm"), "saves and music stay");
    check(keptInData("art/splash/splash0.rix") && keptInData("art/russian/splash/splash0.rix"), "this mod's splash art stays");
    check(keptInData("worldmap.txt") && keptInData("vault13.gcd"), "data/*.txt and data/*.gcd stay");
    check(!keptInData("maps/artemple.map") && !keptInData("proto/items/00000001.pro")
            && !keptInData("text/english/game/misc.msg") && !keptInData("sound/sfx/a.acm"),
        "other loose vanilla files go aside");
    check(!keptInData("text/notes.txt"), "only data/*.txt at the top stays");

    check(movedAside("patch000.dat") && movedAside("F2_RES.DAT"), "the old patch and hi-res archives go aside");
    check(!movedAside("master.dat") && !movedAside("critter.dat"), "the base archives stay");
    std::printf("launcher_rp_tests: ok\n");
    return 0;
}
