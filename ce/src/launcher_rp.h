#pragma once

// Installing the Restoration Project from its official release zip (task 186).
// The zip is RPU's manual package: its files go over the game folder and then
// RPU's own rpu-install.bat runs.  These are that script's rules as plain
// names, so tests/launcher_progress/launcher_rp_test.cc checks them; the
// launcher does the download and the file work.

#include <string>

#include "launcher_uninstall.h"

namespace launcher_rp {

// Where the old files go, as rpu-install.bat keeps them.
inline constexpr const char* kBackupFolder = "backup/rpu";

// Where a file of the package lands in the game folder ('/' separators); empty: not installed.
inline std::string targetOf(const std::string& packagePath)
{
    std::string name = launcher_uninstall::lowered(packagePath);
    if (name == "rpu-install.bat" || name == "rpu-install.sh" || name == "rpu-install.command") return std::string();
    if (name == "data/savegame" || name.rfind("data/savegame/", 0) == 0) return std::string();
    if (name == "mods_order.txt") return "mods/mods_order.txt";
    if (name == "gl_ammomod.int") return "data/scripts/gl_ammomod.int";
    return packagePath;
}

inline bool startsWith(const std::string& text, const char* prefix)
{
    return text.rfind(prefix, 0) == 0;
}

inline bool endsWith(const std::string& text, const char* suffix)
{
    std::string tail = suffix;
    return text.size() >= tail.size() && text.compare(text.size() - tail.size(), tail.size(), tail) == 0;
}

// rpu-install.bat moves data/ aside, so loose vanilla files cannot shadow rpu.dat,
// and brings back the saves, data/*.txt, data/*.gcd and the music.  The splash art
// this mod installs into data/art stays too.  The path is relative to data/.
inline bool keptInData(const std::string& dataPath)
{
    std::string path = launcher_uninstall::lowered(dataPath);
    if (startsWith(path, "savegame/") || startsWith(path, "sound/music/")) return true;
    if (startsWith(path, "art/splash/") || startsWith(path, "art/russian/splash/")) return true;
    return path.find('/') == std::string::npos && (endsWith(path, ".txt") || endsWith(path, ".gcd"));
}

// Game-folder files rpu-install.bat moves aside before RPU's own take over.
inline bool movedAside(const std::string& rootFile)
{
    std::string name = launcher_uninstall::lowered(rootFile);
    return name == "patch000.dat" || name == "f2_res.dat";
}

// Keep the original order and lines another mod added after RP was installed.
inline std::string restoredOrder(const std::string& original, const std::string& installed, const std::string& current)
{
    auto lines = [](const std::string& text) {
        std::vector<std::string> out;
        size_t begin = 0;
        while (begin < text.size()) {
            size_t end = text.find('\n', begin);
            out.push_back(text.substr(begin, end == std::string::npos ? end : end + 1 - begin));
            if (end == std::string::npos) break;
            begin = end + 1;
        }
        return out;
    };
    auto key = [](const std::string& line) { return launcher_uninstall::lowered(launcher_uninstall::trimmed(line)); };
    auto contains = [&key](const auto& list, const std::string& name) {
        return std::any_of(list.begin(), list.end(), [&](const auto& line) { return key(line) == name; });
    };
    auto oldLines = lines(original);
    auto rpLines = lines(installed);
    std::string result = original;
    for (const auto& line : lines(current)) {
        std::string name = key(line);
        if (name.empty() || contains(oldLines, name) || contains(rpLines, name)) continue;
        if (!result.empty() && result.back() != '\n') result += "\r\n";
        result += line;
        oldLines.push_back(line);
    }
    return result;
}

} // namespace launcher_rp
