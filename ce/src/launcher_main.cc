// Windows launcher for Fallout 2: Reimagined (task 091).
//
// One small console exe a player runs against a clean Fallout 2 (Steam, GOG
// or a plain copy).  First run: find the game, install our build from the
// rolling `latest` GitHub release and make the 3D scene data with the packed
// exporter.  Later runs: pull a newer build when the release moved, then
// start the game.  The player's fallout2.cfg, saves and the original
// fallout2.exe are never touched.  No network: the installed game starts as
// it is.
//
// Only Windows system libraries are used (winhttp, bcrypt, advapi32, shell).

#define NOMINMAX
#include <windows.h>
#include <winhttp.h>
#include <bcrypt.h>
#include <shobjidl.h>

#include <algorithm>
#include <cctype>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <sstream>
#include <string>
#include <chrono>
#include <vector>

#include "launcher_routes.h"
#include "launcher_rp.h"
#include "launcher_ru.h"
#include "launcher_uninstall.h"

namespace fs = std::filesystem;

namespace {

constexpr const char* kDefaultRepo = "Eugenius-S/f2-reimagined";
constexpr const char* kZipAsset = "fallout2-ce-3d-win64.zip";
constexpr const char* kDefaultUpdateBase =
    "https://github.com/Eugenius-S/f2-reimagined/releases/download/latest";
constexpr const char* kDefaultReleaseInfoUrl =
    "https://api.github.com/repos/Eugenius-S/f2-reimagined/releases/tags/latest";
// The web page answers 302 to .../releases/tag/<tag>; unlike api.github.com it has no 60 requests/hour limit.
constexpr const char* kRpuRepo = "BGforgeNet/Fallout2_Restoration_Project";
constexpr const char* kRpuLatestUrl =
    "https://github.com/BGforgeNet/Fallout2_Restoration_Project/releases/latest";
constexpr const char* kGameExe = "fallout2-ce.exe";
constexpr const char* kCfg = "fallout2.cfg";
constexpr const char* kVersionTxt = "version.txt";
constexpr const char* kExporterExe = "scene-exporter.exe";
// The launcher as the player sees it in the game folder; the release zip carries it under kZipLauncher.
constexpr const char* kRootLauncher = "Fallout 2 Reimagined.exe";
constexpr const char* kZipLauncher = "fallout2-ce-3d-launcher.exe";
constexpr const char* kDefaultMaps = "all";
// The project mirror (task 235): the CI branch `dist`.  Since task 238 it also holds
// rpu_russian.dat and rpu-version.txt.  --mirror-url overrides it (tests).
std::string g_mirrorBase = std::string("https://raw.githubusercontent.com/") + kDefaultRepo + "/dist/";
// --ui-language ru: the user-facing log lines are printed in Russian (launcher_ru.h).
bool g_ru = false;
// --rp-zip-url: the Restoration Project zip from elsewhere instead of its GitHub release (tests).
std::string g_rpZipUrl;

struct Options {
    std::string gameRoot;
    std::string updateBase = kDefaultUpdateBase;
    std::string releaseInfoUrl = kDefaultReleaseInfoUrl;
    std::string maps = kDefaultMaps;
    bool noStart = false;
    bool forceExport = false;
    bool checkRelease = false;
    bool rollback = false;
    bool installRussian = false;
    bool removeRussian = false;
    bool installRp = false;
    bool uninstall = false;
    std::vector<std::string> gameArgs;
};

const char* tr(const char* english)
{
    return g_ru ? launcher_ru::translate(english) : english;
}

void log(const char* text)
{
    std::printf("%s\n", tr(text));
    std::fflush(stdout);
}

void logf(const char* format, ...)
{
    char buffer[1024];
    va_list args;
    va_start(args, format);
    std::vsnprintf(buffer, sizeof(buffer), tr(format), args);
    va_end(args);
    std::printf("%s\n", buffer);
    std::fflush(stdout);
}

[[noreturn]] void die(const char* format, ...)
{
    char buffer[1024];
    va_list args;
    va_start(args, format);
    std::vsnprintf(buffer, sizeof(buffer), tr(format), args);
    va_end(args);
    logf("ERROR: %s", buffer);
    std::exit(1);
}

std::string trim(const std::string& text)
{
    size_t begin = text.find_first_not_of(" \t\r\n");
    if (begin == std::string::npos) return std::string();
    size_t end = text.find_last_not_of(" \t\r\n");
    return text.substr(begin, end - begin + 1);
}

std::string wideToUtf8(const std::wstring& wide)
{
    if (wide.empty()) return std::string();
    int size = WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (size <= 1) return std::string();
    std::string out(static_cast<size_t>(size), '\0');
    if (WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), -1, out.data(), size, nullptr, nullptr) == 0) return std::string();
    out.resize(static_cast<size_t>(size - 1));
    return out;
}

std::wstring utf8ToWide(const std::string& text)
{
    if (text.empty()) return std::wstring();
    int size = MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, nullptr, 0);
    if (size <= 1) return std::wstring();
    std::wstring out(static_cast<size_t>(size), L'\0');
    if (MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, out.data(), size) == 0) return std::wstring();
    out.resize(static_cast<size_t>(size - 1));
    return out;
}

// --- hashing -------------------------------------------------------------

std::string hashToHex(const unsigned char digest[32])
{
    static const char* digits = "0123456789abcdef";
    std::string out;
    out.reserve(64);
    for (int i = 0; i < 32; i++) {
        out.push_back(digits[digest[i] >> 4]);
        out.push_back(digits[digest[i] & 0xF]);
    }
    return out;
}

bool sha256Data(const unsigned char* data, size_t size, unsigned char digest[32])
{
    BCRYPT_ALG_HANDLE algorithm = nullptr;
    if (BCryptOpenAlgorithmProvider(&algorithm, BCRYPT_SHA256_ALGORITHM, nullptr, 0) != 0) return false;
    BCRYPT_HASH_HANDLE hash = nullptr;
    bool ok = BCryptCreateHash(algorithm, &hash, nullptr, 0, nullptr, 0, 0) == 0;
    if (ok) ok = BCryptHashData(hash, const_cast<PUCHAR>(data), static_cast<ULONG>(size), 0) == 0;
    if (ok) ok = BCryptFinishHash(hash, digest, 32, 0) == 0;
    if (hash != nullptr) BCryptDestroyHash(hash);
    BCryptCloseAlgorithmProvider(algorithm, 0);
    return ok;
}

bool sha256File(const fs::path& path, std::string* hex)
{
    std::ifstream file(path, std::ios::binary);
    if (!file) return false;
    std::vector<unsigned char> contents((std::istreambuf_iterator<char>(file)),
        std::istreambuf_iterator<char>());
    unsigned char digest[32];
    if (!sha256Data(contents.data(), contents.size(), digest)) return false;
    *hex = hashToHex(digest);
    return true;
}

// One hash over every file under the directory (paths and contents); saves
// the player has are caught whatever their names.
bool sha256Tree(const fs::path& root, std::string* hex)
{
    std::vector<fs::path> files;
    std::error_code error;
    for (fs::recursive_directory_iterator it(root, fs::directory_options::skip_permission_denied, error), end;
         !error && it != end; it.increment(error)) {
        if (it->is_regular_file(error)) files.push_back(it->path());
    }
    std::sort(files.begin(), files.end());
    unsigned char digest[32];
    BCRYPT_ALG_HANDLE algorithm = nullptr;
    if (BCryptOpenAlgorithmProvider(&algorithm, BCRYPT_SHA256_ALGORITHM, nullptr, 0) != 0) return false;
    BCRYPT_HASH_HANDLE hash = nullptr;
    bool ok = BCryptCreateHash(algorithm, &hash, nullptr, 0, nullptr, 0, 0) == 0;
    auto feed = [&hash, &ok](const void* data, size_t size) {
        if (ok) ok = BCryptHashData(hash, const_cast<PUCHAR>(static_cast<const unsigned char*>(data)),
                        static_cast<ULONG>(size), 0) == 0;
    };
    char none = 0;
    if (files.empty()) feed(&none, 1);
    for (const fs::path& file : files) {
        std::string relative = wideToUtf8(fs::relative(file, root).generic_wstring());
        feed(relative.data(), relative.size());
        feed(&none, 1);
        std::ifstream in(file, std::ios::binary);
        if (!in) {
            ok = false;
            break;
        }
        char block[65536];
        while (in.read(block, sizeof(block)) || in.gcount() > 0) {
            feed(block, static_cast<size_t>(in.gcount()));
            if (!in) break;
        }
        feed(&none, 1);
    }
    if (ok) ok = BCryptFinishHash(hash, digest, 32, 0) == 0;
    if (hash != nullptr) BCryptDestroyHash(hash);
    BCryptCloseAlgorithmProvider(algorithm, 0);
    if (!ok) return false;
    *hex = hashToHex(digest);
    return true;
}

// --- HTTP ----------------------------------------------------------------

struct Url {
    std::wstring scheme;
    std::wstring host;
    int port = 443;
    std::wstring path;
    bool https = true;
};

bool crackUrlWide(const std::wstring& wide, Url* url)
{
    URL_COMPONENTS parts = {};
    parts.dwStructSize = sizeof(parts);
    parts.dwSchemeLength = (DWORD)-1;
    parts.dwHostNameLength = (DWORD)-1;
    parts.dwUrlPathLength = (DWORD)-1;
    parts.dwExtraInfoLength = (DWORD)-1;
    if (!WinHttpCrackUrl(wide.c_str(), 0, 0, &parts)) return false;
    url->scheme.assign(parts.lpszScheme, parts.dwSchemeLength);
    url->host.assign(parts.lpszHostName, parts.dwHostNameLength);
    url->port = parts.nPort;
    url->path.assign(parts.lpszUrlPath, parts.dwUrlPathLength);
    if (parts.lpszExtraInfo != nullptr && parts.dwExtraInfoLength > 0) {
        url->path.append(parts.lpszExtraInfo, parts.dwExtraInfoLength);
    }
    url->https = url->scheme == L"https";
    return true;
}

// A loopback release server must not travel through the system proxy.
DWORD proxyFor(const Url& url)
{
    if (url.host == L"127.0.0.1" || url.host == L"localhost" || url.host == L"::1") {
        return WINHTTP_ACCESS_TYPE_NO_PROXY;
    }
    return WINHTTP_ACCESS_TYPE_DEFAULT_PROXY;
}

// GET one url; the failure reason lands in errorText when set.
bool httpGet(const std::string& urlString, std::string* body, const char** errorText,
    const wchar_t* headers = WINHTTP_NO_ADDITIONAL_HEADERS)
{
    Url url;
    if (!crackUrlWide(utf8ToWide(urlString), &url)) {
        *errorText = "cannot parse url";
        return false;
    }
    *errorText = nullptr;
    HINTERNET session = WinHttpOpen(L"fallout2-ce-3d-launcher", proxyFor(url),
        WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (session == nullptr) {
        *errorText = "WinHttpOpen failed";
        return false;
    }
    WinHttpSetTimeouts(session, 10000, 15000, 30000, 300000);
    HINTERNET connect = WinHttpConnect(session, url.host.c_str(), url.port, 0);
    if (connect == nullptr) {
        *errorText = "cannot connect";
        WinHttpCloseHandle(session);
        return false;
    }
    HINTERNET request = WinHttpOpenRequest(connect, L"GET", url.path.c_str(), nullptr, WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES, url.https ? WINHTTP_FLAG_SECURE : 0);
    bool ok = request != nullptr;
    if (!ok) {
        *errorText = "cannot open request";
    } else {
        ok = WinHttpSendRequest(request, headers, headers == WINHTTP_NO_ADDITIONAL_HEADERS ? 0 : static_cast<DWORD>(-1L), WINHTTP_NO_REQUEST_DATA, 0, 0, 0);
        if (!ok) *errorText = "send failed";
    }
    if (ok) {
        ok = WinHttpReceiveResponse(request, nullptr);
        if (!ok) *errorText = "receive failed";
    }
    DWORD status = 0;
    DWORD statusSize = sizeof(status);
    if (ok) {
        WinHttpQueryHeaders(request, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
            WINHTTP_HEADER_NAME_BY_INDEX, &status, &statusSize, WINHTTP_NO_HEADER_INDEX);
        ok = status == 200;
        if (!ok) *errorText = "http status is not 200";
    }
    if (ok) {
        DWORD available = 0;
        while (WinHttpQueryDataAvailable(request, &available) && available > 0) {
            std::vector<char> chunk(available);
            DWORD read = 0;
            if (!WinHttpReadData(request, chunk.data(), available, &read) || read == 0) break;
            body->append(chunk.data(), read);
        }
        ok = !body->empty();
        if (!ok) *errorText = "empty response";
    }
    if (request != nullptr) WinHttpCloseHandle(request);
    if (connect != nullptr) WinHttpCloseHandle(connect);
    WinHttpCloseHandle(session);
    return ok;
}

// GET one url without following redirects and return the Location header of the answer.
bool httpRedirectLocation(const std::string& urlString, std::string* location, const char** errorText)
{
    Url url;
    if (!crackUrlWide(utf8ToWide(urlString), &url)) {
        *errorText = "cannot parse url";
        return false;
    }
    *errorText = nullptr;
    HINTERNET session = WinHttpOpen(L"fallout2-ce-3d-launcher", proxyFor(url),
        WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (session == nullptr) {
        *errorText = "WinHttpOpen failed";
        return false;
    }
    WinHttpSetTimeouts(session, 10000, 15000, 30000, 300000);
    HINTERNET connect = WinHttpConnect(session, url.host.c_str(), url.port, 0);
    if (connect == nullptr) {
        *errorText = "cannot connect";
        WinHttpCloseHandle(session);
        return false;
    }
    HINTERNET request = WinHttpOpenRequest(connect, L"GET", url.path.c_str(), nullptr, WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES, url.https ? WINHTTP_FLAG_SECURE : 0);
    bool ok = request != nullptr;
    if (!ok) {
        *errorText = "cannot open request";
    } else {
        DWORD disable = WINHTTP_DISABLE_REDIRECTS;
        WinHttpSetOption(request, WINHTTP_OPTION_DISABLE_FEATURE, &disable, sizeof(disable));
        ok = WinHttpSendRequest(request, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0);
        if (!ok) *errorText = "send failed";
    }
    if (ok) {
        ok = WinHttpReceiveResponse(request, nullptr);
        if (!ok) *errorText = "receive failed";
    }
    if (ok) {
        wchar_t buffer[2048] = {};
        DWORD size = sizeof(buffer) - sizeof(wchar_t);
        ok = WinHttpQueryHeaders(request, WINHTTP_QUERY_LOCATION, WINHTTP_HEADER_NAME_BY_INDEX,
                 buffer, &size, WINHTTP_NO_HEADER_INDEX)
            != FALSE;
        if (ok) *location = wideToUtf8(buffer);
        else *errorText = "no redirect from the release page";
    }
    if (request != nullptr) WinHttpCloseHandle(request);
    if (connect != nullptr) WinHttpCloseHandle(connect);
    WinHttpCloseHandle(session);
    return ok;
}

bool httpDownload(const std::string& urlString, const fs::path& destination, const char** errorText,
    const wchar_t* headers = WINHTTP_NO_ADDITIONAL_HEADERS)
{
    Url url;
    if (!crackUrlWide(utf8ToWide(urlString), &url)) {
        *errorText = "cannot parse url";
        return false;
    }
    *errorText = nullptr;
    HINTERNET session = WinHttpOpen(L"fallout2-ce-3d-launcher", proxyFor(url),
        WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (session == nullptr) {
        *errorText = "WinHttpOpen failed";
        return false;
    }
    WinHttpSetTimeouts(session, 10000, 15000, 30000, 300000);
    HINTERNET connect = WinHttpConnect(session, url.host.c_str(), url.port, 0);
    HINTERNET request = nullptr;
    bool ok = connect != nullptr;
    if (ok) {
        request = WinHttpOpenRequest(connect, L"GET", url.path.c_str(), nullptr, WINHTTP_NO_REFERER,
            WINHTTP_DEFAULT_ACCEPT_TYPES, url.https ? WINHTTP_FLAG_SECURE : 0);
        ok = request != nullptr;
        if (!ok) *errorText = "cannot open request";
    }
    if (ok) {
        ok = WinHttpSendRequest(request, headers, headers == WINHTTP_NO_ADDITIONAL_HEADERS ? 0 : static_cast<DWORD>(-1L), WINHTTP_NO_REQUEST_DATA, 0, 0, 0);
        if (!ok) *errorText = "send failed";
    }
    if (ok) {
        ok = WinHttpReceiveResponse(request, nullptr);
        if (!ok) *errorText = "receive failed";
    }
    DWORD status = 0;
    DWORD statusSize = sizeof(status);
    if (ok) {
        WinHttpQueryHeaders(request, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
            WINHTTP_HEADER_NAME_BY_INDEX, &status, &statusSize, WINHTTP_NO_HEADER_INDEX);
        ok = status == 200;
        if (!ok) *errorText = "http status is not 200";
    }
    std::ofstream out;
    if (ok) {
        out.open(destination, std::ios::binary | std::ios::trunc);
        ok = static_cast<bool>(out);
        if (!ok) *errorText = "cannot open the destination file";
    }
    if (ok) {
        DWORD available = 0;
        size_t total = 0;
        DWORD contentLength = 0;
        DWORD lengthSize = sizeof(contentLength);
        if (!WinHttpQueryHeaders(request, WINHTTP_QUERY_CONTENT_LENGTH | WINHTTP_QUERY_FLAG_NUMBER,
                WINHTTP_HEADER_NAME_BY_INDEX, &contentLength, &lengthSize, WINHTTP_NO_HEADER_INDEX)) {
            contentLength = 0;
        }
        while (WinHttpQueryDataAvailable(request, &available) && available > 0) {
            std::vector<char> chunk(available);
            DWORD read = 0;
            if (!WinHttpReadData(request, chunk.data(), available, &read) || read == 0) break;
            out.write(chunk.data(), read);
            total += read;
            if (total / (1024 * 1024) != (total - read) / (1024 * 1024)) {
                if (contentLength > 0) logf("  ... %zu / %lu MB", total / (1024 * 1024), static_cast<unsigned long>(contentLength / (1024 * 1024)));
                else logf("  ... %zu MB", total / (1024 * 1024));
            }
        }
        out.close();
        ok = total > 0;
        if (!ok) *errorText = "empty download";
        else logf("  downloaded %zu bytes", total);
    }
    if (request != nullptr) WinHttpCloseHandle(request);
    if (connect != nullptr) WinHttpCloseHandle(connect);
    WinHttpCloseHandle(session);
    return ok;
}

std::string joinUrl(const std::string& base, const char* name);

// --- download routes (task 235) -------------------------------------------
// Through some VPNs github.com/.../releases/download answers 504 while the API
// and raw.githubusercontent.com work, so every file tries three routes in turn.

// Where one asset of a repo's release can be had.  Only the official release
// has the mirror (the CI branch `dist`); a custom --update-url gets its one url.
std::vector<launcher_routes::Route> releaseRoutes(const std::string& base, const char* file)
{
    std::vector<launcher_routes::Route> routes = { { "github.com", joinUrl(base, file), "" } };
    if (base == kDefaultUpdateBase) {
        std::string repo = kDefaultRepo;
        routes.push_back({ "api", "https://api.github.com/repos/" + repo + "/releases/tags/latest", file });
        routes.push_back({ "mirror", g_mirrorBase + file, "" });
    }
    return routes;
}

// Fetch into body (text) or into dest (a file); the console names the route that worked.
bool fetchRoutes(const std::vector<launcher_routes::Route>& routes, std::string* body, const fs::path* dest,
    std::string* error)
{
    std::string worked;
    bool ok = launcher_routes::tryRoutes(routes, [&](const launcher_routes::Route& route, std::string* why) {
        std::string url = route.url;
        const wchar_t* headers = WINHTTP_NO_ADDITIONAL_HEADERS;
        const char* e = nullptr;
        if (!route.apiAsset.empty()) {
            std::string json;
            if (!httpGet(url, &json, &e)) {
                *why = std::string(e != nullptr ? e : "network error") + " (release info)";
                logf("  route %s failed: %s", route.label.c_str(), why->c_str());
                return false;
            }
            url = launcher_routes::assetApiUrl(json, route.apiAsset);
            if (url.empty()) {
                *why = "the asset is not in the release";
                logf("  route %s failed: %s", route.label.c_str(), why->c_str());
                return false;
            }
            headers = L"Accept: application/octet-stream";
        }
        if (body != nullptr) body->clear();
        bool got = body != nullptr ? httpGet(url, body, &e, headers) : httpDownload(url, *dest, &e, headers);
        if (!got) {
            *why = e != nullptr ? e : "network error";
            logf("  route %s failed: %s", route.label.c_str(), why->c_str());
        }
        return got;
    }, &worked, error);
    if (ok) logf("  route that worked: %s", worked.c_str());
    return ok;
}

// --- finding the game ----------------------------------------------------

bool looksLikeFallout2(const fs::path& root)
{
    std::error_code error;
    return fs::exists(root / "master.dat", error) && fs::exists(root / "critter.dat", error);
}

std::string readRegistryString(HKEY hive, const char* key, const char* value)
{
    char buffer[MAX_PATH * 2] = {};
    DWORD size = sizeof(buffer);
    DWORD type = 0;
    LSTATUS status = RegGetValueA(hive, key, value, RRF_RT_REG_SZ, &type, buffer, &size);
    if (status != ERROR_SUCCESS) return std::string();
    return trim(std::string(buffer));
}

// A Steam library folder list lives in steamapps/libraryfolders.vdf; the
// entries this launcher needs are plain "path" lines.
std::vector<std::string> steamLibraries(const std::string& steamRoot)
{
    std::vector<std::string> out;
    std::ifstream file(fs::path(steamRoot) / "steamapps" / "libraryfolders.vdf");
    if (!file) return out;
    std::string line;
    while (std::getline(file, line)) {
        size_t key = line.find("\"path\"");
        if (key == std::string::npos) continue;
        size_t open = line.find('"', key + 6);
        if (open == std::string::npos) continue;
        size_t close = line.find('"', open + 1);
        if (close == std::string::npos) continue;
        std::string path = line.substr(open + 1, close - open - 1);
        size_t pos = 0;
        while ((pos = path.find("\\\\", pos)) != std::string::npos) path.replace(pos, 2, "\\");
        if (!path.empty()) out.push_back(path);
    }
    return out;
}

std::string pickFolder()
{
    std::string chosen;
    if (FAILED(CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED))) return chosen;
    IFileDialog* dialog = nullptr;
    if (SUCCEEDED(CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&dialog)))
        && dialog != nullptr) {
        DWORD options = 0;
        dialog->GetOptions(&options);
        dialog->SetOptions(options | FOS_PICKFOLDERS | FOS_FORCEFILESYSTEM);
        dialog->SetTitle(L"Pick your Fallout 2 folder (the one with master.dat)");
        IShellItem* result = nullptr;
        if (SUCCEEDED(dialog->Show(nullptr)) && SUCCEEDED(dialog->GetResult(&result)) && result != nullptr) {
            PWSTR path = nullptr;
            if (SUCCEEDED(result->GetDisplayName(SIGDN_FILESYSPATH, &path)) && path != nullptr) {
                chosen = wideToUtf8(path);
                CoTaskMemFree(path);
            }
            result->Release();
        }
        dialog->Release();
    }
    CoUninitialize();
    return chosen;
}

// Returns an empty string when the player cancelled.
std::string findGameRoot(bool allowPicker)
{
    std::vector<std::string> candidates;

    // GOG: the installer writes the game folder here.
    std::string gog = readRegistryString(HKEY_LOCAL_MACHINE,
        "SOFTWARE\\WOW6432Node\\GOG.com\\Games\\1207658924", "path");
    if (!gog.empty()) {
        logf("registry: GOG Fallout 2 at %s", gog.c_str());
        candidates.push_back(gog);
    }

    // Steam: the primary library plus every folder of libraryfolders.vdf.
    std::string steam = readRegistryString(HKEY_CURRENT_USER, "Software\\Valve\\Steam", "SteamPath");
    if (!steam.empty()) {
        std::vector<std::string> libraries = steamLibraries(steam);
        libraries.push_back(steam);
        for (const std::string& library : libraries) {
            std::string candidate = (fs::path(library) / "steamapps" / "common" / "Fallout 2").string();
            candidates.push_back(candidate);
        }
    }

    for (const std::string& candidate : candidates) {
        if (looksLikeFallout2(candidate)) {
            logf("game found: %s", candidate.c_str());
            return candidate;
        }
    }

    if (!allowPicker) return std::string();
    log("the game was not found automatically; pick the Fallout 2 folder in the dialog");
    for (;;) {
        std::string picked = pickFolder();
        if (picked.empty()) return std::string();
        if (looksLikeFallout2(picked)) {
            logf("game found: %s", picked.c_str());
            return picked;
        }
        log("master.dat and critter.dat are not in that folder; pick the Fallout 2 folder again");
    }
}

// --- install and update --------------------------------------------------

// Starts the process and waits for it; false only when it never started.
// The exit code lands in exitCode: the game may leave by any door, the
// launcher still reports its own state afterwards.
// passOutput: the tool writes to this process's stdout/stderr, so the GUI's log pipe sees it.
// Never for the game: it would keep the pipe open for as long as it runs.
bool runTool(const std::wstring& command, const std::wstring& workingDirectory, DWORD* exitCode = nullptr,
    bool passOutput = false)
{
    STARTUPINFOW startup = {};
    startup.cb = sizeof(startup);
    if (passOutput) {
        startup.dwFlags = STARTF_USESTDHANDLES;
        startup.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
        startup.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
        startup.hStdError = GetStdHandle(STD_ERROR_HANDLE);
    }
    PROCESS_INFORMATION process = {};
    std::wstring mutableCommand = command;
    BOOL created = CreateProcessW(nullptr, mutableCommand.data(), nullptr, nullptr, passOutput ? TRUE : FALSE, 0, nullptr,
        workingDirectory.c_str(), &startup, &process);
    if (!created) return false;
    CloseHandle(process.hThread);
    WaitForSingleObject(process.hProcess, INFINITE);
    DWORD code = 1;
    GetExitCodeProcess(process.hProcess, &code);
    CloseHandle(process.hProcess);
    if (exitCode != nullptr) *exitCode = code;
    return true;
}

bool unpackZip(const fs::path& zip, const fs::path& stage)
{
    char systemDirectory[MAX_PATH] = {};
    GetSystemDirectoryA(systemDirectory, MAX_PATH);
    fs::path tar = fs::path(systemDirectory) / "tar.exe";
    if (fs::exists(tar)) {
        std::wstring command = L"\"" + tar.wstring() + L"\" -xf \"" + zip.wstring() + L"\" -C \""
            + stage.wstring() + L"\"";
        if (runTool(command, stage.wstring())) return true;
        log("tar.exe failed; falling back to Expand-Archive");
    }
    std::wstring command = L"powershell -NoProfile -ExecutionPolicy Bypass -Command \"Expand-Archive "
        L"-LiteralPath '" + zip.wstring() + L"' -DestinationPath '" + stage.wstring() + L"' -Force\"";
    return runTool(command, stage.wstring());
}

bool copyIfPresent(const fs::path& stage, const char* name, const fs::path& destination, bool overwrite,
    const char* note)
{
    fs::path source = stage / name;
    if (!fs::exists(source)) {
        logf("  %s: not in the package, skipped", name);
        return false;
    }
    std::error_code error;
    fs::copy_file(source, destination / name,
        overwrite ? fs::copy_options::overwrite_existing : fs::copy_options::none, error);
    if (error) {
        logf("  %s: copy failed (%s)", name, error.message().c_str());
        return false;
    }
    logf("  %s: %s", name, tr(note));
    return true;
}

// A running exe cannot be overwritten but can be renamed: the old copy moves aside
// (deleted on the next run) and the new one takes its name.
bool copyLauncher(const fs::path& source, const fs::path& target)
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

// The wide menu, death and loading art ships as folders next to the exe.
void copyTreeIfPresent(const fs::path& stage, const char* name, const fs::path& destination)
{
    fs::path source = stage / name;
    if (!fs::is_directory(source)) {
        logf("  %s: not in the package, skipped", name);
        return;
    }
    std::error_code error;
    fs::create_directories(destination / name, error);
    fs::copy(source, destination / name,
        fs::copy_options::recursive | fs::copy_options::overwrite_existing, error);
    logf("  %s: %s", name, error ? error.message().c_str() : "installed");
}

const std::vector<std::string>& buildFiles()
{
    static const std::vector<std::string> names = {kGameExe, "ce.dat", kExporterExe, kVersionTxt, kRootLauncher};
    return names;
}

bool backupInstalledBuild(const fs::path& root, const std::string& version, std::string* folderName)
{
    std::string backupVersion = version.empty() ? "manual" : version;
    for (char& ch : backupVersion) {
        if (!(std::isalnum(static_cast<unsigned char>(ch)) || ch == '.' || ch == '-' || ch == '_')) ch = '_';
    }
    if (backupVersion.size() > 100) backupVersion.resize(100);
    fs::path backup = root / "f2reimagined-backups" / backupVersion;
    *folderName = backupVersion;
    std::error_code error;
    fs::create_directories(backup, error);
    if (error) {
        logf("cannot create build backup: %s", error.message().c_str());
        return false;
    }
    bool savedAny = false;
    for (const std::string& name : buildFiles()) {
        fs::path source = root / name;
        if (!fs::is_regular_file(source, error)) continue;
        fs::copy_file(source, backup / name, fs::copy_options::overwrite_existing, error);
        if (error) {
            logf("cannot back up %s: %s", name.c_str(), error.message().c_str());
            return false;
        }
        savedAny = true;
    }
    if (!savedAny) {
        log("no installed build files were available to back up");
        return false;
    }
    if (!fs::exists(backup / kVersionTxt)) {
        std::ofstream versionFile(backup / kVersionTxt, std::ios::binary | std::ios::trunc);
        versionFile << backupVersion;
        if (!versionFile) return false;
    }
    logf("backed up build %s to f2reimagined-backups", version.c_str());
    return true;
}

// What an older build of ours leaves in the game folder and the new one does not need:
// every backup but the newest (rollback only ever takes that one), the launcher under
// its zip name from a hand-unpacked release, and work folders of runs that were cut short.
void cleanOlderBuildLeftovers(const fs::path& root, const std::string& keptBackup)
{
    std::error_code error;
    for (const fs::path& gone : launcher_uninstall::staleBackups(root / "f2reimagined-backups", keptBackup)) {
        fs::remove_all(gone, error);
        logf("  older backup %s: %s", gone.filename().string().c_str(), error ? error.message().c_str() : "removed");
    }
    // Fails quietly while it is the running program; the next start takes it.
    if (fs::remove(root / kZipLauncher, error)) logf("  %s: removed (the launcher is %s now)", kZipLauncher, kRootLauncher);

    char tempPath[MAX_PATH] = {};
    GetTempPathA(MAX_PATH, tempPath);
    std::string own = std::to_string(GetCurrentProcessId());
    auto dayAgo = fs::file_time_type::clock::now() - std::chrono::hours(24);
    for (fs::directory_iterator it(tempPath, error), end; !error && it != end; it.increment(error)) {
        std::string name = it->path().filename().string();
        if (name.rfind("f2ce3d-", 0) != 0 || name.substr(name.rfind('-') + 1) == own) continue;
        std::error_code ignored;
        if (!it->is_directory(ignored) || fs::last_write_time(it->path(), ignored) > dayAgo) continue;
        fs::remove_all(it->path(), ignored);
    }
}

bool rollbackInstalledBuild(const fs::path& root)
{
    std::string currentVersion;
    {
        std::ifstream file(root / kVersionTxt);
        if (file) currentVersion = trim(std::string((std::istreambuf_iterator<char>(file)),
            std::istreambuf_iterator<char>()));
    }
    fs::path backupRoot = root / "f2reimagined-backups";
    fs::path selected;
    std::string selectedVersion;
    fs::file_time_type selectedTime = fs::file_time_type::min();
    std::error_code error;
    if (!fs::is_directory(backupRoot, error)) {
        log("no previous build backup is available");
        return false;
    }
    for (fs::directory_iterator it(backupRoot, error), end; !error && it != end; it.increment(error)) {
        if (!it->is_directory(error)) continue;
        std::ifstream versionFile(it->path() / kVersionTxt);
        if (!versionFile) continue;
        std::string version = trim(std::string((std::istreambuf_iterator<char>(versionFile)),
            std::istreambuf_iterator<char>()));
        if (version.empty() || version == currentVersion) continue;
        fs::file_time_type time = fs::last_write_time(it->path(), error);
        if (error) { error.clear(); continue; }
        if (selected.empty() || time > selectedTime) {
            selected = it->path();
            selectedVersion = version;
            selectedTime = time;
        }
    }
    if (selected.empty()) {
        log("no build different from the installed version is available for rollback");
        return false;
    }
    for (const std::string& name : buildFiles()) {
        fs::path source = selected / name;
        if (!fs::is_regular_file(source, error)) continue;
        if (name == kRootLauncher) {
            if (!copyLauncher(source, root / name)) {
                logf("rollback failed for %s", name.c_str());
                return false;
            }
            continue;
        }
        fs::copy_file(source, root / name, fs::copy_options::overwrite_existing, error);
        if (error) {
            logf("rollback failed for %s: %s", name.c_str(), error.message().c_str());
            return false;
        }
    }
    logf("restored previous build %s", selectedVersion.c_str());
    return true;
}

bool setLanguage(const fs::path& root, const char* language)
{
    fs::path path = root / kCfg;
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        log("cannot set the game language: fallout2.cfg is missing");
        return false;
    }
    std::string input((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    in.close();
    bool found = false;
    std::string output = launcher_uninstall::withLanguage(input, language, &found);
    if (!found) {
        log("cannot set the game language: [system].language was not found");
        return false;
    }
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    out.write(output.data(), static_cast<std::streamsize>(output.size()));
    if (!out) {
        log("cannot write fallout2.cfg");
        return false;
    }
    return true;
}

std::string jsonStringValue(const std::string& json, const char* key);

bool enableRussianMod(const fs::path& root)
{
    fs::path mods = root / "mods";
    fs::create_directories(mods);
    fs::path orderPath = mods / "mods_order.txt";
    std::string input;
    {
        std::ifstream in(orderPath, std::ios::binary);
        if (in) input.assign((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    }
    std::string lower = input;
    std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
    if (lower.find("rpu_russian.dat") == std::string::npos) {
        std::string output;
        std::stringstream lines(input);
        std::string line;
        bool inserted = false;
        while (std::getline(lines, line)) {
            if (!line.empty() && line.back() == '\r') line.pop_back(); // CRLF files must not gain an empty line
            output += line + "\r\n";
            std::string normalized = trim(line);
            std::transform(normalized.begin(), normalized.end(), normalized.begin(),
                [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
            if (!inserted && normalized == "rpu.dat") {
                output += "rpu_russian.dat\r\n";
                inserted = true;
            }
        }
        if (!inserted) output = "rpu_russian.dat\r\n" + output;
        std::ofstream out(orderPath, std::ios::binary | std::ios::trunc);
        out.write(output.data(), static_cast<std::streamsize>(output.size()));
        if (!out) {
            log("cannot update mods/mods_order.txt");
            return false;
        }
    }
    return setLanguage(root, "russian");
}

// The official RPU release tag from github.com (the 302 page, then the api); empty when neither answers.
std::string rpuTagFromGithub()
{
    const char* networkError = nullptr;
    std::string location;
    std::string tag;
    if (httpRedirectLocation(kRpuLatestUrl, &location, &networkError)) {
        const std::string marker = "/releases/tag/";
        size_t at = location.find(marker);
        if (at != std::string::npos) tag = trim(location.substr(at + marker.size()));
    } else {
        logf("  RPU release page failed: %s", networkError != nullptr ? networkError : "network error");
    }
    if (tag.empty()) {
        std::string json;
        if (httpGet(std::string("https://api.github.com/repos/") + kRpuRepo + "/releases/latest", &json, &networkError)) {
            tag = jsonStringValue(json, "tag_name");
        }
    }
    return tag;
}

// rpu_russian.dat from RPU's own zip (github.com, then the api) into dat; false when any step fails.
bool rpuDatFromGithub(const std::string& tag, const fs::path& work, const fs::path& dat)
{
    std::string filename = "rpu_" + tag + "_translations.zip";
    std::vector<launcher_routes::Route> rpuRoutes = {
        { "github.com", std::string("https://github.com/") + kRpuRepo + "/releases/download/" + tag + "/" + filename, "" },
        { "api", std::string("https://api.github.com/repos/") + kRpuRepo + "/releases/tags/" + tag, filename },
    };
    fs::path zip = work / filename;
    logf("downloading the official Russian translation from RPU %s", tag.c_str());
    std::string routeError;
    if (!fetchRoutes(rpuRoutes, nullptr, &zip, &routeError)) return false;
    std::error_code error;
    fs::create_directories(work / "stage", error);
    if (!unpackZip(zip, work / "stage")) {
        log("the Russian translation archive could not be unpacked");
        return false;
    }
    for (fs::recursive_directory_iterator it(work / "stage", error), end; !error && it != end; it.increment(error)) {
        if (it->is_regular_file() && it->path().filename() == "rpu_russian.dat") {
            fs::copy_file(it->path(), dat, fs::copy_options::overwrite_existing, error);
            return !error;
        }
    }
    log("the official translation archive does not contain rpu_russian.dat");
    return false;
}

bool installRussianTranslation(const fs::path& root)
{
    fs::path translation = root / "mods" / "rpu_russian.dat";
    if (!fs::exists(translation)) {
        char tempPath[MAX_PATH] = {};
        GetTempPathA(MAX_PATH, tempPath);
        fs::path work = fs::path(tempPath) / ("f2ce3d-russian-" + std::to_string(GetCurrentProcessId()));
        std::error_code error;
        fs::remove_all(work, error);
        fs::create_directories(work, error);
        if (error) {
            logf("cannot create translation staging folder: %s", error.message().c_str());
            return false;
        }
        fs::path dat = work / "rpu_russian.dat";
        // Route 1 (task 238): the project mirror, which CI fills with the same file and its tag;
        // 6 MB instead of RPU's 134 MB translations zip, and it works through VPNs that block
        // github.com.  Route 2: RPU's own release on github.com.
        std::string mirrorTag;
        std::string mirrorError;
        fetchRoutes({ { "mirror", g_mirrorBase + "rpu-version.txt", "" } }, &mirrorTag, nullptr, &mirrorError);
        mirrorTag = trim(mirrorTag);
        bool got = fetchRoutes({ { "mirror", g_mirrorBase + "rpu_russian.dat", "" } }, nullptr, &dat, &mirrorError);
        std::string source = mirrorTag.empty() ? "?" : mirrorTag;
        if (got && !mirrorTag.empty()) logf("  the mirror's RPU version: %s", mirrorTag.c_str());
        if (!got) {
            std::string tag = rpuTagFromGithub();
            if (!tag.empty()) got = rpuDatFromGithub(tag, work, dat);
            source = "RPU";
            if (!got && tag.empty()) log("cannot read the official RPU release version on GitHub");
            if (!got) logf("the Russian translation could not be downloaded: GitHub and the project mirror both failed (%s)", mirrorError.c_str());
        }
        bool copied = false;
        if (got) {
            fs::create_directories(translation.parent_path(), error);
            fs::copy_file(dat, translation, fs::copy_options::overwrite_existing, error);
            copied = !error;
            if (!copied) log("failed to install the Russian translation file");
        }
        fs::remove_all(work, error);
        if (!copied) return false;
        if (source == "RPU") log("  installed mods/rpu_russian.dat from the official RPU release");
        else logf("  installed mods/rpu_russian.dat from the project mirror (RPU %s)", source.c_str());
    } else {
        log("  kept the existing mods/rpu_russian.dat");
    }
    if (!enableRussianMod(root)) return false;
    log("  game language set to Russian; mods/mods_order.txt updated");
    return true;
}

// --- the Restoration Project (task 186) -------------------------------------

bool rpOnDisk(const fs::path& root)
{
    std::error_code error;
    return fs::exists(root / "mods" / "rpu.dat", error);
}

// One game-folder file into backup/rpu under the same relative path.
bool moveToRpBackup(const fs::path& root, const fs::path& relative)
{
    std::error_code error;
    fs::path target = root / launcher_rp::kBackupFolder / relative;
    fs::create_directories(target.parent_path(), error);
    fs::rename(root / relative, target, error);
    if (error) logf("  cannot move %s aside (%s)", wideToUtf8(relative.generic_wstring()).c_str(), error.message().c_str());
    return !error;
}

// RPU from its own GitHub release at the player's request (never a copy of ours), put in
// place the way its manual package's rpu-install.bat does; every file it moves or replaces
// is kept in backup/rpu.
bool installRestorationProject(const fs::path& root)
{
    if (rpOnDisk(root)) {
        log("  kept the installed Restoration Project (mods/rpu.dat)");
        return true;
    }
    // A test zip (--rp-zip-url) needs no GitHub lookup.
    std::string tag = g_rpZipUrl.empty() ? rpuTagFromGithub() : std::string("test");
    if (tag.empty()) {
        log("cannot read the official RPU release version on GitHub");
        return false;
    }
    char tempPath[MAX_PATH] = {};
    GetTempPathA(MAX_PATH, tempPath);
    fs::path work = fs::path(tempPath) / ("f2ce3d-rp-" + std::to_string(GetCurrentProcessId()));
    std::error_code error;
    fs::remove_all(work, error);
    fs::create_directories(work / "stage", error);
    if (error) {
        logf("cannot create the Restoration Project staging folder: %s", error.message().c_str());
        return false;
    }
    std::string filename = "rpu_" + tag + ".zip";
    std::vector<launcher_routes::Route> routes = {
        { "github.com", std::string("https://github.com/") + kRpuRepo + "/releases/download/" + tag + "/" + filename, "" },
        { "api", std::string("https://api.github.com/repos/") + kRpuRepo + "/releases/tags/" + tag, filename },
    };
    fs::path zip = work / filename;
    logf("downloading the Restoration Project %s from its official release (about 900 MB)", tag.c_str());
    std::string routeError;
    if (!g_rpZipUrl.empty()) routes = { { "test", g_rpZipUrl, "" } };
    if (!fetchRoutes(routes, nullptr, &zip, &routeError)) {
        logf("the Restoration Project could not be downloaded (%s)", routeError.c_str());
        fs::remove_all(work, error);
        return false;
    }
    log("unpacking");
    fs::path stage = work / "stage";
    bool unpacked = unpackZip(zip, stage);
    fs::remove(zip, error);
    if (!unpacked || !fs::exists(stage / "mods" / "rpu.dat")) {
        log("the Restoration Project package could not be unpacked or has no mods/rpu.dat; nothing was changed");
        fs::remove_all(work, error);
        return false;
    }

    log("installing the Restoration Project into the game folder:");
    bool ok = true;
    // As rpu-install.bat: the old patch archives and the loose vanilla data go aside.
    std::vector<fs::path> aside;
    for (fs::directory_iterator it(root, error), end; !error && it != end; it.increment(error)) {
        if (it->is_regular_file() && launcher_rp::movedAside(wideToUtf8(it->path().filename().wstring()))) {
            aside.push_back(it->path().filename());
        }
    }
    error.clear();
    for (fs::recursive_directory_iterator it(root / "data", error), end; !error && it != end; it.increment(error)) {
        if (!it->is_regular_file()) continue;
        fs::path relative = fs::relative(it->path(), root / "data");
        if (!launcher_rp::keptInData(wideToUtf8(relative.generic_wstring()))) aside.push_back(fs::path("data") / relative);
    }
    error.clear(); // a game without data/ is fine
    for (const fs::path& relative : aside) {
        if (!moveToRpBackup(root, relative)) ok = false;
    }
    // The package over the game folder; a file it replaces is kept first.
    // mods/rpu.dat goes last: it marks RP as installed, so a copy cut short is tried again next time.
    std::vector<std::pair<fs::path, std::string>> files;
    for (fs::recursive_directory_iterator it(stage, error), end; !error && it != end; it.increment(error)) {
        if (!it->is_regular_file()) continue;
        std::string target = launcher_rp::targetOf(wideToUtf8(fs::relative(it->path(), stage).generic_wstring()));
        if (!target.empty()) files.push_back({ it->path(), target });
    }
    std::stable_partition(files.begin(), files.end(),
        [](const auto& file) { return launcher_uninstall::lowered(file.second) != "mods/rpu.dat"; });
    int installed = 0;
    for (const auto& [source, target] : files) {
        if (!ok || error) break;
        fs::path destination = root / utf8ToWide(target);
        fs::path kept = root / launcher_rp::kBackupFolder / utf8ToWide(target);
        std::error_code copyError;
        if (fs::exists(destination, copyError) && !fs::exists(kept, copyError)) {
            fs::create_directories(kept.parent_path(), copyError);
            fs::copy_file(destination, kept, copyError);
        }
        if (!copyError) {
            fs::create_directories(destination.parent_path(), copyError);
            fs::copy_file(source, destination, fs::copy_options::overwrite_existing, copyError);
        }
        if (copyError) {
            logf("  %s: copy failed (%s)", target.c_str(), copyError.message().c_str());
            ok = false;
        } else {
            installed++;
        }
    }
    if (error) {
        logf("  cannot read the Restoration Project package (%s)", error.message().c_str());
        ok = false;
    }
    fs::remove_all(work, error);
    if (ok) logf("  installed %d files of RPU %s; what they replaced is in backup/rpu", installed, tag.c_str());
    return ok;
}

// --- removing the mod (task 234) -------------------------------------------

std::string readWholeFile(const fs::path& path)
{
    std::ifstream in(path, std::ios::binary);
    return in ? std::string((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>()) : std::string();
}

// Takes the translation out: its file, its line in mods_order.txt, and the game language back to English.
bool removeRussianTranslation(const fs::path& root)
{
    std::error_code error;
    fs::path translation = root / "mods" / "rpu_russian.dat";
    bool ok = true;
    if (fs::exists(translation, error)) {
        fs::remove(translation, error);
        if (error) {
            log("cannot remove mods/rpu_russian.dat; is the game running? Close it and try again");
            ok = false;
        } else {
            log("  removed mods/rpu_russian.dat");
        }
    }
    fs::path orderPath = root / "mods" / "mods_order.txt";
    if (fs::exists(orderPath, error)) {
        bool removed = false;
        std::string order = launcher_uninstall::removeLine(readWholeFile(orderPath), "rpu_russian.dat", &removed);
        if (removed) {
            if (launcher_uninstall::hasContent(order)) {
                std::ofstream out(orderPath, std::ios::binary | std::ios::trunc);
                out.write(order.data(), static_cast<std::streamsize>(order.size()));
                if (!out) {
                    log("cannot update mods/mods_order.txt");
                    ok = false;
                }
            } else {
                fs::remove(orderPath, error); // the mod made the list: nothing else is in it
            }
            log("  mods/mods_order.txt: the translation line removed");
        }
    }
    fs::remove(root / "mods", error); // only when empty
    if (launcher_uninstall::languageOf(readWholeFile(root / kCfg)) == "russian") {
        if (setLanguage(root, "english")) log("  game language set back to English");
        else ok = false;
    }
    return ok;
}

// Gives the game back as it was before the mod.  Saves (data/), the original game files
// and fallout2.cfg are never touched.  The folder f2reimagined-backups only holds older
// mod builds, so it goes with the mod.
bool uninstallMod(const fs::path& root)
{
    bool ok = true;
    std::error_code error;
    log("removing the mod from the game folder:");
    for (const std::string& name : launcher_uninstall::installedFiles()) {
        if (!fs::exists(root / name, error)) continue;
        fs::remove(root / name, error);
        if (error) {
            logf("  cannot remove %s (%s); is the game running? Close it and try again", name.c_str(), error.message().c_str());
            ok = false;
        } else {
            logf("  removed %s", name.c_str());
        }
    }
    for (const std::string& name : launcher_uninstall::installedFolders()) {
        if (!fs::exists(root / name, error)) continue;
        fs::remove_all(root / name, error);
        if (error) {
            logf("  cannot remove the folder %s (%s)", name.c_str(), error.message().c_str());
            ok = false;
        } else {
            logf("  removed the folder %s", name.c_str());
        }
    }
    fs::remove(root / "user-assets", error); // only when nothing of the player's is left in it
    if (!removeRussianTranslation(root)) ok = false;

    // The launcher itself, last: if it is the running program, it moves aside and a helper deletes it after we close.
    fs::path launcher = root / kRootLauncher;
    if (fs::exists(launcher, error)) {
        fs::remove(launcher, error);
        if (!error) {
            logf("  removed %s", kRootLauncher);
        } else {
            fs::path aside = launcher;
            aside += ".old";
            fs::remove(aside, error);
            fs::rename(launcher, aside, error);
            if (error) {
                logf("  cannot remove %s (%s)", kRootLauncher, error.message().c_str());
                ok = false;
            } else {
                // cmd repeats the delete until the running copy is closed; it stops by itself at the latest after ten minutes.
                std::wstring command = L"cmd.exe /c \"for /l %i in (1,1,600) do @(del /f /q \"" + aside.wstring()
                    + L"\" 2>nul & if not exist \"" + aside.wstring() + L"\" exit & ping -n 2 127.0.0.1 >nul)\"";
                STARTUPINFOW startup = {};
                startup.cb = sizeof(startup);
                PROCESS_INFORMATION process = {};
                if (CreateProcessW(nullptr, command.data(), nullptr, nullptr, FALSE, CREATE_NO_WINDOW | DETACHED_PROCESS,
                        nullptr, root.c_str(), &startup, &process)) {
                    CloseHandle(process.hThread);
                    CloseHandle(process.hProcess);
                }
                logf("  %s: removed (the last file goes when this window closes)", kRootLauncher);
            }
        }
    }
    log(ok ? "the mod is removed; your saves and the original Fallout 2 files were not touched"
           : "the mod is only partly removed; see the lines above");
    return ok;
}

// --- command line --------------------------------------------------------

// The release base may come with a trailing slash; the asset names do not.
std::string joinUrl(const std::string& base, const char* name)
{
    std::string out = base;
    while (!out.empty() && out.back() == '/') out.pop_back();
    out += "/";
    out += name;
    return out;
}

std::string jsonStringValue(const std::string& json, const char* key)
{
    std::string needle = std::string("\"") + key + "\"";
    size_t pos = json.find(needle);
    if (pos == std::string::npos) return std::string();
    pos = json.find(':', pos + needle.size());
    if (pos == std::string::npos) return std::string();
    pos = json.find('"', pos + 1);
    if (pos == std::string::npos) return std::string();
    std::string value;
    for (size_t i = pos + 1; i < json.size(); i++) {
        char ch = json[i];
        if (ch == '"') return value;
        if (ch == '\\' && i + 1 < json.size()) {
            char escaped = json[++i];
            switch (escaped) {
            case 'n': value.push_back('\n'); break;
            case 'r': value.push_back('\r'); break;
            case 't': value.push_back('\t'); break;
            case '\\': value.push_back('\\'); break;
            case '"': value.push_back('"'); break;
            default: value.push_back(escaped); break;
            }
        } else {
            value.push_back(ch);
        }
    }
    return std::string();
}

std::string shortNotes(const std::string& body)
{
    std::stringstream lines(body);
    std::string line;
    std::string result;
    int kept = 0;
    while (std::getline(lines, line) && kept < 2) {
        line = trim(line);
        while (!line.empty() && (line[0] == '#' || line[0] == '*' || line[0] == '-' || line[0] == ' ')) {
            line.erase(line.begin());
        }
        line = trim(line);
        if (line.empty()) continue;
        if (!result.empty()) result += " | ";
        result += line;
        kept++;
    }
    return result;
}

bool fetchReleaseInfo(const std::string& updateBase, const std::string& releaseInfoUrl,
    std::string* version, std::string* notes, std::string* routeError)
{
    const char* error = nullptr;
    std::string versionBody;
    if (!fetchRoutes(releaseRoutes(updateBase, kVersionTxt), &versionBody, nullptr, routeError)) return false;
    *version = trim(versionBody);
    notes->clear();
    if (!releaseInfoUrl.empty()) {
        std::string response;
        if (httpGet(releaseInfoUrl, &response, &error)) {
            *notes = shortNotes(jsonStringValue(response, "body"));
        } else {
            logf("release notes are unavailable (GitHub API rate limit or network: %s)",
                error != nullptr ? error : "network error");
        }
    }
    return !version->empty();
}

Options parseOptions(int argc, char** argv)
{
    Options options;
    bool passthrough = false;
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (passthrough) {
            options.gameArgs.push_back(arg);
        } else if (arg == "--") {
            passthrough = true;
        } else if (arg == "--help" || arg == "-h") {
            log("fallout2-ce-3d-launcher: installs Fallout 2: Reimagined onto a clean Fallout 2 and keeps");
            log("it updated from the rolling `latest` GitHub release, then starts the game.");
            log("");
            log("  --game-root <dir>   the Fallout 2 folder (default: this launcher's own folder when it is a game root, else registry lookup, then a picker)");
            log("  --update-url <url>  where version.txt and the zip live (default: the latest release)");
            log("  --release-info-url <url>  release API for the version notes");
            log("  --maps <list>       maps the exporter covers, comma separated (default: all, every map)");
            log("  --force-export      rebuild all the 3D scene data even if it is current");
            log("  --no-start          do everything except starting the game");
            log("  --rollback          restore the most recent backed-up game build");
            log("  --russian           install the official RPU Russian text translation");
            log("  --remove-russian    remove the Russian translation and set the game language back to English");
            log("  --restoration-project  install the Restoration Project from its official release and build its 3D scenes");
            log("  --ui-language <ru|en>  the language of the log lines (default: en)");
            log("  --mirror-url <url>  the mirror folder for the release files and the Russian translation (tests)");
            log("  --rp-zip-url <url>  the Restoration Project zip from this url instead of its release (tests)");
            log("  --uninstall         remove the mod and give back the game as it was (saves and original files stay)");
            log("  --check-release     print the latest version and release notes");
            log("  -- <args>           arguments handed over to the game");
            std::exit(0);
        } else if (arg == "--game-root" && i + 1 < argc) {
            options.gameRoot = argv[++i];
        } else if (arg == "--update-url" && i + 1 < argc) {
            options.updateBase = argv[++i];
        } else if (arg == "--release-info-url" && i + 1 < argc) {
            options.releaseInfoUrl = argv[++i];
        } else if (arg == "--maps" && i + 1 < argc) {
            options.maps = argv[++i];
        } else if (arg == "--no-start") {
            options.noStart = true;
        } else if (arg == "--force-export") {
            options.forceExport = true;
        } else if (arg == "--rollback") {
            options.rollback = true;
        } else if (arg == "--russian") {
            options.installRussian = true;
        } else if (arg == "--remove-russian") {
            options.removeRussian = true;
        } else if (arg == "--restoration-project") {
            options.installRp = true;
        } else if (arg == "--rp-zip-url" && i + 1 < argc) {
            g_rpZipUrl = argv[++i];
        } else if (arg == "--ui-language" && i + 1 < argc) {
            g_ru = std::string(argv[++i]) == "ru";
        } else if (arg == "--mirror-url" && i + 1 < argc) {
            g_mirrorBase = argv[++i];
            if (g_mirrorBase.back() != '/') g_mirrorBase += '/';
        } else if (arg == "--uninstall") {
            options.uninstall = true;
        } else if (arg == "--check-release") {
            options.checkRelease = true;
        } else {
            die("unknown option: %s (see --help)", arg.c_str());
        }
    }
    return options;
}

std::vector<std::string> splitMaps(const std::string& maps)
{
    std::vector<std::string> out;
    std::stringstream stream(maps);
    std::string item;
    while (std::getline(stream, item, ',')) {
        std::string name = trim(item);
        if (!name.empty()) out.push_back(name);
    }
    return out;
}

} // namespace

// The folder of this exe when it is a game root (the launcher installed there), else empty.
std::string ownGameRoot()
{
    wchar_t path[MAX_PATH] = {};
    if (GetModuleFileNameW(nullptr, path, MAX_PATH) == 0) return std::string();
    fs::path folder = fs::path(path).parent_path();
    std::error_code error;
    if (!looksLikeFallout2(folder) || !fs::exists(folder / kVersionTxt, error)) return std::string();
    return wideToUtf8(folder.wstring());
}

std::string findGameRootForUi()
{
    return findGameRoot(false);
}

int runLauncherUi(int argc, char** argv);

int runConsole(Options options)
{
    log("fallout2-ce-3d-launcher");
    log("");

    if (options.checkRelease) {
        std::string version;
        std::string notes;
        std::string routeError;
        if (!fetchReleaseInfo(options.updateBase, options.releaseInfoUrl, &version, &notes, &routeError)) {
            logf("RELEASE_ERROR:%s", routeError.c_str());
            log("release check failed: GitHub does not answer");
            return 1;
        }
        logf("RELEASE_VERSION:%s", version.c_str());
        logf("RELEASE_NOTES:%s", notes.c_str());
        return 0;
    }

    // --- the game folder ---
    fs::path root;
    if (!options.gameRoot.empty()) {
        root = fs::path(utf8ToWide(options.gameRoot));
        logf("game root (from --game-root): %s", options.gameRoot.c_str());
        if (!looksLikeFallout2(root)) die("this is not a Fallout 2 folder: master.dat and critter.dat are not there");
    } else {
        std::string found = ownGameRoot();
        if (!found.empty()) logf("game root (the folder of this launcher): %s", found.c_str());
        else found = findGameRoot(true);
        if (found.empty()) die("no Fallout 2 folder was chosen");
        root = found;
    }

    {
        std::error_code ignored;
        fs::path old = root / kRootLauncher;
        old += ".old";
        fs::remove(old, ignored);
    }

    if (options.uninstall) return uninstallMod(root) ? 0 : 1;
    if (options.removeRussian) {
        log("removing the Russian translation:");
        return removeRussianTranslation(root) ? 0 : 1;
    }
    if (options.rollback) return rollbackInstalledBuild(root) ? 0 : 1;

    bool installedGamePresent = fs::exists(root / kGameExe);
    std::string installedVersion;
    {
        std::ifstream file(root / kVersionTxt);
        if (file) installedVersion = trim(std::string((std::istreambuf_iterator<char>(file)),
            std::istreambuf_iterator<char>()));
    }
    if (!installedVersion.empty()) {
        logf("installed build: %s", installedVersion.c_str());
    } else if (installedGamePresent) {
        log("installed build: an older manual install (no version.txt)");
    } else {
        log("installed build: none, fresh install");
    }

    // --- what does the release hold? ---
    std::string remoteVersion;
    std::string routeError;
    if (!fetchRoutes(releaseRoutes(options.updateBase, kVersionTxt), &remoteVersion, nullptr, &routeError)) {
        remoteVersion.clear();
        logf("GitHub does not answer (%s)", routeError.c_str());
    } else {
        remoteVersion = trim(remoteVersion);
        logf("release build: %s", remoteVersion.c_str());
    }

    if (remoteVersion.empty()) {
        if (installedVersion.empty() && !installedGamePresent) {
            die("GitHub does not answer, and the first install needs the internet. Try again later or use another network");
        }
        log("GitHub does not answer: the installed game is used as it is");
    }

    bool freshInstall = !installedGamePresent;
    bool updated = false;
    // An older launcher installed the build without the wide art folders: pull once more for them.
    // The picture itself, not its folder: an install over an older build left the folders empty.
    std::error_code artError;
    bool wideArtMissing = fs::file_size(root / launcher_uninstall::kWideMenuArt, artError) == 0 || artError;
    if (!remoteVersion.empty() && (remoteVersion != installedVersion || wideArtMissing)) {
        // --- download and unpack ---
        if (installedVersion.empty()) logf("installing: none -> %s", remoteVersion.c_str());
        else logf("updating: %s -> %s", installedVersion.c_str(), remoteVersion.c_str());
        char tempPath[MAX_PATH] = {};
        GetTempPathA(MAX_PATH, tempPath);
        fs::path work = fs::path(tempPath) / ("f2ce3d-" + std::to_string(GetCurrentProcessId()));
        std::error_code error;
        fs::remove_all(work, error);
        fs::create_directories(work, error);
        fs::path zip = work / kZipAsset;
        fs::path stage = work / "stage";
        fs::create_directories(stage, error);

        std::string downloadError;
        logf("downloading %s", kZipAsset);
        if (!fetchRoutes(releaseRoutes(options.updateBase, kZipAsset), nullptr, &zip, &downloadError)) {
            fs::remove_all(work, error);
            die("GitHub does not answer, download failed (%s)", downloadError.c_str());
        }
        log("unpacking");
        if (!unpackZip(zip, stage)) {
            fs::remove_all(work, error);
            die("the downloaded file could not be unpacked; try the install again");
        }
        if (!fs::exists(stage / kGameExe)) {
            fs::remove_all(work, error);
            die("the downloaded package is damaged (the game is not in it); try the install again");
        }

        std::string keptBackup;
        // A refill of the wide art at the same version is no update: the older build stays the one to roll back to.
        if (installedGamePresent && remoteVersion != installedVersion && !backupInstalledBuild(root,
            installedVersion.empty() ? std::string("manual") : installedVersion, &keptBackup)) {
            fs::remove_all(work, error);
            die("could not save the current build before updating; nothing was changed");
        }

        // --- into the game folder; the player's things stay theirs ---
        log("installing into the game folder:");
        copyIfPresent(stage, kGameExe, root, true, "installed");
        copyIfPresent(stage, "ce.dat", root, true, "installed");
        copyIfPresent(stage, kExporterExe, root, true, "installed");
        if (!fs::exists(stage / kZipLauncher)) {
            logf("  %s: not in the package, skipped", kRootLauncher);
        } else if (copyLauncher(stage / kZipLauncher, root / kRootLauncher)) {
            logf("  %s: installed", kRootLauncher);
        } else {
            logf("  %s: copy failed", kRootLauncher);
        }
        copyTreeIfPresent(stage, "user-assets", root);
        copyTreeIfPresent(stage, "data/art/splash", root);
        copyTreeIfPresent(stage, "data/art/russian/splash", root);
        if (fs::exists(root / kCfg)) {
            log("  fallout2.cfg: kept (the player's file is never overwritten)");
        } else {
            copyIfPresent(stage, kCfg, root, false, "installed (default)");
        }
        {
            std::ofstream file(root / kVersionTxt, std::ios::binary | std::ios::trunc);
            file << remoteVersion;
        }
        logf("  version.txt: %s", remoteVersion.c_str());
        fs::remove_all(work, error);
        cleanOlderBuildLeftovers(root, keptBackup);
        updated = true;
        installedVersion = remoteVersion;
    } else if (!remoteVersion.empty()) {
        logf("the installed build matches the release: nothing to pull");
    }

    // --- the Restoration Project, before the scenes: its maps and art go into them ---
    bool rpInstalledNow = false;
    if (options.installRp && !rpOnDisk(root)) {
        if (!installRestorationProject(root)) {
            log("the Restoration Project could not be installed (see the lines above)");
            return 1;
        }
        rpInstalledNow = true;
    }

    // --- 3D scene data, always made on this machine from this game ---
    // RP's own maps are the exporter's "rp" group, outside "all".
    std::vector<std::string> maps = splitMaps(options.maps == kDefaultMaps && rpOnDisk(root) ? "all,rp" : options.maps);
    std::string mapList;
    for (size_t i = 0; i < maps.size(); i++) {
        if (i != 0) mapList += ",";
        mapList += maps[i];
    }
    fs::path exporter = root / kExporterExe;
    if (!fs::exists(exporter)) {
        log("scene-exporter.exe is missing; the game will start without 3D");
    } else {
        bool needExport = options.forceExport || freshInstall || updated || rpInstalledNow;
        if (!needExport) {
            // The exporter owns the map groups: "all" is its name, only
            // it knows which scene directories that covers.  Exit 0 = every
            // base scene exists, anything else = make them.
            std::string command = "\"" + exporter.string() + "\" --game-root \"" + root.string()
                + "\" --maps " + mapList + " --check";
            DWORD checkCode = 1;
            runTool(utf8ToWide(command), root.wstring(), &checkCode);
            needExport = checkCode != 0;
        }
        if (needExport) {
            std::string command = "\"" + exporter.string() + "\" --game-root \"" + root.string()
                + "\" --maps " + mapList;
            if (options.forceExport) command += " --force";
            logf("making the 3D scene data for: %s", mapList.c_str());
            if (freshInstall) log("  the first install builds every map: about 20 minutes; later updates take seconds");
            if (!runTool(utf8ToWide(command), root.wstring(), nullptr, true)) {
                log("the 3D data could not be built; the game will start without 3D");
            }
        } else {
            log("the 3D scene data is current");
        }
    }
    if (!installedVersion.empty()) logf("the game folder holds build %s", installedVersion.c_str());

    if (options.installRussian && !installRussianTranslation(root)) {
        // Exit 2: the game itself is installed, only the translation will be tried again.
        log("the Russian translation could not be installed (see the lines above)");
        return 2;
    }

    if (options.noStart) {
        log("done (--no-start)");
        return 0;
    }

    // --- hashes of the player's things, before the run ---
    std::string cfgBefore = "<missing>";
    std::string savesBefore;
    if (fs::exists(root / kCfg)) sha256File(root / kCfg, &cfgBefore);
    sha256Tree(root / "data" / "savegame", &savesBefore);
    logf("fallout2.cfg sha256 before: %s", cfgBefore.c_str());
    logf("data/savegame sha256 before: %s", savesBefore.c_str());

    // --- start the game ---
    fs::path game = root / kGameExe;
    if (!fs::exists(game)) die("the game is not installed yet; press Install first");
    std::string command = "\"" + game.string() + "\"";
    for (const std::string& argument : options.gameArgs) {
        command += " ";
        if (argument.find(' ') != std::string::npos) command += "\"" + argument + "\"";
        else command += argument;
    }
    logf("starting %s", game.string().c_str());
    DWORD gameExitCode = 0;
    if (!runTool(utf8ToWide(command), root.wstring(), &gameExitCode)) {
        die("the game could not be started");
    }
    logf("the game exited with code %lu", static_cast<unsigned long>(gameExitCode));

    // --- and after: must be byte for byte the same ---
    std::string cfgAfter = "<missing>";
    std::string savesAfter;
    if (fs::exists(root / kCfg)) sha256File(root / kCfg, &cfgAfter);
    sha256Tree(root / "data" / "savegame", &savesAfter);
    logf("fallout2.cfg sha256 after:  %s (%s)", cfgAfter.c_str(),
        cfgAfter == cfgBefore ? "unchanged" : "CHANGED");
    logf("data/savegame sha256 after:  %s (%s)", savesAfter.c_str(),
        savesAfter == savesBefore ? "unchanged" : "CHANGED");
    log("bye");
    return 0;
}

int main(int argc, char** argv)
{
    if (argc == 1 || (argc >= 2 && std::string(argv[1]) == "--ui")) {
        return runLauncherUi(argc, argv);
    }
    return runConsole(parseOptions(argc, argv));
}
