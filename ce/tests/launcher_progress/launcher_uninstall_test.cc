#include "launcher_uninstall.h"

#include <cstdio>
#include <cstdlib>

using namespace launcher_uninstall;

static void check(bool ok, const char* what)
{
    if (!ok) {
        std::printf("FAILED: %s\n", what);
        std::exit(1);
    }
}

int main()
{
    bool removed = false;
    std::string order = "rpu.dat\r\nRPU_Russian.dat\r\nother.dat\r\n";
    check(removeLine(order, "rpu_russian.dat", &removed) == "rpu.dat\r\nother.dat\r\n" && removed,
        "the translation line goes, the others and their endings stay");
    check(removeLine("rpu.dat\r\n", "rpu_russian.dat", &removed) == "rpu.dat\r\n" && !removed, "no line, no change");
    check(removeLine("rpu_russian.dat", "rpu_russian.dat").empty(), "last line without an ending");
    check(!hasContent(removeLine("rpu_russian.dat\r\n\r\n", "rpu_russian.dat")), "only blanks left means an empty list");
    check(hasContent("a.dat\r\n"), "a real line is content");

    std::string cfg = "[system]\r\nlanguage=russian\r\nmusic=1\r\n[other]\r\nlanguage=keep\r\n";
    bool found = false;
    std::string english = withLanguage(cfg, "english", &found);
    check(found && english == "[system]\r\nlanguage=english\r\nmusic=1\r\n[other]\r\nlanguage=keep\r\n",
        "only [system] language changes");
    withLanguage("[other]\r\nlanguage=x\r\n", "english", &found);
    check(!found, "no [system] language, nothing found");
    check(languageOf(cfg) == "russian", "language read from [system]");
    check(languageOf("[other]\nlanguage=russian\n").empty(), "other sections are not the game language");

    check(installedFiles().size() == 4 && installedFolders().size() == 2, "the removal lists");
    for (const std::string& name : installedFiles()) {
        check(name != "master.dat" && name != "critter.dat" && name != "patch000.dat" && name != "fallout2.exe"
                && name != "fallout2.cfg",
            "the player's and the original files are never on the list");
    }
    check(installedFolders()[0] != "data" && installedFolders()[1] != "data", "saves live in data and stay");

    std::printf("launcher_uninstall_tests: ok\n");
    return 0;
}
