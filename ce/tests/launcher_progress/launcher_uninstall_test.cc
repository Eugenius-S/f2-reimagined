#include "launcher_uninstall.h"

#include <cstdio>
#include <chrono>
#include <fstream>
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

    check(installedFiles().size() == 4 && installedFolders().size() == 3, "the removal lists");
    for (const std::string& name : installedFiles()) {
        check(name != "master.dat" && name != "critter.dat" && name != "patch000.dat" && name != "fallout2.exe"
                && name != "fallout2.cfg",
            "the player's and the original files are never on the list");
    }
    for (const std::string& name : installedFolders()) check(name != "data", "saves live in data and stay");

    namespace fs = std::filesystem;
    check(samePath(fs::path(L"C:/Games/Fallout 2"), fs::path(L"c:\\games\\fallout 2")),
        "game roots compare case-insensitively after path normalization");
    check(!samePath(fs::path(L"C:/Games/A"), fs::path(L"C:/Games/B")), "different game roots stay distinct");
    fs::path backups = fs::temp_directory_path() / "f2r-stale-backups-test";
    fs::remove_all(backups);
    for (const char* name : {"v1", "v2", "v3"}) fs::create_directories(backups / name);
    std::vector<fs::path> stale = staleBackups(backups, "v3");
    check(stale.size() == 2 && stale[0].filename() != "v3" && stale[1].filename() != "v3", "every backup but the kept one is stale");
    check(staleBackups(backups, "").empty(), "no backup made, nothing removed");
    check(staleBackups(backups / "missing", "v3").empty(), "no backup folder, nothing removed");
    fs::remove_all(backups);

    fs::path appData = fs::temp_directory_path() /
        ("f2r-settings-cleanup-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    fs::path settings = settingsDirectory(appData);
    fs::create_directories(settings);
    std::ofstream(settings / "launcher.ini") << "auto_updates=1\n";
    std::ofstream(settings / "keep.ini") << "keep\n";
    std::error_code error;
    removed = false;
    bool settingsRemovedOk = removeLauncherSettings(appData, &removed, &error);
    check(settingsRemovedOk && removed && !fs::exists(settings / "launcher.ini"), "launcher settings are removed");
    check(fs::exists(settings / "keep.ini") && fs::is_directory(settings),
        "unrelated AppData files and their directory remain");
    fs::remove(settings / "keep.ini", error);
    removed = true;
    check(removeLauncherSettings(appData, &removed, &error) && !removed && !fs::exists(settings),
        "only the now-empty launcher settings directory is removed");
    fs::remove(appData, error);

    std::printf("launcher_uninstall_tests: ok\n");
    return 0;
}
