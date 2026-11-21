#ifndef FALLOUT_SCENE_BUILD_H
#define FALLOUT_SCENE_BUILD_H

#include <filesystem>
#include <string>

namespace fallout {
namespace scene_build {

// The exporter version this build of the game expects in a scene cache's
// provenance.json (273). A cache written by any other version is rebuilt,
// not loaded. tools/test_exporter_version_sync.py fails CI when this drifts
// from tools/scene_export/export.py's EXPORTER_VERSION.
extern const char kExpectedExporterVersion[];

// What the game can know about a map elevation's cache before loading it.
enum class CacheState {
    Missing, // no cache directory: nothing was ever exported
    StaleVersion, // no provenance.json, an unreadable one, or another version
    Current, // written by the exporter version this game ships with
};

CacheState cacheState(const std::filesystem::path& cacheDirectory);

// provenance.json's "exporter_version" value; "" when the file is missing,
// damaged or carries no such key. Never throws.
std::string readExporterVersion(const std::filesystem::path& cacheDirectory);

// The scene-exporter.exe shipped beside the game executable.
std::filesystem::path exporterPath();

// The command line that (re)builds one map's scenes into the cache root the
// game reads (the dev cache override included). force rewrites scenes whose
// files are damaged even though their provenance still matches.
std::string exporterCommand(const std::filesystem::path& exporter,
    const std::filesystem::path& gameRoot, const std::filesystem::path& cacheRoot,
    const std::string& map, bool force);

// One exporter run at a time (273): the game keeps drawing while it works.
class SceneBuild {
public:
    enum class Status {
        Idle, // nothing was started
        Running, // the exporter is still working
        Succeeded, // it exited 0
        Failed, // nonzero exit, it could not start, or the time limit hit
    };

    // Starts the exporter for `map`. False when the exporter is missing, a
    // build is already running, or the map name is empty; the caller then
    // keeps its own fallback.
    bool start(const std::string& map, const std::filesystem::path& exporter,
        const std::filesystem::path& gameRoot, const std::filesystem::path& cacheRoot,
        bool force);
    // Non-blocking progress check; enforces the time limit on a stuck run.
    Status poll();
    Status status() const { return _status; }
    const std::string& map() const { return _map; }
    unsigned elapsedMs() const { return _elapsedMs; }

private:
    void* _process = nullptr; // the OS process handle, opaque here
    std::string _map;
    Status _status = Status::Idle;
    unsigned _elapsedMs = 0;
    long long _startedSteadyMs = 0;
};

} // namespace scene_build
} // namespace fallout

#endif
