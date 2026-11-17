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
    // material, so removing the obstruction lets a retry finish it (I2).
    writeMarker(root, "v1");
    fs::create_directories(root / "ce.dat");  // a directory where a file must stand
    check(!restoreSavedBuild(root, backup, files, trees, &blocked), "the restore fails on the blocked path");
    check(blocked == "ce.dat", "the blocked file is named");
    check(readMarker(root) == "v1", "the marker stays so the next start retries");
    fs::remove_all(root);

    // The marker is atomic: a second write replaces the first whole, and no
    // temporary file survives either write (Opus audit, marker shape).
    check(writeMarker(root, "v0") && readMarker(root) == "v0", "the second marker wins");
    check(!fs::exists(backupsRoot(root) / "update-in-progress.txt.tmp"), "no marker temp file is left");
    check(hasPendingUpdate(root), "a standing marker says an update is pending");
    clearMarker(root);
    check(!hasPendingUpdate(root), "a cleared marker says nothing is pending");
    fs::remove_all(root / "f2reimagined-backups");

    // Repair selection (B1): without a marker, a backup whose version equals
    // the installed stamp is skipped; with the marker, exactly the named
    // backup wins even over a newer one.
    fs::path backups = root / "f2reimagined-backups";
    put(backups / "v1" / "version.txt", "v1\n");
    put(backups / "v0" / "version.txt", "v0\n");
    std::string version;
    check(repairBackup(root, "v1", &version).filename() == "v0" && version == "v0",
        "no marker: the same-version backup is skipped, the different one is taken");
    writeMarker(root, "v1");
    check(repairBackup(root, "v1", &version).filename() == "v1" && version == "v1",
        "a marker names its own backup even against a newer one");
    check(repairBackup(root, "v0", &version).filename() == "v1",
        "the marker's backup wins whatever the installed stamp is");
    clearMarker(root);
    check(repairBackup(root, "v1", &version).empty() == false, "a plain rollback still finds the older build");

    // R2, the retry trap: the backup bytes never change during a restore.
    std::string backupExe = get(backups / "v1" / "version.txt");
    put(root / "fallout2-ce.exe", "new-exe");
    writeMarker(root, "v1");
    appendCopied(root, "fallout2-ce.exe");
    check(restoreSavedBuild(root, backups / "v1", files, trees, &blocked), "the restore succeeds");
    check(get(backups / "v1" / "version.txt") == backupExe, "the backup stays byte for byte identical");

    fs::remove_all(root);

    std::printf("launcher_update_tests: ok\n");
    return 0;
}
