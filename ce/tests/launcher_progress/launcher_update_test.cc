// The interrupted-update machinery (307): marker, journal, restore.

#include "launcher_update.h"

#include <cstdio>
#include <cstdlib>
#include <fstream>

using namespace launcher_update;

namespace fs = std::filesystem;

static void check(bool ok, const char* what)
{
    if (!ok) {
        std::printf("FAILED: %s\n", what);
        std::exit(1);
    }
}

static void put(const fs::path& path, const std::string& text)
{
    std::error_code error;
    fs::create_directories(path.parent_path(), error);
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    out << text;
}

static std::string get(const fs::path& path)
{
    std::ifstream in(path, std::ios::binary);
    return std::string((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
}

int main()
{
    // The marker names the backup an interrupted update restores from.
    fs::path root = fs::temp_directory_path() / "f2r-update-test";
    fs::remove_all(root);
    fs::create_directories(root);
    check(readMarker(root).empty(), "no marker, no interrupted update");
    check(writeMarker(root, "v1"), "the marker is written");
    check(readMarker(root) == "v1", "the marker names the backup");
    clearMarker(root);
    check(readMarker(root).empty(), "the marker is gone after the commit");

    // The journal lists every file the update touched, in order.
    appendCopied(root, "fallout2-ce.exe");
    appendCopied(root, "user-assets/fullscreen-20261028/wallpapers/art.png");
    check(readCopied(root).size() == 2 && readCopied(root)[0] == "fallout2-ce.exe",
        "the journal keeps the copied files");
    clearMarker(root);
    check(readCopied(root).empty(), "the journal dies with the marker");

    // An update interrupted between replacement steps: the marker and the
    // journal stand, the game folder holds new and old files mixed, and the
    // restore puts the saved build back without touching the player's own
    // files.
    fs::path backup = root / "v1";
    put(backup / "fallout2-ce.exe", "old-exe");
    put(backup / "ce.dat", "old-dat");
    put(backup / "version.txt", "v1\n");
    put(backup / "user-assets/fullscreen-20261028/wallpapers/art.png", "old-art");
    put(root / "fallout2-ce.exe", "new-exe");                                   // overwritten
    put(root / "ce.dat", "old-dat");                                            // not reached yet
    put(root / "version.txt", "v2\n");                                          // not written yet
    put(root / "user-assets/fullscreen-20261028/wallpapers/art.png", "new-art"); // overwritten
    put(root / "user-assets/fullscreen-20261028/wallpapers/introduced.png", "new"); // the package introduced it
    put(root / "user-assets/fullscreen-20261028/wallpapers/player.png", "mine"); // the player's own file
    writeMarker(root, "v1");
    appendCopied(root, "fallout2-ce.exe");
    appendCopied(root, "user-assets/fullscreen-20261028/wallpapers/art.png");
    appendCopied(root, "user-assets/fullscreen-20261028/wallpapers/introduced.png");

    std::vector<std::string> files = {"fallout2-ce.exe", "ce.dat", "version.txt"};
    std::vector<std::string> trees = {"user-assets"};
    std::string blocked;
    check(restoreSavedBuild(root, backup, files, trees, &blocked), "the restore succeeds");
    check(blocked.empty(), "nothing is blocked");
    check(readMarker(root).empty() && readCopied(root).empty(), "the restore leaves no interrupted state to redo");
    check(get(root / "fallout2-ce.exe") == "old-exe", "the overwritten exe comes back whole");
    check(get(root / "ce.dat") == "old-dat", "the untouched file stays");
    check(get(root / "version.txt") == "v1\n", "the version stays old");
    check(get(root / "user-assets/fullscreen-20261028/wallpapers/art.png") == "old-art",
        "the overwritten art comes back from the backup");
    check(!fs::exists(root / "user-assets/fullscreen-20261028/wallpapers/introduced.png"),
        "the file the package introduced is gone");
    check(get(root / "user-assets/fullscreen-20261028/wallpapers/player.png") == "mine",
        "the player's own file survives the restore byte for byte");

    // A restore that is blocked reports the file and keeps the recovery
    // material, so removing the obstruction lets a retry finish it.
    writeMarker(root, "v1");
    fs::create_directories(root / "ce.dat");  // a directory where a file must stand
    check(!restoreSavedBuild(root, backup, files, trees, &blocked), "the restore fails on the blocked path");
    check(blocked == "ce.dat", "the blocked file is named");
    check(readMarker(root) == "v1", "the marker stays so the next start retries");
    fs::remove_all(root);

    std::printf("launcher_update_tests: ok\n");
    return 0;
}
