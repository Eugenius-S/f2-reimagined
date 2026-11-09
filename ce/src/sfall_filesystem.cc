#include "sfall_filesystem.h"

#include <stdio.h>
#include <string.h>

#include <filesystem>
#ifdef _WIN32
#include <process.h>
#else
#include <unistd.h>
#endif
#include <string>
#include <system_error>
#include <vector>

#include "art.h"
#include "db.h"
#include "platform_compat.h"

namespace fallout {

typedef struct SfallFileSystemAlias {
    bool active;
    std::string path;
    std::string source;
    std::string normalizedPath;
    // The bytes fs_seek, fs_read_short and fs_write_short work on, read from
    // the source on first use; once written they are served from a temp file.
    std::vector<unsigned char> data;
    bool loaded;
    size_t position;
    bool dirty;
    std::string writtenPath;
} SfallFileSystemAlias;

static std::vector<SfallFileSystemAlias> aliases;

static std::string normalizePath(const char* path)
{
    std::string normalized = path != nullptr ? path : "";
    for (char& ch : normalized) {
        if (ch == '/') {
            ch = '\\';
        } else if (ch >= 'A' && ch <= 'Z') {
            ch = static_cast<char>(ch - 'A' + 'a');
        }
    }
    return normalized;
}

static bool isSoundEffectsPath(const std::string& normalizedPath)
{
    return normalizedPath.rfind("sound\\sfx\\", 0) == 0;
}

static int findAliasIndex(const char* path)
{
    if (path == nullptr || path[0] == '\0') {
        return -1;
    }

    std::string normalized = normalizePath(path);
    for (size_t index = 0; index < aliases.size(); index++) {
        if (aliases[index].active && aliases[index].normalizedPath == normalized) {
            return static_cast<int>(index);
        }
    }

    return -1;
}

int sfallFileSystemCopy(const char* path, const char* source)
{
    if (path == nullptr || path[0] == '\0' || source == nullptr || source[0] == '\0') {
        return -1;
    }

    int existingIndex = findAliasIndex(path);
    if (existingIndex != -1) {
        return existingIndex;
    }

    int fileSize = 0;
    if (dbGetFileSize(source, &fileSize) != 0) {
        return -1;
    }

    SfallFileSystemAlias alias = {};
    alias.active = true;
    alias.path = path;
    alias.source = normalizePath(source);
    alias.normalizedPath = normalizePath(path);

    for (size_t index = 0; index < aliases.size(); index++) {
        if (!aliases[index].active) {
            aliases[index] = alias;
            artCacheFlush();
            return static_cast<int>(index);
        }
    }

    aliases.push_back(alias);
    artCacheFlush();
    return static_cast<int>(aliases.size() - 1);
}

int sfallFileSystemFind(const char* path)
{
    return findAliasIndex(path);
}

// Two games on one machine (the owner's and a test run) keep apart.
static int processId()
{
#ifdef _WIN32
    return _getpid();
#else
    return getpid();
#endif
}

static void removeWrittenFile(SfallFileSystemAlias& alias)
{
    if (!alias.writtenPath.empty()) {
        std::error_code error;
        std::filesystem::remove(alias.writtenPath, error);
        alias.writtenPath.clear();
    }
}

void sfallFileSystemDelete(int id)
{
    if (id < 0 || id >= static_cast<int>(aliases.size()) || !aliases[id].active) {
        return;
    }

    removeWrittenFile(aliases[id]);
    aliases[id] = {};
    artCacheFlush();
}

void sfallFileSystemReset()
{
    if (aliases.empty()) {
        return;
    }

    for (auto& alias : aliases) {
        removeWrittenFile(alias);
    }
    aliases.clear();
    artCacheFlush();
}

static SfallFileSystemAlias* aliasData(int id)
{
    if (id < 0 || id >= static_cast<int>(aliases.size()) || !aliases[id].active) {
        return nullptr;
    }

    SfallFileSystemAlias& alias = aliases[id];
    if (!alias.loaded) {
        alias.loaded = true;
        File* stream = fileOpen(alias.source.c_str(), "rb");
        if (stream != nullptr) {
            int size = fileGetSize(stream);
            if (size > 0) {
                alias.data.resize(size);
                if (fileRead(alias.data.data(), 1, size, stream) != static_cast<size_t>(size)) {
                    alias.data.clear();
                }
            }
            fileClose(stream);
        }
    }
    return &alias;
}

// sfall's FSseek: a position past the end is ignored.
void sfallFileSystemSeek(int id, int position)
{
    SfallFileSystemAlias* alias = aliasData(id);
    if (alias != nullptr && position >= 0 && static_cast<size_t>(position) <= alias->data.size()) {
        alias->position = position;
    }
}

// sfall's FSread_short: big-endian, sign-extended; 0 past the end.
int sfallFileSystemReadShort(int id)
{
    SfallFileSystemAlias* alias = aliasData(id);
    if (alias == nullptr || alias->position + 2 > alias->data.size()) {
        return 0;
    }
    int16_t value = static_cast<int16_t>((alias->data[alias->position] << 8) | alias->data[alias->position + 1]);
    alias->position += 2;
    return value;
}

// sfall's FSwrite_short: big-endian; ignored past the end.
void sfallFileSystemWriteShort(int id, int value)
{
    SfallFileSystemAlias* alias = aliasData(id);
    if (alias == nullptr || alias->position + 2 > alias->data.size()) {
        return;
    }
    alias->data[alias->position++] = static_cast<unsigned char>((value >> 8) & 0xFF);
    alias->data[alias->position++] = static_cast<unsigned char>(value & 0xFF);
    alias->dirty = true;
}

// Writes an edited alias to its own temp file, which the game then reads in
// place of the path.
static bool writeAliasData(SfallFileSystemAlias& alias, size_t index)
{
    if (!alias.dirty) {
        return !alias.writtenPath.empty();
    }

    std::error_code error;
    std::filesystem::path directory = std::filesystem::temp_directory_path(error) / "f2ce-sfall-fs";
    std::filesystem::create_directories(directory, error);
    std::string path = (directory / (std::to_string(processId()) + "-" + std::to_string(index) + ".bin")).string();

    FILE* stream = fopen(path.c_str(), "wb");
    if (stream == nullptr) {
        return false;
    }
    bool written = fwrite(alias.data.data(), 1, alias.data.size(), stream) == alias.data.size();
    fclose(stream);
    if (!written) {
        return false;
    }

    alias.writtenPath = path;
    alias.dirty = false;
    // Art read before the edit is cached; the next draw reads the edited file.
    artCacheFlush();
    return true;
}

bool sfallFileSystemResolveAlias(const char* path, char* resolvedPath, size_t resolvedPathSize)
{
    if (path == nullptr || path[0] == '\0' || resolvedPath == nullptr || resolvedPathSize == 0) {
        return false;
    }

    std::string normalized = normalizePath(path);
    if (isSoundEffectsPath(normalized)) {
        return false;
    }

    int aliasIndex = findAliasIndex(path);
    if (aliasIndex == -1) {
        return false;
    }

    const char* target = aliases[aliasIndex].source.c_str();
    if (writeAliasData(aliases[aliasIndex], aliasIndex)) {
        target = aliases[aliasIndex].writtenPath.c_str();
    } else if (aliases[aliasIndex].source == path) {
        return false;
    }

    int written = snprintf(resolvedPath, resolvedPathSize, "%s", target);
    return written >= 0 && static_cast<size_t>(written) < resolvedPathSize;
}

} // namespace fallout
