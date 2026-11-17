#pragma once

// The build update's interrupted-state machinery (307).  An update that dies
// between replacement steps leaves newer files mixed with old ones; the next
// start must notice and restore the saved build before anything launches or
// mutates the half-installed state.  Plain text and filesystem only, no
// windows: tests/launcher_progress/launcher_update_test.cc checks this part
// with a pluggable hash, and the worker plugs its sha256File in.
//
// The safety contract, in the order the update runs:
//   1. The backup is taken and finished with a receipt: one sha256 and size
//      per file it carries.  A version-named backup with a valid receipt is
//      never rebuilt, so a retry can never capture a mixed state over it.
//   2. The marker names that backup and goes through temp+rename; its file's
//      mere existence means an update is pending, whatever it says, so the
//      UI refuses to launch on a corrupt marker too.
//   3. The journal is the intended touched set, written and flushed before
//      the first copy: one relative path per line, prefixed with Y (the file
//      stood in the game folder and will be replaced) or N (the update
//      introduces it).  A crash between a copy and any bookkeeping cannot
//      lose a path any more, and a replaced file is restored rather than
//      removed, so the running launcher survives its own restore.
//   4. The restore validates the receipt before any destructive step, then
//      removes exactly the N paths, then copies the backup's files and trees
//      back.  A blocked restore names the file and keeps the marker and the
//      backup; removing the obstruction and retrying finishes it.
//   5. The marker clears first, with its removal checked; the journal is
//      deleted only after a successful marker transition.

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <functional>
#include <string>
#include <vector>

namespace launcher_update {

namespace fs = std::filesystem;

constexpr const char* kBackupsFolder = "f2reimagined-backups";
// Written before the first copy of an update and removed after the last one:
// its existence says the game folder may hold a half-installed build, and its
// one line names the backup folder the restore comes from.
constexpr const char* kMarkerName = "update-in-progress.txt";
// The intended touched set, written before the first copy (see 3 above).
constexpr const char* kManifestName = "update-manifest.txt";
// One sha256 and size per file of a finished backup; its presence and match
// is what makes a backup known-good (4 above).
constexpr const char* kReceiptName = "f2r-backup-receipt.txt";

inline fs::path backupsRoot(const fs::path& gameRoot) { return gameRoot / kBackupsFolder; }
inline fs::path markerPath(const fs::path& gameRoot) { return backupsRoot(gameRoot) / kMarkerName; }

// The marker's file exists: an update is pending, whatever the file says.
// An empty or unreadable marker is a pending state that recovery must block
// on, not a clean state to sail past.
inline bool markerExists(const fs::path& gameRoot)
{
    std::error_code error;
    return fs::exists(markerPath(gameRoot), error) && !error;
}

inline bool hasPendingUpdate(const fs::path& gameRoot) { return markerExists(gameRoot); }

// The marker line: the backup folder's name, empty when absent or unreadable
// (the caller treats an empty line of an existing marker as blocked).
inline std::string readMarker(const fs::path& gameRoot)
{
    std::error_code error;
    if (!fs::exists(markerPath(gameRoot), error) || error) return std::string();
    std::ifstream in(markerPath(gameRoot), std::ios::binary);
    if (!in) return std::string();
    std::string line;
    std::getline(in, line);
    while (!line.empty() && (line.back() == '\r' || line.back() == '\n'
        || line.back() == ' ' || line.back() == '\t')) line.pop_back();
    return line;
}

// Temp file, then rename: a crash mid-write must not leave a truncated
// marker naming a wrong backup (same shape as writeBuildVersion).
inline bool writeMarker(const fs::path& gameRoot, const std::string& backupFolder)
{
    std::error_code error;
    fs::create_directories(backupsRoot(gameRoot), error);
    if (error) return false;
    fs::path marker = markerPath(gameRoot);
    fs::path temp = marker;
    temp += ".tmp";
    {
        std::ofstream out(temp, std::ios::binary | std::ios::trunc);
        if (!out) return false;
        out << backupFolder << "\n";
        out.flush();
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

// The marker goes first, and its removal is checked: while it stands, every
// start knows the build may be half-installed.  The journal is deleted only
// after the marker is really gone, so an interrupted transition keeps the
// receipt that recovery needs.  False: nothing was removed, the caller must
// not report the update committed.
inline bool clearMarker(const fs::path& gameRoot)
{
    std::error_code error;
    fs::remove(markerPath(gameRoot), error);
    if (error) return false;
    fs::remove(backupsRoot(gameRoot) / kManifestName, error);
    return !error;
}

// One path the update intends to touch: replaced (the file stood in the game
// folder) or introduced (the update creates it).
struct PendingPath {
    std::string relative;
    bool existed = false;
};

// The intended set goes down whole, through temp+rename, before the first
// copy, and ends with an "END count" line: a valid Y/N prefix that stops at
// EOF is a truncation, not a complete transaction.
inline bool writeIntendedSet(const fs::path& gameRoot, const std::vector<PendingPath>& paths)
{
    std::error_code error;
    fs::create_directories(backupsRoot(gameRoot), error);
    if (error) return false;
    fs::path manifest = backupsRoot(gameRoot) / kManifestName;
    fs::path temp = manifest;
    temp += ".tmp";
    {
        std::ofstream out(temp, std::ios::binary | std::ios::trunc);
        if (!out) return false;
        for (const PendingPath& path : paths) {
            out << (path.existed ? "Y " : "N ") << path.relative << "\n";
        }
        out << "END " << paths.size() << "\n";
        out.flush();
        out.close();
        if (!out) {
            std::error_code ignored;
            fs::remove(temp, ignored);
            return false;
        }
    }
    fs::rename(temp, manifest, error);
    if (error) {
        std::error_code ignored;
        fs::remove(temp, ignored);
        return false;
    }
    return true;
}

inline std::vector<PendingPath> readIntendedSet(const fs::path& gameRoot)
{
    std::ifstream in(backupsRoot(gameRoot) / kManifestName, std::ios::binary);
    std::vector<PendingPath> paths;
    std::string line;
    while (std::getline(in, line)) {
        while (!line.empty() && (line.back() == '\r' || line.back() == '\n')) line.pop_back();
        if (line.rfind("END ", 0) == 0) break;
        if (line.size() < 3 || (line[0] != 'Y' && line[0] != 'N') || line[1] != ' ') continue;
        paths.push_back({line.substr(2), line[0] == 'Y'});
    }
    return paths;
}

// True when the intended set cannot be established: the manifest is missing,
// unreadable, malformed, carries no END count, or its count disagrees with
// the entries it lists.  While the marker stands, a missing manifest is an
// unknown touched set (the update died somewhere between the marker and its
// journal, or the journal was lost), and recovery must block instead of
// guessing.  The worker journals at least one path, so an entry-less
// manifest is corruption too.
inline bool intendedSetCorrupt(const fs::path& gameRoot)
{
    if (!markerExists(gameRoot)) return false;  // nothing pending, nothing to establish
    std::error_code error;
    fs::path manifest = backupsRoot(gameRoot) / kManifestName;
    if (!fs::exists(manifest, error) || error) return true;
    std::ifstream in(manifest, std::ios::binary);
    if (!in) return true;  // exists but unreadable: the set is unknown
    size_t entries = 0;
    bool endSeen = false;
    size_t endCount = 0;
    bool endCountOk = false;
    bool trailing = false;
    std::string line;
    while (std::getline(in, line)) {
        while (!line.empty() && (line.back() == '\r' || line.back() == '\n')) line.pop_back();
        if (line.empty()) continue;
        if (endSeen) {
            // Nothing may follow the END line but blank padding.
            trailing = true;
            break;
        }
        if (line.rfind("END ", 0) == 0) {
            endSeen = true;
            try {
                endCount = static_cast<size_t>(std::stoull(line.substr(4)));
                endCountOk = true;
            } catch (...) {
                endCountOk = false;
            }
            continue;
        }
        if (line.size() < 3 || (line[0] != 'Y' && line[0] != 'N') || line[1] != ' '
            || line.substr(2).find_first_not_of(' ') == std::string::npos) {
            return true;
        }
        entries++;
    }
    if (!endSeen || !endCountOk || trailing || entries != endCount || entries == 0) return true;
    return false;
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
    if (!fs::is_regular_file(source, error)) return true;  // the caller's receipt check names this
    fs::path target = gameRoot / fs::u8path(name);
    bool ok = name == "Fallout 2 Reimagined.exe"
        ? copyLauncherFile(source, target)
        : (fs::copy_file(source, target, fs::copy_options::overwrite_existing, error), !error);
    if (!ok && blockedFile != nullptr) *blockedFile = name;
    return ok;
}

// Copy the saved build's asset tree back over the game's.  Files the backup
// does not carry stay alone: only the intended set's introduced paths are
// removed, and the caller did that before this pass.
inline bool restoreBuildTree(const fs::path& backupFolder, const fs::path& gameRoot,
    const std::string& name, std::string* blockedFile)
{
    fs::path source = backupFolder / fs::u8path(name);
    std::error_code error;
    if (!fs::is_directory(source, error)) return true;  // the caller's receipt check names this
    fs::copy(source, gameRoot / fs::u8path(name),
        fs::copy_options::recursive | fs::copy_options::overwrite_existing, error);
    if (error) {
        if (blockedFile != nullptr) *blockedFile = name;
        return false;
    }
    return true;
}

// The receipt: the backup's checked inventory.  One "sha256 size path" line
// per carried file (forward slashes), then one "absent path" line per
// required build file the machine did not have when the backup was taken.
// Written after the last copy, so a partial backup never carries one; its
// completeness check is what makes a restore refuse a truncated or
// hand-made backup instead of restoring "what little exists".
// `hash` is the worker's whole-file sha256 (or a test stub).
using HashFn = std::function<bool(const fs::path&, std::string*)>;

inline bool writeBackupReceipt(const fs::path& backupFolder, const std::vector<std::string>& requiredFiles,
    const HashFn& hash, std::string* blockedFile)
{
    std::vector<fs::path> files;
    std::error_code error;
    for (fs::recursive_directory_iterator it(backupFolder, fs::directory_options::skip_permission_denied, error), end;
         !error && it != end; it.increment(error)) {
        if (!it->is_regular_file(error) && error) break;
        if (it->is_regular_file(error)) {
            if (it->path().filename().string() == kReceiptName) continue;
            files.push_back(it->path());
        }
    }
    if (error) {
        if (blockedFile != nullptr) *blockedFile = kReceiptName;
        return false;
    }
    std::sort(files.begin(), files.end());
    fs::path receipt = backupFolder / kReceiptName;
    fs::path temp = receipt;
    temp += ".tmp";
    {
        std::ofstream out(temp, std::ios::binary | std::ios::trunc);
        if (!out) return false;
        for (const fs::path& file : files) {
            std::string hex;
            if (!hash(file, &hex)) {
                if (blockedFile != nullptr) *blockedFile = fs::relative(file, backupFolder).generic_string();
                std::error_code ignored;
                fs::remove(temp, ignored);
                return false;
            }
            out << hex << " " << static_cast<unsigned long long>(fs::file_size(file)) << " "
                << fs::relative(file, backupFolder).generic_string() << "\n";
        }
        // The required inventory: a build file the machine did not have is
        // recorded absent, so validation can tell a whole snapshot from a
        // truncated one.
        std::vector<std::string> listed;
        for (const fs::path& file : files) {
            listed.push_back(fs::relative(file, backupFolder).generic_string());
        }
        std::sort(listed.begin(), listed.end());
        for (const std::string& required : requiredFiles) {
            if (!std::binary_search(listed.begin(), listed.end(), required)) {
                out << "absent " << required << "\n";
            }
        }
        out.flush();
        out.close();
        if (!out) {
            std::error_code ignored;
            fs::remove(temp, ignored);
            return false;
        }
    }
    fs::rename(temp, receipt, error);
    if (error) {
        std::error_code ignored;
        fs::remove(temp, ignored);
        return false;
    }
    return true;
}

// Is this backup known-good?  A receipt must exist, be readable to its last
// line, list every file the backup carries with matching bytes, and account
// for every required build file (carried and checked, or recorded absent
// when the machine did not have it).  A backup from before this contract
// has no receipt and promises nothing: it is refused, never presented as
// known-good.  False names the first bad path; the caller refuses to
// restore and keeps the material.
inline bool validateBackup(const fs::path& backupFolder, const std::vector<std::string>& requiredFiles,
    const HashFn& hash, std::string* blockedFile)
{
    std::error_code error;
    fs::path receipt = backupFolder / kReceiptName;
    if (!fs::is_regular_file(receipt, error)) {
        if (blockedFile != nullptr) *blockedFile = kReceiptName;
        return false;
    }
    std::ifstream in(receipt, std::ios::binary);
    if (!in) {
        if (blockedFile != nullptr) *blockedFile = kReceiptName;
        return false;
    }
    std::vector<std::string> listed;
    std::vector<std::string> absent;
    bool corruptLine = false;
    std::string line;
    while (std::getline(in, line)) {
        while (!line.empty() && (line.back() == '\r' || line.back() == '\n')) line.pop_back();
        if (line.empty()) continue;
        if (line.rfind("absent ", 0) == 0) {
            if (line.size() <= 7) { corruptLine = true; break; }
            absent.push_back(line.substr(7));
            continue;
        }
        size_t firstSpace = line.find(' ');
        size_t secondSpace = line.find(' ', firstSpace + 1);
        if (firstSpace == std::string::npos || secondSpace == std::string::npos) {
            corruptLine = true;
            break;
        }
        std::string hex = line.substr(0, firstSpace);
        unsigned long long size = 0;
        try { size = std::stoull(line.substr(firstSpace + 1, secondSpace - firstSpace - 1)); }
        catch (...) {
            corruptLine = true;
            break;
        }
        std::string relative = line.substr(secondSpace + 1);
        fs::path file = backupFolder / fs::u8path(relative);
        if (!fs::is_regular_file(file, error)
            || static_cast<unsigned long long>(fs::file_size(file, error)) != size || error) {
            if (blockedFile != nullptr) *blockedFile = relative;
            return false;
        }
        std::string actual;
        if (!hash(file, &actual) || actual != hex) {
            if (blockedFile != nullptr) *blockedFile = relative;
            return false;
        }
        listed.push_back(relative);
    }
    if (corruptLine || !in.eof()) {
        // A truncated or malformed receipt promises an unknown inventory.
        if (blockedFile != nullptr) *blockedFile = kReceiptName;
        return false;
    }
    // Completeness: every file the backup carries must stand in the receipt.
    // A receipt that misses files promises less than the folder holds, so a
    // restore could copy unvalidated bytes.
    std::sort(listed.begin(), listed.end());
    for (fs::recursive_directory_iterator it(backupFolder, fs::directory_options::skip_permission_denied, error), end;
         !error && it != end; it.increment(error)) {
        if (!it->is_regular_file(error)) continue;
        if (it->path().filename().string() == kReceiptName) continue;
        std::string relative = fs::relative(it->path(), backupFolder).generic_string();
        if (!std::binary_search(listed.begin(), listed.end(), relative)) {
            if (blockedFile != nullptr) *blockedFile = relative;
            return false;
        }
    }
    if (error) {
        if (blockedFile != nullptr) *blockedFile = kReceiptName;
        return false;
    }
    // Every required build file must be accounted for: carried and checked,
    // or recorded absent when the machine did not have it.
    for (const std::string& required : requiredFiles) {
        bool carried = std::binary_search(listed.begin(), listed.end(), required);
        bool wasAbsent = std::find(absent.begin(), absent.end(), required) != absent.end();
        if (!carried && !wasAbsent) {
            if (blockedFile != nullptr) *blockedFile = required;
            return false;
        }
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

// The restore itself: introduced paths (N in the intended set) removed first,
// replaced paths never removed (the running launcher survives its own
// restore through the rename-aside path), then every saved build file and
// asset tree copies back.  The caller validated the receipt before calling;
// false carries the first blocked path in *blockedFile and leaves the marker
// and the backup so removing the obstruction and retrying finishes it.
inline bool restoreSavedBuild(const fs::path& gameRoot, const fs::path& backupFolder,
    const std::vector<std::string>& buildFiles, const std::vector<std::string>& buildTrees,
    std::string* blockedFile)
{
    for (const PendingPath& path : readIntendedSet(gameRoot)) {
        if (path.existed) continue;
        fs::path target = gameRoot / fs::u8path(path.relative);
        std::error_code error;
        if (fs::exists(target, error)) {
            fs::remove(target, error);
            if (error) {
                if (blockedFile != nullptr) *blockedFile = path.relative;
                return false;
            }
        }
    }
    for (const std::string& name : buildFiles) {
        if (!restoreBuildFile(backupFolder, gameRoot, name, blockedFile)) return false;
    }
    for (const std::string& name : buildTrees) {
        if (!restoreBuildTree(backupFolder, gameRoot, name, blockedFile)) return false;
    }
    return true;
}

} // namespace launcher_update
