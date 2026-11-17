#pragma once

// The build update's interrupted-state machinery (307).  An update that dies
// between replacement steps leaves newer files mixed with old ones; the next
// start must notice and restore the saved build before anything launches or
// mutates the half-installed state.  Plain text and filesystem only, no
// windows: tests/launcher_progress/launcher_update_test.cc checks this part.

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace launcher_update {

namespace fs = std::filesystem;

constexpr const char* kBackupsFolder = "f2reimagined-backups";
// Written before the first copy of an update and removed after the last one:
// its existence says the game folder may hold a half-installed build, and its
// one line names the backup folder the restore comes from.
constexpr const char* kMarkerName = "update-in-progress.txt";
// One relative path per copied file, appended as the update copies; a
// restore removes exactly these when the backup does not carry them, so
// files the player put there themselves survive.
constexpr const char* kManifestName = "update-manifest.txt";

inline fs::path backupsRoot(const fs::path& gameRoot) { return gameRoot / kBackupsFolder; }
inline fs::path markerPath(const fs::path& gameRoot) { return backupsRoot(gameRoot) / kMarkerName; }

// The marker line: the backup folder's name, empty when there is no marker.
inline std::string readMarker(const fs::path& gameRoot)
{
    std::ifstream in(markerPath(gameRoot), std::ios::binary);
    if (!in) return std::string();
    std::string line;
    std::getline(in, line);
    return line;
}

inline bool writeMarker(const fs::path& gameRoot, const std::string& backupFolder)
{
    std::error_code error;
    fs::create_directories(backupsRoot(gameRoot), error);
    if (error) return false;
    // Temp file, then rename: a crash mid-write must not leave a truncated
    // marker naming a wrong backup (same shape as writeBuildVersion).
    fs::path marker = markerPath(gameRoot);
    fs::path temp = marker;
    temp += ".tmp";
    {
        std::ofstream out(temp, std::ios::binary | std::ios::trunc);
        if (!out) return false;
        out << backupFolder << "\n";
        out.close();
        if (!out) {
            std::error_code ignored;
            fs::remove(temp, ignored);
            return false;
        }
    }
    fs::rename(temp, marker, error);
    if (error) {
        std::error_code ignored;
        fs::remove(temp, ignored);
        return false;
    }
    return true;
}

// True while an interrupted update waits for its restore; the UI refuses to
// launch the game in that state, exactly like the RP recovery pending.
inline bool hasPendingUpdate(const fs::path& gameRoot)
{
    return !readMarker(gameRoot).empty();
}

inline void clearMarker(const fs::path& gameRoot)
{
    std::error_code ignored;
    fs::remove(markerPath(gameRoot), ignored);
    fs::remove(backupsRoot(gameRoot) / kManifestName, ignored);
}

// One copied file, appended the moment it lands, so a crash mid-tree still
// knows what was touched.  Empty path: nothing recorded.
inline void appendCopied(const fs::path& gameRoot, const std::string& relative)
{
    if (relative.empty()) return;
    std::ofstream out(backupsRoot(gameRoot) / kManifestName,
        std::ios::binary | std::ios::app);
    if (!out) return;  // without the journal the restore still copies the backup over
    out << relative << "\n";
}

inline std::vector<std::string> readCopied(const fs::path& gameRoot)
{
    std::ifstream in(backupsRoot(gameRoot) / kManifestName, std::ios::binary);
    std::vector<std::string> lines;
    std::string line;
    while (std::getline(in, line)) {
        while (!line.empty() && (line.back() == '\r' || line.back() == '\n')) line.pop_back();
        if (!line.empty()) lines.push_back(line);
    }
    return lines;
}

// The update copied `relative` (forward slashes, under the game root) over or
// into the game folder.  A restore removes it when the backup does not carry
// it; the backup's own copy comes back in the file pass afterwards.
inline bool removeCopiedFile(const fs::path& gameRoot, const std::string& relative,
    std::string* blockedFile)
{
    fs::path path = gameRoot / fs::u8path(relative);
    std::error_code error;
    if (fs::exists(path, error)) {
        fs::remove(path, error);
        if (error) {
            if (blockedFile != nullptr) *blockedFile = relative;
            return false;
        }
    }
    return true;
}

// A running exe cannot be overwritten but can be renamed: the old copy moves
// aside (deleted on the next run) and the new one takes its name.
inline bool copyLauncherFile(const fs::path& source, const fs::path& target)
{
    std::error_code error;
    fs::path aside = target;
    aside += ".old";
    fs::remove(aside, error);
    bool moved = false;
    if (fs::exists(target, error)) {
        fs::rename(target, aside, error);
        moved = !error;
    }
    fs::copy_file(source, target, fs::copy_options::overwrite_existing, error);
    if (error && moved) {
        std::error_code ignored;
        fs::rename(aside, target, ignored);
    }
    return !error;
}

// Copy one regular file of the saved build back into the game folder.  The
// launcher takes the rename-aside path (kRootLauncher in launcher_main.cc;
// the name is ASCII, and the backup folder names are sanitized ASCII too).
inline bool restoreBuildFile(const fs::path& backupFolder, const fs::path& gameRoot,
    const std::string& name, std::string* blockedFile)
{
    fs::path source = backupFolder / fs::u8path(name);
    std::error_code error;
    if (!fs::is_regular_file(source, error)) return true;  // the old backup did not carry it
    fs::path target = gameRoot / fs::u8path(name);
    bool ok = name == "Fallout 2 Reimagined.exe"
        ? copyLauncherFile(source, target)
        : (fs::copy_file(source, target, fs::copy_options::overwrite_existing, error), !error);
    if (!ok && blockedFile != nullptr) *blockedFile = name;
    return ok;
}

// Copy the saved build's asset tree back over the game's.  Files the backup
// does not carry are left alone here: only the journal knows which of them
// the interrupted update introduced, and the caller removes those first.
inline bool restoreBuildTree(const fs::path& backupFolder, const fs::path& gameRoot,
    const std::string& name, std::string* blockedFile)
{
    fs::path source = backupFolder / fs::u8path(name);
    std::error_code error;
    if (!fs::is_directory(source, error)) return true;  // the old backup did not carry it
    fs::copy(source, gameRoot / fs::u8path(name),
        fs::copy_options::recursive | fs::copy_options::overwrite_existing, error);
    if (error) {
        if (blockedFile != nullptr) *blockedFile = name;
        return false;
    }
    return true;
}

// The restore itself: journal files gone first (introduced ones die here,
// overwritten ones come back in the next pass), then every saved build file
// and asset tree copies back.  False carries the first blocked path in
// *blockedFile; the marker and the backup stay untouched then, so removing
// the obstruction and retrying finishes the recovery.
inline bool restoreSavedBuild(const fs::path& gameRoot, const fs::path& backupFolder,
    const std::vector<std::string>& buildFiles, const std::vector<std::string>& buildTrees,
    std::string* blockedFile)
{
    for (const std::string& relative : readCopied(gameRoot)) {
        if (!removeCopiedFile(gameRoot, relative, blockedFile)) return false;
    }
    for (const std::string& name : buildFiles) {
        if (!restoreBuildFile(backupFolder, gameRoot, name, blockedFile)) return false;
    }
    for (const std::string& name : buildTrees) {
        if (!restoreBuildTree(backupFolder, gameRoot, name, blockedFile)) return false;
    }
    return true;
}

// The backup a repair takes: first the one the interrupted update's marker
// names (that state is what needs repairing, whatever else sits there),
// and without a marker the newest backup whose version differs from the
// installed stamp.  Empty when there is nothing to take.  The version
// equals the stamp only for the marker's own backup, so an explicit
// rollback can repair the interrupted state instead of walking two
// versions back.
inline fs::path repairBackup(const fs::path& gameRoot, const std::string& currentVersion,
    std::string* selectedVersion)
{
    fs::path backupRoot = backupsRoot(gameRoot);
    std::string marker = readMarker(gameRoot);
    if (!marker.empty()) {
        fs::path named = backupRoot / fs::u8path(marker);
        std::error_code error;
        if (fs::is_regular_file(named / "version.txt", error)) {
            std::ifstream in(named / "version.txt");
            std::string version;
            std::getline(in, version);
            if (selectedVersion != nullptr) *selectedVersion = version;
            return named;
        }
    }
    std::error_code error;
    if (!fs::is_directory(backupRoot, error)) return fs::path();
    fs::path selected;
    fs::file_time_type selectedTime = fs::file_time_type::min();
    for (fs::directory_iterator it(backupRoot, error), end; !error && it != end; it.increment(error)) {
        if (!it->is_directory(error)) continue;
        std::ifstream versionFile(it->path() / "version.txt");
        if (!versionFile) continue;
        std::string version;
        std::getline(versionFile, version);
        while (!version.empty() && (version.back() == '\r' || version.back() == '\n'
            || version.back() == ' ' || version.back() == '\t')) version.pop_back();
        if (version.empty()) continue;
        if (version == currentVersion) continue;
        fs::file_time_type time = fs::last_write_time(it->path(), error);
        if (error) { error.clear(); continue; }
        if (selected.empty() || time > selectedTime) {
            selected = it->path();
            if (selectedVersion != nullptr) *selectedVersion = version;
            selectedTime = time;
        }
    }
    return selected;
}

} // namespace launcher_update
