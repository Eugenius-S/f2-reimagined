// The interrupted-update machinery (307): marker, intended set, receipt,
// restore.  The hash is a stub (the file's own bytes in hex), so the
// receipt's match and mismatch cases run without Windows crypto.

#include "launcher_update.h"

#include <chrono>
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

// A stub whole-file hash: every byte's two hex digits joined, so one changed
// byte changes the hash and a copied file always matches.
static bool stubHash(const fs::path& path, std::string* hex)
{
    std::ifstream in(path, std::ios::binary);
    if (!in) return false;
    static const char* digits = "0123456789abcdef";
    std::string out;
    char byte;
    while (in.get(byte)) {
        unsigned char value = static_cast<unsigned char>(byte);
        out += digits[value >> 4];
        out += digits[value & 15];
    }
    *hex = out;
    return true;
}

int main()
{
    // The marker names the backup an interrupted update restores from; its
    // file's existence is the pending state, whatever the file says.
    fs::path root = fs::temp_directory_path() / "f2r-update-test";
    fs::remove_all(root);
    fs::create_directories(root);
    check(!markerExists(root) && !hasPendingUpdate(root), "no marker, no pending update");
    check(writeMarker(root, "v1"), "the marker is written");
    check(readMarker(root) == "v1" && hasPendingUpdate(root), "the marker names the backup");
    // Atomic replacement: a second write over a standing marker wins whole.
    check(writeMarker(root, "v0") && readMarker(root) == "v0", "the second marker replaces the first");
    check(!fs::exists(backupsRoot(root) / "update-in-progress.txt.tmp"), "no marker temp file is left");
    // A corrupt (empty) marker is a pending state, not a clean one.
    put(markerPath(root), "");
    check(hasPendingUpdate(root) && readMarker(root).empty(), "an empty marker stays pending");
    check(clearMarker(root), "the marker clears");
    check(!markerExists(root), "the marker is gone after the clear");

    // The intended set goes down whole before the first copy, and reads back
    // with its replaced/introduced marks.
    check(writeIntendedSet(root, {{"fallout2-ce.exe", true}, {"data/art/splash/new.png", false}}),
        "the intended set is written");
    check(readIntendedSet(root).size() == 2 && readIntendedSet(root)[0].existed
        && !readIntendedSet(root)[1].existed, "the intended set keeps its marks");
    check(!fs::exists(backupsRoot(root) / "update-manifest.txt.tmp"), "no manifest temp file is left");
    // A damaged or absent journal says the touched set is unknown, not
    // empty: with a marker standing, recovery blocks on all of these.
    writeMarker(root, "v1");  // corruption only matters while an update pends
    check(!intendedSetCorrupt(root), "a good journal is not corruption");
    put(backupsRoot(root) / "update-manifest.txt", "Y a.png\ngarbage\n");
    check(intendedSetCorrupt(root), "a corrupt journal blocks the restore");
    put(backupsRoot(root) / "update-manifest.txt", "");
    check(intendedSetCorrupt(root), "an empty journal file is an unknown set, not an empty transaction");
    put(backupsRoot(root) / "update-manifest.txt", "Y a.png\n");
    check(intendedSetCorrupt(root), "a valid prefix cut at EOF has no END count");
    put(backupsRoot(root) / "update-manifest.txt", "Y a.png\nEND 2\n");
    check(intendedSetCorrupt(root), "an END count that disagrees with the entries");
    put(backupsRoot(root) / "update-manifest.txt", "Y a.png\nEND 1\njunk\n");
    check(intendedSetCorrupt(root), "anything after the END line breaks the transaction");
    // 312: the END count is the whole token, plain decimal digits only.
    for (const char* bad : { "1junk", "1 ", " 1", "+1", "-1", "1.0", "0x1", "", "18446744073709551617",
             "99999999999999999999999" }) {
        put(backupsRoot(root) / "update-manifest.txt", std::string("Y a.png\nEND ") + bad + "\n");
        check(intendedSetCorrupt(root), (std::string("a malformed END count is refused: [") + bad + "]").c_str());
    }
    put(backupsRoot(root) / "update-manifest.txt", "Y a.png\r\nEND 1\r\n");
    check(!intendedSetCorrupt(root), "a CRLF journal with a plain END count stays good");
    std::error_code journalError;
    fs::remove_all(backupsRoot(root) / "update-manifest.txt", journalError);
    check(intendedSetCorrupt(root), "a missing journal under a standing marker is an unknown set");
    check(clearMarker(root), "the clear takes the journal with it");
    check(readIntendedSet(root).empty(), "the journal is gone");
    check(!intendedSetCorrupt(root), "no marker, no journal, nothing pending");

    // An interrupted update: the marker and the intended set stand, the game
    // folder holds new and old files mixed, and the restore puts the saved
    // build back.  The replaced launcher file (Y) is restored through the
    // rename-aside path, the introduced file (N) is removed, and the
    // player's own unlisted files survive.
    fs::path backup = root / "v1";
    put(backup / "fallout2-ce.exe", "old-exe");
    put(backup / "ce.dat", "old-dat");
    put(backup / "version.txt", "v1\n");
    put(backup / "Fallout 2 Reimagined.exe", "old-launcher");
    put(backup / "user-assets/fullscreen-20261028/wallpapers/art.png", "old-art");
    std::vector<std::string> files = {"fallout2-ce.exe", "ce.dat", "version.txt", "Fallout 2 Reimagined.exe"};
    std::vector<std::string> trees = {"user-assets"};
    check(writeBackupReceipt(backup, files, stubHash, nullptr), "the receipt is written");
    put(root / "fallout2-ce.exe", "new-exe");                                     // replaced (Y)
    put(root / "Fallout 2 Reimagined.exe", "old-launcher");                       // replaced (Y)
    put(root / "ce.dat", "old-dat");                                              // not reached yet
    put(root / "version.txt", "v2\n");                                            // not written yet
    put(root / "user-assets/fullscreen-20261028/wallpapers/art.png", "new-art");  // replaced (Y)
    put(root / "user-assets/fullscreen-20261028/wallpapers/introduced.png", "new"); // introduced (N)
    put(root / "user-assets/fullscreen-20261028/wallpapers/player.png", "mine");  // the player's own file
    writeMarker(root, "v1");
    writeIntendedSet(root, {{"fallout2-ce.exe", true}, {"Fallout 2 Reimagined.exe", true},
        {"user-assets/fullscreen-20261028/wallpapers/art.png", true},
        {"user-assets/fullscreen-20261028/wallpapers/introduced.png", false}});

    // The receipt holds: the backup is known-good before any destructive step.
    std::string bad;
    check(validateBackup(backup, files, stubHash, &bad),
        "the fresh backup validates against its receipt");

    std::string blocked;
    check(restoreSavedBuild(root, backup, files, trees, &blocked), "the restore succeeds");
    check(blocked.empty(), "nothing is blocked");
    // restoreSavedBuild repairs the files only: the caller decides when the
    // interrupted state (marker and journal) may go, so a failed caller can
    // still retry; the recovery clears it after a whole restore.
    check(readMarker(root) == "v1", "the restore itself leaves the marker standing");
    clearMarker(root);
    check(!markerExists(root) && readIntendedSet(root).empty(), "the recovery's clear removes the whole state");
    check(get(root / "fallout2-ce.exe") == "old-exe", "the overwritten exe comes back whole");
    check(get(root / "Fallout 2 Reimagined.exe") == "old-launcher",
        "the replaced launcher comes back through the rename-aside path, not by removing it first");
    check(get(root / "ce.dat") == "old-dat", "the untouched file stays");
    check(get(root / "version.txt") == "v1\n", "the version stays old");
    check(get(root / "user-assets/fullscreen-20261028/wallpapers/art.png") == "old-art",
        "the overwritten art comes back from the backup");
    check(!fs::exists(root / "user-assets/fullscreen-20261028/wallpapers/introduced.png"),
        "the file the update introduced is gone");
    check(get(root / "user-assets/fullscreen-20261028/wallpapers/player.png") == "mine",
        "the player's own file survives the restore byte for byte");

    // A restore that is blocked reports the file and keeps the recovery
    // material; removing the obstruction and retrying finishes it (I2).
    writeMarker(root, "v1");
    std::error_code error;
    fs::remove(root / "ce.dat", error);
    fs::create_directories(root / "ce.dat");  // a directory where a file must stand
    check(!restoreSavedBuild(root, backup, files, trees, &blocked), "the restore fails on the blocked path");
    check(blocked == "ce.dat", "the blocked file is named");
    check(readMarker(root) == "v1", "the marker stays so the next start retries");
    fs::remove_all(root / "ce.dat", error);
    check(restoreSavedBuild(root, backup, files, trees, &blocked), "the retry finishes the restore");
    clearMarker(root);  // the repair is done; the selection tests below start clean

    // A corrupted backup is refused before any destructive step: the receipt
    // names the first file whose bytes no longer match, and the caller never
    // restores from a backup that failed validation.
    put(backup / "ce.dat", "tampered");
    check(!validateBackup(backup, files, stubHash, &bad) && bad == "ce.dat",
        "the receipt catches the tampered file");
    check(get(root / "fallout2-ce.exe") == "old-exe",
        "nothing was touched while validation refused");
    put(backup / "ce.dat", "old-dat");
    check(validateBackup(backup, files, stubHash, &bad), "the restored bytes validate again");

    // A backup from before this contract carries no receipt and promises
    // nothing about completeness: refused, never presented as known-good.
    fs::path legacy = root / "v0";
    put(legacy / "version.txt", "v0\n");
    check(!validateBackup(legacy, files, stubHash, &bad) && bad == "f2r-backup-receipt.txt",
        "a receipt-less backup is refused, not trusted on its stamp");

    // A receipt that does not account for every required file is refused:
    // hashing listed bytes is not completeness.
    fs::path partial = root / "v2";
    put(partial / "version.txt", "v2\n");
    put(partial / "ce.dat", "old-dat");
    std::vector<std::string> partialFiles = {"version.txt", "ce.dat"};
    check(writeBackupReceipt(partial, partialFiles, stubHash, nullptr), "the partial receipt is written");
    check(!validateBackup(partial, files, stubHash, &bad) && bad == "fallout2-ce.exe",
        "a receipt that skips a required file is refused");
    // An unlisted extra file in the folder makes the receipt's promise wrong.
    put(partial / "fallout2-ce.exe", "old-exe");
    check(writeBackupReceipt(partial, files, stubHash, nullptr), "the full receipt is rewritten");
    put(partial / "unlisted.dat", "sneaky");
    check(!validateBackup(partial, files, stubHash, &bad) && bad == "unlisted.dat",
        "an unlisted file breaks the receipt's promise");
    // A truncated or empty receipt promises an unknown inventory.
    put(partial / "f2r-backup-receipt.txt", "abc123");
    check(!validateBackup(partial, files, stubHash, &bad) && bad == "f2r-backup-receipt.txt",
        "a truncated receipt is refused");
    put(partial / "f2r-backup-receipt.txt", "");
    check(!validateBackup(partial, files, stubHash, &bad), "an empty receipt is refused");

    // Repair selection (B1): without a marker, a backup whose version equals
    // the installed stamp is skipped; with the marker, exactly the named
    // backup wins even over a newer one.  The mtimes are set explicitly so
    // the newest-wins rule never rides on directory creation order.
    fs::path backups = root / "f2reimagined-backups";
    put(backups / "v1" / "version.txt", "v1\n");
    put(backups / "v0" / "version.txt", "v0\n");
    auto now = fs::file_time_type::clock::now();
    std::error_code timeError;
    fs::last_write_time(backups / "v1", now - std::chrono::hours(2), timeError);
    fs::last_write_time(backups / "v0", now - std::chrono::hours(1), timeError);
    std::string version;
    check(repairBackup(root, "v1", &version).filename() == "v0" && version == "v0",
        "no marker: the same-version backup is skipped, the different one is taken");
    writeMarker(root, "v1");
    check(repairBackup(root, "v1", &version).filename() == "v1" && version == "v1",
        "a marker names its own backup even against a newer one");
    check(repairBackup(root, "v0", &version).filename() == "v1",
        "the marker's backup wins whatever the installed stamp is");
    clearMarker(root);
    check(!repairBackup(root, "v1", &version).empty(), "a plain rollback still finds the older build");

    fs::remove_all(root);

    std::printf("launcher_update_tests: ok\n");
    return 0;
}
