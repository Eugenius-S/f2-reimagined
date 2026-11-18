#include "scene_build.h"

#include <chrono>
#include <cstdint>
#include <fstream>
#include <system_error>
#include <vector>

#if defined(_WIN32)
#include <windows.h>
#endif

namespace fallout {
namespace scene_build {

// export.py writes this into every provenance.json it produces; the sync
// test keeps the pair together.
const char kExpectedExporterVersion[] = "1.13.0";

// One map export takes seconds to a minute; the launcher's full first
// install (every map) is the only thing measured in tens of minutes.
constexpr unsigned kBuildTimeoutMs = 600000;

namespace {

long long steadyNowMs()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch())
        .count();
}

// The value of the first "exporter_version" string in the text, with JSON
// escapes left as they came (versions are plain digits and dots).
std::string findExporterVersion(const std::string& text)
{
    const std::string key = "\"exporter_version\"";
    const size_t at = text.find(key);
    if (at == std::string::npos) return "";
    const size_t first = text.find('"', at + key.size());
    if (first == std::string::npos) return "";
    const size_t second = text.find('"', first + 1);
    if (second == std::string::npos) return "";
    return text.substr(first + 1, second - first - 1);
}

} // namespace

std::string readExporterVersion(const std::filesystem::path& cacheDirectory)
{
    std::error_code error;
    if (!std::filesystem::is_directory(cacheDirectory, error)) return "";
    std::ifstream file(cacheDirectory / "provenance.json", std::ios::binary);
    if (!file) return "";
    // provenance.json carries archive lists; a sane cap keeps a damaged file
    // from being read as a whole into memory.
    std::string text(4096, '\0');
    file.read(text.data(), static_cast<std::streamsize>(text.size()));
    text.resize(static_cast<size_t>(file.gcount()));
    // The version key sits near the end of the object: what fit in the first
    // 4 KiB was not enough, read the rest too.
    if (text.find("\"exporter_version\"") == std::string::npos && !file.eof()) {
        std::string rest((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        text += rest;
    }
    return findExporterVersion(text);
}

CacheState cacheState(const std::filesystem::path& cacheDirectory)
{
    std::error_code error;
    if (!std::filesystem::is_directory(cacheDirectory, error)) return CacheState::Missing;
    const std::string version = readExporterVersion(cacheDirectory);
    if (version.empty()) return CacheState::StaleVersion;
    if (version != kExpectedExporterVersion) return CacheState::StaleVersion;
    return CacheState::Current;
}

std::filesystem::path exporterPath()
{
#if defined(_WIN32)
    wchar_t module[MAX_PATH];
    const DWORD length = GetModuleFileNameW(nullptr, module, MAX_PATH);
    if (length == 0 || length >= MAX_PATH) return {};
    std::filesystem::path self(module);
#else
    // The exporter ships with the Windows install layout; other platforms
    // have none beside the binary and the caller keeps its fallback.
    return {};
#endif
    std::error_code error;
    return self.parent_path() / "scene-exporter.exe";
}

std::string exporterCommand(const std::filesystem::path& exporter,
    const std::filesystem::path& gameRoot, const std::filesystem::path& cacheRoot,
    const std::string& map, bool force)
{
    std::string command = "\"" + exporter.string() + "\" --game-root \"" + gameRoot.string()
        + "\" --cache-root \"" + cacheRoot.string() + "\" --maps " + map;
    if (force) command += " --force";
    return command;
}

bool SceneBuild::start(const std::string& map, const std::filesystem::path& exporter,
    const std::filesystem::path& gameRoot, const std::filesystem::path& cacheRoot, bool force)
{
    if (_status == Status::Running) return false;
    if (map.empty()) return false;
    std::error_code error;
    if (!std::filesystem::is_regular_file(exporter, error)) return false;
#if defined(_WIN32)
    // The command line buffer belongs to CreateProcessW: it mutates it.
    std::wstring wide = L"\"";
    wide += exporter.wstring();
    wide += L"\" --game-root \"";
    wide += gameRoot.wstring();
    wide += L"\" --cache-root \"";
    wide += cacheRoot.wstring();
    wide += L"\" --maps ";
    for (const char ch : map) wide += static_cast<wchar_t>(ch);
    if (force) wide += L" --force";
    std::vector<wchar_t> mutableCommand(wide.begin(), wide.end());
    mutableCommand.push_back(L'\0');
    STARTUPINFOW startup = {};
    startup.cb = sizeof(startup);
    PROCESS_INFORMATION process = {};
    if (!CreateProcessW(nullptr, mutableCommand.data(), nullptr, nullptr, FALSE,
            CREATE_NO_WINDOW, nullptr, nullptr, &startup, &process)) {
        return false;
    }
    CloseHandle(process.hThread);
    _process = process.hProcess;
    _map = map;
    _status = Status::Running;
    _elapsedMs = 0;
    _startedSteadyMs = steadyNowMs();
    return true;
#else
    (void)gameRoot;
    (void)cacheRoot;
    (void)force;
    return false;
#endif
}

SceneBuild::Status SceneBuild::poll()
{
    if (_status != Status::Running) return _status;
    _elapsedMs = static_cast<unsigned>(steadyNowMs() - _startedSteadyMs);
#if defined(_WIN32)
    HANDLE process = static_cast<HANDLE>(_process);
    if (WaitForSingleObject(process, 0) == WAIT_OBJECT_0) {
        DWORD code = STILL_ACTIVE;
        GetExitCodeProcess(process, &code);
        CloseHandle(process);
        _process = nullptr;
        _status = code == 0 ? Status::Succeeded : Status::Failed;
        return _status;
    }
    if (_elapsedMs >= kBuildTimeoutMs) {
        TerminateProcess(process, 1);
        WaitForSingleObject(process, 5000);
        CloseHandle(process);
        _process = nullptr;
        _status = Status::Failed;
        return _status;
    }
    return Status::Running;
#else
    _status = Status::Failed;
    return _status;
#endif
}

} // namespace scene_build
} // namespace fallout
