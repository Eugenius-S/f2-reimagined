// 273: the on-entry scene build decides what a map's cache is (missing,
// stale, current) without loading it, and builds the command line that
// rebuilds it. Every damaged input must read as not current: a cache of
// unknown origin is rebuilt, never loaded on trust.
#include "../../src/scene/scene_build.h"

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>

namespace fs = std::filesystem;
using fallout::scene_build::CacheState;
using fallout::scene_build::SceneBuild;

namespace {

int failures = 0;

void check(bool condition, const char* what)
{
    if (!condition) {
        std::fprintf(stderr, "FAIL %s\n", what);
        failures++;
    }
}

fs::path scratchDir(const char* name)
{
    fs::path dir = fs::temp_directory_path() / ("scene-build-test" + std::string(name));
    std::error_code error;
    fs::remove_all(dir, error);
    fs::create_directories(dir, error);
    return dir;
}

void writeProvenance(const fs::path& cache, const std::string& text)
{
    fs::create_directories(cache);
    std::ofstream file(cache / "provenance.json", std::ios::binary);
    file << text;
}

} // namespace

int main()
{
    const std::string expected = fallout::scene_build::kExpectedExporterVersion;
    check(!expected.empty(), "expected exporter version is set");

    // Cache states, one directory per case.
    {
        const fs::path none = scratchDir("missing") / "0";
        check(fallout::scene_build::cacheState(none) == CacheState::Missing,
            "no cache directory reads as missing");

        const fs::path current = scratchDir("current");
        writeProvenance(current, "{\"exporter_version\": \"" + expected + "\", \"map\": \"x\"}\n");
        check(fallout::scene_build::cacheState(current) == CacheState::Current,
            "matching provenance reads as current");

        const fs::path older = scratchDir("older");
        writeProvenance(older, "{\"exporter_version\": \"0.0.1\"}\n");
        check(fallout::scene_build::cacheState(older) == CacheState::StaleVersion,
            "another exporter version reads as stale");

        const fs::path unversioned = scratchDir("unversioned");
        writeProvenance(unversioned, "{\"map\": \"x\", \"elevation\": 0}\n");
        check(fallout::scene_build::cacheState(unversioned) == CacheState::StaleVersion,
            "provenance without the version key reads as stale");

        const fs::path noProvenance = scratchDir("no-provenance");
        fs::create_directories(noProvenance);
        check(fallout::scene_build::cacheState(noProvenance) == CacheState::StaleVersion,
            "cache without provenance.json reads as stale");

        const fs::path damaged = scratchDir("damaged");
        fs::create_directories(damaged);
        std::ofstream file(damaged / "provenance.json", std::ios::binary);
        file << "{ not json at all [[[ \"exporter_version\"";
        check(fallout::scene_build::cacheState(damaged) == CacheState::StaleVersion,
            "damaged provenance reads as stale");

        const fs::path truncated = scratchDir("truncated");
        writeProvenance(truncated, "{\"exporter_version\": \"" + expected.substr(0, 3));
        check(fallout::scene_build::cacheState(truncated) == CacheState::StaleVersion,
            "truncated provenance never reads as current");
    }

    // The version reader itself.
    {
        const fs::path dir = scratchDir("version");
        writeProvenance(dir,
            "{\n  \"adapter_sha256\": {\"a\": \"00\"},\n  \"exporter_version\": \"9.9.9\",\n"
            "  \"map\": \"dirtest\"\n}\n");
        check(fallout::scene_build::readExporterVersion(dir) == "9.9.9",
            "version value parsed among other keys");
        const fs::path absent = scratchDir("version-absent");
        fs::create_directories(absent);
        check(fallout::scene_build::readExporterVersion(absent).empty(),
            "missing provenance gives no version");
    }

    // The command line: the launcher-style quotes, the map by name, and
    // --force only for a damaged cache.
    {
        const std::string command = fallout::scene_build::exporterCommand(
            fs::path("C:/Games/F2R/scene-exporter.exe"), fs::path("C:/Games/F2R"),
            fs::path("C:/Games/F2R/mod-cache/scene"), "kladwtwn", false);
        check(command.find("\"C:/Games/F2R/scene-exporter.exe\" --game-root \"C:/Games/F2R\"") == 0,
            "command starts with the quoted exporter and game root");
        check(command.find("--cache-root \"C:/Games/F2R/mod-cache/scene\"") != std::string::npos,
            "command targets the cache root");
        check(command.find("--maps kladwtwn") != std::string::npos, "command names the map");
        check(command.find("--force") == std::string::npos, "no force without damage");
        const std::string forced = fallout::scene_build::exporterCommand(
            fs::path("e"), fs::path("g"), fs::path("c"), "denbus1", true);
        check(forced.find("--maps denbus1 --force") != std::string::npos, "force lands after the map");
    }

    // The build itself refuses what it cannot run.
    {
        SceneBuild build;
        check(build.status() == SceneBuild::Status::Idle, "build starts idle");
        const fs::path nowhere = scratchDir("nowhere") / "scene-exporter.exe";
        check(!build.start("kladwtwn", nowhere, fs::path("g"), fs::path("c"), false),
            "missing exporter refuses to start");
        check(build.status() == SceneBuild::Status::Idle, "refused build stays idle");
        check(!build.start("", fs::path("e"), fs::path("g"), fs::path("c"), false),
            "empty map name refuses to start");
        check(build.poll() == SceneBuild::Status::Idle, "poll without a run stays idle");
    }

    if (failures != 0) {
        std::fprintf(stderr, "scene_build_tests: %d failure(s)\n", failures);
        return 1;
    }
    std::printf("scene_build_tests: ok\n");
    return 0;
}
