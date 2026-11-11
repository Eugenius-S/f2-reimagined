#pragma once

// The launcher's progress bar and per-map log lines, read from the worker's
// own output (task 232).  No windows, no IPC: the UI feeds it each line.
//   download  "  ... N / M MB"                      0..10 %
//   unpack    "unpacking" (or its Russian line)      10 %
//   export    {"maps_total": K, "scenes_total": N}  12..100 %, then one
//             scene line per scene ({"output": ...}, "current": true when
//             skipped) or "FAILED map eN: ..."

#include <algorithm>
#include <cstdio>
#include <string>

#include "launcher_ru.h"

namespace launcher_progress {

enum class MapResult { None, Built, Current, Failed };

struct Tracker {
    int value = 0; // percent, never goes back
    int mapsDone = 0;
    int mapsTotal = 0;
    int scenesDone = 0;
    int scenesTotal = 0;
    // Set by feed() when a map has just finished.
    std::string finishedMap;
    MapResult finishedResult = MapResult::None;
    // Set by feed() for lines the log window should not show (raw scene JSON).
    bool hideLine = false;

    void raise(int percent) { value = std::max(value, std::min(percent, 99)); }

    static bool numberAfter(const std::string& line, const char* key, int* out)
    {
        size_t at = line.find(key);
        if (at == std::string::npos) return false;
        return std::sscanf(line.c_str() + at + std::string(key).size(), " %d", out) == 1;
    }

    // The map folder of a scene line: .../<map>/<elevation>, JSON-escaped.
    static std::string mapOfOutput(const std::string& line)
    {
        const std::string key = "\"output\": \"";
        size_t at = line.find(key);
        if (at == std::string::npos) return std::string();
        std::string path;
        for (size_t i = at + key.size(); i < line.size() && line[i] != '"'; i++) {
            if (line[i] == '\\' && i + 1 < line.size()) i++;
            path += line[i] == '\\' ? '/' : line[i];
        }
        size_t last = path.find_last_of('/');
        if (last == std::string::npos || last == 0) return std::string();
        size_t before = path.find_last_of('/', last - 1);
        return path.substr(before == std::string::npos ? 0 : before + 1, last - before - 1);
    }

    void sceneDone(const std::string& map, MapResult result)
    {
        if (!openMap.empty() && map != openMap) closeMap();
        openMap = map;
        if (result == MapResult::Failed) openFailed = true;
        else if (result == MapResult::Built) openBuilt = true;
        scenesDone++;
        if (scenesTotal > 0) raise(12 + 88 * scenesDone / scenesTotal);
        if (scenesTotal > 0 && scenesDone >= scenesTotal) closeMap();
    }

    void feed(const std::string& line)
    {
        finishedMap.clear();
        finishedResult = MapResult::None;
        hideLine = false;
        int a = 0;
        int b = 0;
        if (std::sscanf(line.c_str(), "  ... %d / %d MB", &a, &b) == 2 && b > 0) {
            raise(10 * a / b);
        } else if (line == launcher_ru::kUnpacking || line == launcher_ru::kUnpackingRu) {
            raise(10);
        } else if (line.rfind(launcher_ru::kMakingScene, 0) == 0 || line.rfind(launcher_ru::kMakingSceneRu, 0) == 0) {
            raise(12);
        } else if (line.rfind("{\"maps_total\"", 0) == 0) {
            hideLine = true;
            numberAfter(line, "\"maps_total\":", &mapsTotal);
            numberAfter(line, "\"scenes_total\":", &scenesTotal);
        } else if (line.rfind("{", 0) == 0 && line.find("\"output\":") != std::string::npos) {
            hideLine = true;
            sceneDone(mapOfOutput(line),
                line.find("\"current\": true") != std::string::npos ? MapResult::Current : MapResult::Built);
        } else if (line.rfind("FAILED ", 0) == 0) {
            // "FAILED map eN: ..." is one scene; "FAILED map: ..." never entered the total.
            size_t space = line.find(' ', 7);
            if (space != std::string::npos && line.compare(space, 2, " e") == 0) {
                sceneDone(line.substr(7, space - 7), MapResult::Failed);
            }
        }
    }

private:
    std::string openMap;
    bool openBuilt = false;
    bool openFailed = false;

    void closeMap()
    {
        if (openMap.empty()) return;
        finishedMap = openMap;
        finishedResult = openFailed ? MapResult::Failed : (openBuilt ? MapResult::Built : MapResult::Current);
        mapsDone++;
        openMap.clear();
        openBuilt = openFailed = false;
    }
};

} // namespace launcher_progress
