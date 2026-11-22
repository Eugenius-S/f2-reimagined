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
#include <iomanip>
#include <map>
#include <set>
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
#include "launcher_voices.h"
#include "launcher_translation_urls.h"

namespace fs = std::filesystem;

bool supportedVoiceRp(const fs::path& root);
bool pinnedRpDat(const fs::path& root);
bool knownActiveManualVoice(const fs::path& root);

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
constexpr const char* kOfficialRussianHash = "f32d9d0b2696a5066a05a13e431c8dfc9d0772c058932121209049c374d8a8c7";
constexpr const char* kVoiceRpTransition = ".f2r-voice-rp-transition";
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
    std::string textTranslation;
    std::string voiceTranslation;
    bool textWithInstall = false;
    bool recoverTextOnly = false;
    bool installRp = false;
    bool removeRp = false;
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
    BCRYPT_ALG_HANDLE algorithm = nullptr;
    if (BCryptOpenAlgorithmProvider(&algorithm, BCRYPT_SHA256_ALGORITHM, nullptr, 0) != 0) return false;
    BCRYPT_HASH_HANDLE hash = nullptr;
    bool ok = BCryptCreateHash(algorithm, &hash, nullptr, 0, nullptr, 0, 0) == 0;
    char buffer[65536];
    while (ok && file) {
        file.read(buffer, sizeof(buffer));
        if (file.gcount() > 0) ok = BCryptHashData(hash, reinterpret_cast<PUCHAR>(buffer), static_cast<ULONG>(file.gcount()), 0) == 0;
    }
    unsigned char digest[32];
    ok = ok && file.eof();
    if (ok) ok = BCryptFinishHash(hash, digest, 32, 0) == 0;
    if (hash != nullptr) BCryptDestroyHash(hash);
    BCryptCloseAlgorithmProvider(algorithm, 0);
    if (ok) *hex = hashToHex(digest);
    return ok;
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
        ok = launcher_routes::downloadComplete(total, contentLength);
        if (ok) {
            logf("  downloaded %zu bytes", total);
        } else {
            *errorText = total == 0 ? "empty download" : "the download was cut short";
            // A stump left in place would be taken for the whole file by the next run.
            std::error_code ignored;
            fs::remove(destination, ignored);
        }
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
        DWORD code = 1;
        if (runTool(command, stage.wstring(), &code, true) && code == 0) return true;
        log("tar.exe failed; falling back to Expand-Archive");
        // A failed extractor may have left files behind; the fallback starts clean.
        std::error_code error;
        fs::remove_all(stage, error);
        if (error) return false;
        fs::create_directories(stage, error);
        if (error) return false;
    }
    std::wstring command = L"powershell -NoProfile -ExecutionPolicy Bypass -Command \"Expand-Archive "
        L"-LiteralPath '" + zip.wstring() + L"' -DestinationPath '" + stage.wstring() + L"' -Force -ErrorAction Stop\"";
    DWORD code = 1;
    return runTool(command, stage.wstring(), &code, true) && code == 0;
}

bool copyIfPresent(const fs::path& stage, const char* name, const fs::path& destination, bool overwrite,
    const char* note)
{
    fs::path source = stage / name;
    std::error_code error;
    bool present = fs::exists(source, error);
    if (!error && !present) {
        logf("  %s: not in the package, skipped", name);
        return true;
    }
    if (!error) fs::copy_file(source, destination / name,
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
    if (error) logf("  %s: copy failed (%s)", kRootLauncher, error.message().c_str());
    if (error && moved) {
        std::error_code ignored;
        fs::rename(aside, target, ignored);
    }
    return !error;
}

// The wide menu, death and loading art ships as folders next to the exe.
bool copyTreeIfPresent(const fs::path& stage, const char* name, const fs::path& destination)
{
    fs::path source = stage / name;
    std::error_code error;
    bool present = fs::exists(source, error);
    if (!error && !present) {
        logf("  %s: not in the package, skipped", name);
        return true;
    }
    if (!error && !fs::is_directory(source, error)) error = std::make_error_code(std::errc::not_a_directory);
    if (!error) fs::create_directories(destination / name, error);
    if (!error) fs::copy(source, destination / name,
        fs::copy_options::recursive | fs::copy_options::overwrite_existing, error);
    if (error) {
        logf("  %s: copy failed (%s)", name, error.message().c_str());
        return false;
    }
    logf("  %s: installed", name);
    return true;
}

// Refuse a stamp we already know cannot be replaced before copying a
// payload. A newly acquired lock can still race the final atomic rename.
bool canReplaceBuildVersion(const fs::path& root)
{
    const fs::path target = root / kVersionTxt;
    HANDLE file = CreateFileW(target.c_str(), DELETE | FILE_READ_ATTRIBUTES,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr,
        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) return GetLastError() == ERROR_FILE_NOT_FOUND;
    BY_HANDLE_FILE_INFORMATION info {};
    const bool writable = GetFileInformationByHandle(file, &info)
        && (info.dwFileAttributes & (FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_DIRECTORY)) == 0;
    CloseHandle(file);
    return writable;
}

bool writeBuildVersion(const fs::path& root, const std::string& version)
{
    fs::path target = root / kVersionTxt;
    fs::path temporary = target;
    temporary += ".new-" + std::to_string(GetCurrentProcessId());
    std::ofstream file(temporary, std::ios::binary | std::ios::trunc);
    file << version;
    file.close();
    // Replace only a fully written stamp, keeping the old version on any failure.
    bool written = file && MoveFileExW(temporary.c_str(), target.c_str(),
        MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH);
    if (!written) {
        log("  version.txt: write failed");
        std::error_code ignored;
        fs::remove(temporary, ignored);
    }
    return written;
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
bool effectiveModOrder(const fs::path& root, std::string* order);
std::string readWholeFile(const fs::path& path);
bool writeTextFile(const fs::path& path, const std::string& value);
std::string insertOfficialOrder(const std::string& input);

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
    std::error_code error;
    if (!fs::exists(orderPath, error)) {
        if (error || !effectiveModOrder(root, &input)) return false;
        std::ofstream out(orderPath, std::ios::binary | std::ios::trunc);
        out.write(input.data(), static_cast<std::streamsize>(input.size()));
        if (!out) return false;
    }
    bool active = false;
    launcher_uninstall::removeModLine(input, "rpu_russian.dat", &active);
    if (!active) {
        std::string output = insertOfficialOrder(input);
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
            std::string hash;
            if (!sha256File(dat, &hash) || hash != kOfficialRussianHash) got = false;
        }
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
        std::string hash;
        if (!sha256File(translation, &hash) || hash != kOfficialRussianHash) {
            log("the existing RPU translation is not recognized; it was left untouched");
            return false;
        }
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

constexpr const char* kRpManifest = "backup/rpu/f2r-manifest.txt";
constexpr const char* kRpOrder = "backup/rpu/f2r-order.txt";

struct RpFile {
    std::string path;
    std::string original = "-";
    std::string installed = "-";
    bool autoOriginal = false;
};
struct RpManifest {
    bool pending = true;
    bool restored = false;
    bool installing = true;
    std::vector<RpFile> files;
    std::set<std::string> directories;
};

// A package or edited manifest must never escape the game, alias a file, or touch saves.
bool rpRelativePath(const std::string& path)
{
    if (std::any_of(path.begin(), path.end(), [](unsigned char ch) { return ch < 32 || ch == 127; })) return false;
    if (path.empty() || path.find_first_of("\\:*?\"<>|\r\n\t") != std::string::npos) return false;
    std::string lower = launcher_uninstall::lowered(path);
    if (lower == "backup" || lower.rfind("backup/", 0) == 0
        || lower == "data/savegame" || lower.rfind("data/savegame/", 0) == 0
        || lower == "master.dat" || lower == "critter.dat") return false;
    std::stringstream parts(lower);
    std::string part;
    while (std::getline(parts, part, '/')) {
        if (part.empty() || part == "." || part == ".." || part.back() == '.' || part.back() == ' ') return false;
        std::string base = part.substr(0, part.find('.'));
        if (base == "con" || base == "nul" || base == "aux" || base == "prn"
            || (base.size() == 4 && (base.rfind("com", 0) == 0 || base.rfind("lpt", 0) == 0)
                && base[3] >= '1' && base[3] <= '9')) return false;
    }
    return path.back() != '/';
}

// Lexical paths are relative and normalized above; reject every child reparse point too.
bool rpContained(const fs::path& root, const fs::path& relative)
{
    fs::path current = root;
    for (const auto& part : relative) {
        current /= part;
        DWORD attributes = GetFileAttributesW(current.c_str());
        if (attributes == INVALID_FILE_ATTRIBUTES) {
            DWORD error = GetLastError();
            if (error != ERROR_FILE_NOT_FOUND && error != ERROR_PATH_NOT_FOUND) return false;
        } else if (attributes & FILE_ATTRIBUTE_REPARSE_POINT) return false;
    }
    return true;
}

bool rpPresent(const fs::path& path)
{
    return GetFileAttributesW(path.c_str()) != INVALID_FILE_ATTRIBUTES;
}

fs::path rpOriginal(const fs::path& root, const std::string& path)
{
    return root / launcher_rp::kBackupFolder / "originals" / utf8ToWide(path);
}

void replaceRpFile(const fs::path& temp, const fs::path& destination, const std::string& expected, std::error_code& error)
{
    std::string hash;
    if (!error && (!sha256File(temp, &hash) || hash != expected)) error = std::make_error_code(std::errc::io_error);
    if (!error && !MoveFileExW(temp.c_str(), destination.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
        error = std::error_code(GetLastError(), std::system_category());
    if (!error && (!sha256File(destination, &hash) || hash != expected)) error = std::make_error_code(std::errc::io_error);
}

bool writeRpManifest(const fs::path& root, const RpManifest& manifest)
{
    fs::path path = root / kRpManifest;
    fs::path temp = path;
    temp += ".tmp";
    if (!rpContained(root, temp.lexically_relative(root))) return false;
    std::error_code error;
    fs::remove(temp, error);
    if (error) return false;
    std::ofstream out(temp, std::ios::binary | std::ios::trunc);
    out << "F2R-RP-1 " << (manifest.restored ? "restored" : manifest.installing ? "installing" : manifest.pending ? "pending" : "installed") << '\n';
    for (const auto& file : manifest.files)
        out << "F " << file.original << ' ' << file.installed << ' ' << std::quoted(file.path) << (file.autoOriginal ? " auto" : "") << '\n';
    for (const auto& directory : manifest.directories) out << "D " << std::quoted(directory) << '\n';
    out.close();
    return out && MoveFileExW(temp.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH);
}

bool readRpManifest(const fs::path& root, RpManifest* manifest)
{
    if (!rpContained(root, kRpManifest) || !rpContained(root, kRpOrder)
        || !rpContained(root, fs::path(launcher_rp::kBackupFolder) / "f2r-restore.tmp")) return false;
    std::ifstream in(root / kRpManifest, std::ios::binary);
    std::string line;
    if (!std::getline(in, line) || (line != "F2R-RP-1 installing" && line != "F2R-RP-1 pending" && line != "F2R-RP-1 installed" && line != "F2R-RP-1 restored")) return false;
    manifest->installing = line == "F2R-RP-1 installing";
    manifest->pending = line == "F2R-RP-1 pending" || manifest->installing;
    manifest->restored = line == "F2R-RP-1 restored";
    std::set<std::string> names;
    auto hash = [](const std::string& value) {
        return value == "-" || (value.size() == 64 && value.find_first_not_of("0123456789abcdef") == std::string::npos);
    };
    while (std::getline(in, line)) {
        std::istringstream row(line);
        char type;
        RpFile file;
        if (!(row >> type)) return false;
        if (type == 'F') {
            if (!(row >> file.original >> file.installed >> std::quoted(file.path))
                || !hash(file.original) || !hash(file.installed) || (file.original == "-" && file.installed == "-")) return false;
            row >> std::ws;
            if (!row.eof()) {
                std::string flag;
                if (!(row >> flag) || flag != "auto" || launcher_uninstall::lowered(file.path) != "mods/mods_order.txt"
                    || file.original == "-") return false;
                file.autoOriginal = true;
            }
        } else if (type == 'D') {
            if (!(row >> std::quoted(file.path))) return false;
        } else return false;
        row >> std::ws;
        std::string name = launcher_uninstall::lowered(file.path);
        if (!row.eof() || !rpRelativePath(file.path) || !rpContained(root, utf8ToWide(file.path))
            || !names.insert(name).second) return false;
        if (type == 'F') {
            if (!rpContained(root, fs::path(launcher_rp::kBackupFolder) / "originals" / utf8ToWide(file.path))) return false;
            manifest->files.push_back(file);
        } else manifest->directories.insert(file.path);
    }
    std::set<std::string> files;
    for (const auto& file : manifest->files) files.insert(launcher_uninstall::lowered(file.path));
    for (const auto& name : names) {
        for (fs::path parent = fs::path(name).parent_path(); !parent.empty(); parent = parent.parent_path())
            if (files.count(parent.generic_string())) return false;
    }
    return in.eof() && files.count("mods/rpu.dat");
}

std::string rpRead(const fs::path& path)
{
    std::ifstream in(path, std::ios::binary);
    return std::string(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
}

// The restored receipt survives a failed backup cleanup, so retry needs no deleted originals.
bool cleanupRp(const fs::path& root, const RpManifest& manifest)
{
    for (const auto& file : manifest.files) {
        fs::path path = rpOriginal(root, file.path);
        if (file.original == "-" || !rpPresent(path)) continue;
        std::string hash;
        if (!sha256File(path, &hash) || hash != file.original) {
            logf("RP restore: invalid or missing backup for %s; backup/rpu was kept", file.path.c_str());
            return false;
        }
    }
    bool ok = true;
    auto remove = [&](const fs::path& path) {
        std::error_code error;
        fs::remove(path, error);
        if (error) {
            logf("RP cleanup: cannot remove %s (%s); the restore receipt was kept",
                wideToUtf8(path.lexically_relative(root).generic_wstring()).c_str(), error.message().c_str());
            ok = false;
        }
    };
    for (const auto& file : manifest.files) if (file.original != "-") remove(rpOriginal(root, file.path));
    remove(root / kRpOrder);
    remove(root / launcher_rp::kBackupFolder / "f2r-restore.tmp");
    remove(fs::path(root / kRpManifest).wstring() + L".tmp");
    if (!ok) return false;
    remove(root / kRpManifest);
    if (!ok) return false;
    std::error_code error;
    for (auto it = manifest.directories.rbegin(); it != manifest.directories.rend(); ++it)
        if (fs::is_directory(root / utf8ToWide(*it), error)) fs::remove(root / utf8ToWide(*it), error);
    // Only known backup parents are ours; unrelated empty folders and junctions stay.
    std::set<fs::path> directories;
    const fs::path backup = root / launcher_rp::kBackupFolder;
    for (const auto& file : manifest.files) {
        if (file.original == "-") continue;
        for (fs::path parent = rpOriginal(root, file.path).parent_path(); parent != backup; parent = parent.parent_path())
            directories.insert(parent);
    }
    for (auto it = directories.rbegin(); it != directories.rend(); ++it) fs::remove(*it, error);
    fs::remove(backup, error);
    log("the Restoration Project was removed; original files restored and saves kept");
    return true;
}

// Both a failed copy and player removal restore the same saved originals.
bool restoreRp(const fs::path& root, RpManifest manifest, bool cleanup = true)
{
    if (manifest.restored) return cleanup ? cleanupRp(root, manifest) : true;
    for (const auto& directory : manifest.directories) {
        fs::path path = root / utf8ToWide(directory);
        std::error_code error;
        if (rpPresent(path) && !fs::is_directory(path, error)) {
            logf("RP restore: %s was changed by the player; backup/rpu was kept", directory.c_str());
            return false;
        }
    }
    std::map<std::string, std::string> sharedOrders;
    for (const auto& file : manifest.files) {
        std::string original;
        if (file.original != "-" && (!sha256File(rpOriginal(root, file.path), &original) || original != file.original)) {
            logf("RP restore: invalid or missing backup for %s; backup/rpu was kept", file.path.c_str());
            return false;
        }
        fs::path destination = root / utf8ToWide(file.path);
        if (!rpPresent(destination)) continue;
        std::string current;
        std::error_code error;
        if (!fs::is_regular_file(destination, error) || !sha256File(destination, &current)) {
            logf("RP restore: cannot read %s; backup/rpu was kept", file.path.c_str());
            return false;
        }
        if (launcher_uninstall::lowered(file.path) == "mods/mods_order.txt" && file.installed != "-") {
            std::string marker = rpRead(root / "mods/f2r-text-choice.txt");
            size_t at = marker.find("\nretired=");
            size_t end = at == std::string::npos ? at : marker.find('\n', at + 9);
            std::string retired = at == std::string::npos ? "" : trim(marker.substr(at + 9, end - (at + 9)));
            const auto* oldText = launcher_translations::find(retired);
            bool suppressOfficial = marker.find("\nsuppress-official=1\n") != std::string::npos;
            bool suppressManualVoice = fs::exists(root / "mods/f2r-voice-manual-order.txt");
            std::string now = rpRead(destination);
            bool retiredVoice = false;
            for (const auto& voice : launcher_translations::voices) {
                bool active = false;
                launcher_uninstall::removeModLine(now, voice.filename, &active);
                if (!active && !fs::exists(root / "mods" / voice.filename)) retiredVoice = true;
            }
            if (oldText || suppressOfficial || retiredVoice || suppressManualVoice) {
                std::string installed;
                if (!sha256File(root / kRpOrder, &installed) || installed != file.installed) return false;
                std::string result = launcher_rp::restoredOrder(file.original == "-" ? "" : rpRead(rpOriginal(root, file.path)),
                    rpRead(root / kRpOrder), now);
                if (oldText) {
                    for (const char* name : { oldText->filename, launcher_translations::fonts.filename }) {
                        bool active = false;
                        launcher_uninstall::removeModLine(now, name, &active);
                        if (!active && !fs::exists(root / "mods" / name)) result = launcher_uninstall::removeModLine(result, name);
                    }
                }
                for (const auto& voice : launcher_translations::voices) {
                    bool active = false;
                    launcher_uninstall::removeModLine(now, voice.filename, &active);
                    if (!active && !fs::exists(root / "mods" / voice.filename)) result = launcher_uninstall::removeModLine(result, voice.filename);
                }
                if (suppressOfficial) result = launcher_uninstall::removeModLine(result, "rpu_russian.dat");
                if (suppressManualVoice) result = launcher_uninstall::removeLine(result, "upu_russian_sound.dat");
                sharedOrders[file.path] = result;
                continue;
            }
        }
        if (current == file.original || current == file.installed) continue;
        if (launcher_uninstall::lowered(file.path) == "mods/mods_order.txt" && file.installed != "-") {
            std::string installed;
            if (!sha256File(root / kRpOrder, &installed) || installed != file.installed) {
                logf("RP restore: invalid or missing backup for %s; backup/rpu was kept", file.path.c_str());
                return false;
            }
            sharedOrders[file.path] = launcher_rp::restoredOrder(file.original == "-" ? "" : rpRead(rpOriginal(root, file.path)),
                rpRead(root / kRpOrder), rpRead(destination));
        } else {
            logf("RP restore: %s was changed by the player; backup/rpu was kept", file.path.c_str());
            return false;
        }
    }
    if (!manifest.pending) {
        manifest.pending = true;
        if (!writeRpManifest(root, manifest)) {
            log("RP restore: cannot save recovery state; the game was left untouched");
            return false;
        }
    }
    bool ok = true;
    const fs::path temp = root / launcher_rp::kBackupFolder / "f2r-restore.tmp";
    for (const auto& file : manifest.files) {
        fs::path destination = root / utf8ToWide(file.path);
        std::error_code error;
        std::string current;
        if (file.autoOriginal && manifest.installing) {
            if (rpPresent(destination)) fs::remove(destination, error);
            if (error) ok = false;
            continue;
        }
        auto order = sharedOrders.find(file.path);
        if (order == sharedOrders.end() && file.original != "-" && sha256File(destination, &current) && current == file.original) continue;
        bool hasOrder = order != sharedOrders.end();
        if (file.original != "-" || hasOrder) {
            fs::create_directories(destination.parent_path(), error);
            if (!error) fs::remove(temp, error);
            std::string expected = file.original;
            if (!error && hasOrder) {
                std::ofstream out(temp, std::ios::binary | std::ios::trunc);
                out << order->second;
                out.close();
                unsigned char digest[32];
                if (!out || !sha256Data(reinterpret_cast<const unsigned char*>(order->second.data()), order->second.size(), digest))
                    error = std::make_error_code(std::errc::io_error);
                else expected = hashToHex(digest);
            } else if (!error) fs::copy_file(rpOriginal(root, file.path), temp, error);
            replaceRpFile(temp, destination, expected, error);
        } else if (rpPresent(destination)) fs::remove(destination, error);
        if (error) {
            logf("RP restore: cannot restore %s (%s); backup/rpu was kept", file.path.c_str(), error.message().c_str());
            ok = false;
        }
    }
    if (!ok) return false;
    manifest.restored = true;
    if (!writeRpManifest(root, manifest)) {
        log("RP cleanup: cannot save the restore receipt; backup/rpu was kept");
        return false;
    }
    return cleanup ? cleanupRp(root, manifest) : true;
}

bool removeRestorationProject(const fs::path& root, bool refreshScenes = false)
{
    if (!rpPresent(root / kRpManifest)) {
        if (rpOnDisk(root) || rpPresent(root / launcher_rp::kBackupFolder))
            log("Restoration Project has no installer manifest; a manual install was left untouched");
        return true;
    }
    RpManifest manifest;
    if (!readRpManifest(root, &manifest)) {
        log("RP restore: invalid installer manifest; the game and backup/rpu were left untouched");
        return false;
    }
    if (!restoreRp(root, manifest, !refreshScenes)) return false;
    if (!refreshScenes) return true;
    log("refreshing the vanilla 3D scenes after removing Restoration Project");
    fs::path exporter = root / kExporterExe;
    std::wstring command = L"\"" + exporter.wstring() + L"\" --game-root \"" + root.wstring() + L"\" --maps all";
    DWORD code = 1;
    if (!fs::is_regular_file(exporter) || !runTool(command, root.wstring(), &code, true) || code != 0) {
        log("RP removal: vanilla 3D scenes could not be refreshed; retry removal before playing, backup/rpu was kept");
        return false;
    }
    RpManifest restored;
    return readRpManifest(root, &restored) && cleanupRp(root, restored);
}

// RPU from its own GitHub release at the player's request (never a copy of ours), put in
// place the way its manual package's rpu-install.bat does; every file it moves or replaces
// is kept in backup/rpu.
bool installRestorationProject(const fs::path& root, bool* installedNow)
{
    *installedNow = false;
    if (rpPresent(root / kRpManifest)) {
        RpManifest previous;
        if (!readRpManifest(root, &previous)) {
            log("RP restore: invalid installer manifest; the game and backup/rpu were left untouched");
            return false;
        }
        if ((previous.pending || previous.restored) && !restoreRp(root, previous)) return false;
    }
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

    // Merge the effective pre-RP order into the package before the RP journal
    // is written, so an interrupted install cannot lose automatically found mods.
    std::string oldOrder;
    if (!effectiveModOrder(root, &oldOrder)) { fs::remove_all(work, error); return false; }
    fs::path packageOrder = stage / "mods_order.txt";
    if (!fs::exists(packageOrder, error)) packageOrder = stage / "mods/mods_order.txt";
    bool hasPackageOrder = fs::exists(packageOrder, error);
    if (error) { fs::remove_all(work, error); return false; }
    fs::path autoOriginal = work / "auto-order.txt";
    bool syntheticOriginal = !fs::exists(root / "mods/mods_order.txt", error) && !oldOrder.empty();
    if (error || (syntheticOriginal && !writeTextFile(autoOriginal, oldOrder))) { fs::remove_all(work, error); return false; }
    if (hasPackageOrder || !oldOrder.empty()) {
        std::string merged = launcher_rp::restoredOrder(hasPackageOrder ? readWholeFile(packageOrder) : "rpu.dat\r\n", "", oldOrder);
        std::string marker = readWholeFile(root / "mods/f2r-text-choice.txt");
        std::string id = trim(marker.substr(0, marker.find('\n')));
        if (const auto* oldText = launcher_translations::find(id)) {
            merged = launcher_uninstall::removeModLine(merged, oldText->filename);
            merged = launcher_uninstall::removeModLine(merged, launcher_translations::fonts.filename);
        }
        bool official = false;
        std::string withoutOfficial = launcher_uninstall::removeModLine(merged, "rpu_russian.dat", &official);
        if (official) merged = insertOfficialOrder(withoutOfficial);
        if (!writeTextFile(packageOrder, merged)) { fs::remove_all(work, error); return false; }
    }

    RpManifest manifest;
    std::map<std::string, fs::path> sources;
    std::set<std::string> targets;
    auto add = [&](const std::string& path, const fs::path& source) {
        if (!rpRelativePath(path) || !rpContained(root, utf8ToWide(path))) return false;
        std::string name = launcher_uninstall::lowered(path);
        if (!source.empty() && !targets.insert(name).second) return false;
        auto found = std::find_if(manifest.files.begin(), manifest.files.end(), [&](const auto& file) {
            return launcher_uninstall::lowered(file.path) == name;
        });
        if (found == manifest.files.end()) {
            manifest.files.push_back({path});
            found = manifest.files.end() - 1;
        }
        if (!source.empty()) {
            if (!sha256File(source, &found->installed)) return false;
            sources[found->path] = source;
        }
        return true;
    };
    bool ok = !rpPresent(root / launcher_rp::kBackupFolder) && rpContained(root, launcher_rp::kBackupFolder);
    for (fs::directory_iterator it(root, error), end; ok && !error && it != end; it.increment(error)) {
        if (it->is_regular_file(error) && launcher_rp::movedAside(wideToUtf8(it->path().filename().wstring())))
            ok = add(wideToUtf8(it->path().filename().wstring()), {});
    }
    if (error) ok = false;
    if (ok && rpPresent(root / "data")) {
        if (!rpContained(root, "data")) ok = false;
        for (fs::recursive_directory_iterator it(root / "data", error), end; ok && !error && it != end; it.increment(error)) {
            fs::path relative = it->path().lexically_relative(root / "data");
            std::string path = wideToUtf8(relative.generic_wstring());
            if (launcher_rp::keptInData(path + "/")) { it.disable_recursion_pending(); continue; }
            if (!rpContained(root, fs::path("data") / relative)) { ok = false; break; }
            if (it->is_regular_file(error) && !launcher_rp::keptInData(path)) ok = add("data/" + path, {});
        }
        if (error) ok = false;
    }
    for (fs::recursive_directory_iterator it(stage, error), end; ok && !error && it != end; it.increment(error)) {
        if (!rpContained(stage, it->path().lexically_relative(stage))) { ok = false; break; }
        if (!it->is_regular_file(error)) continue;
        std::string target = launcher_rp::targetOf(wideToUtf8(it->path().lexically_relative(stage).generic_wstring()));
        if (!target.empty()) ok = add(target, it->path());
    }
    if (error) ok = false;
    // Reject package file/parent collisions before taking any originals aside.
    for (const auto& target : targets)
        for (fs::path parent = fs::path(target).parent_path(); !parent.empty(); parent = parent.parent_path())
            if (targets.count(parent.generic_string())) ok = false;
    if (!targets.count("mods/rpu.dat")) ok = false;
    if (!ok) {
        log("RP install: unsafe or duplicate target, unreadable files, or existing backup/rpu; nothing was changed");
        fs::remove_all(work, error);
        return false;
    }
    for (auto& file : manifest.files) {
        fs::path destination = root / utf8ToWide(file.path);
        if (rpPresent(destination) && (!fs::is_regular_file(destination, error) || !sha256File(destination, &file.original))) ok = false;
        else if (syntheticOriginal && launcher_uninstall::lowered(file.path) == "mods/mods_order.txt") {
            file.autoOriginal = true;
            if (!sha256File(autoOriginal, &file.original)) ok = false;
        }
        for (fs::path parent = destination.parent_path(); parent != root && !rpPresent(parent); parent = parent.parent_path())
            manifest.directories.insert(wideToUtf8(parent.lexically_relative(root).generic_wstring()));
    }
    if (!ok) { fs::remove_all(work, error); return false; }
    fs::create_directories(root / launcher_rp::kBackupFolder, error);
    ok = !error;
    for (const auto& file : manifest.files) {
        if (!ok || file.original == "-") continue;
        fs::path kept = rpOriginal(root, file.path);
        fs::create_directories(kept.parent_path(), error);
        fs::path original = root / utf8ToWide(file.path);
        if (syntheticOriginal && launcher_uninstall::lowered(file.path) == "mods/mods_order.txt" && !rpPresent(original)) original = autoOriginal;
        if (!error) fs::copy_file(original, kept, error);
        std::string hash;
        ok = !error && sha256File(kept, &hash) && hash == file.original;
    }
    for (const auto& file : manifest.files) {
        if (ok && launcher_uninstall::lowered(file.path) == "mods/mods_order.txt" && file.installed != "-") {
            fs::copy_file(sources[file.path], root / kRpOrder, error);
            ok = !error;
        }
    }
    if (ok) ok = writeRpManifest(root, manifest);
    if (!ok) {
        log("RP install: cannot save originals and installer manifest; nothing was changed");
        fs::remove_all(root / launcher_rp::kBackupFolder, error);
        fs::remove_all(work, error);
        return false;
    }
    log("installing the Restoration Project into the game folder:");
    // Remove loose originals before RP takes over, then put its marker archive last.
    for (const auto& file : manifest.files) {
        if (file.original == "-" || sources.count(file.path)) continue;
        fs::remove(root / utf8ToWide(file.path), error);
        if (error) {
            logf("  cannot move %s aside (%s)", file.path.c_str(), error.message().c_str());
            ok = false;
            break;
        }
    }
    std::stable_partition(manifest.files.begin(), manifest.files.end(), [](const auto& file) {
        return launcher_uninstall::lowered(file.path) != "mods/rpu.dat";
    });
    int installed = 0;
    for (const auto& file : manifest.files) {
        if (!ok || file.installed == "-") continue;
        fs::path destination = root / utf8ToWide(file.path);
        fs::create_directories(destination.parent_path(), error);
        fs::path temp = root / launcher_rp::kBackupFolder / "f2r-restore.tmp";
        if (!error) fs::remove(temp, error);
        if (!error) fs::copy_file(sources[file.path], temp, error);
        replaceRpFile(temp, destination, file.installed, error);
        if (error) {
            logf("  %s: copy failed (%s)", file.path.c_str(), error.message().c_str());
            ok = false;
        } else ++installed;
    }
    if (ok) {
        manifest.pending = false;
        manifest.installing = false;
        ok = writeRpManifest(root, manifest);
        if (!ok) { manifest.pending = true; manifest.installing = true; }
    }
    if (!ok) {
        log("RP install failed; restoring the original game files");
        restoreRp(root, manifest);
    } else {
        *installedNow = true;
        logf("  installed %d files of RPU %s; what they replaced is in backup/rpu", installed, tag.c_str());
    }
    fs::remove_all(work, error);
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
        std::string hash;
        if (!sha256File(translation, &hash)
            || hash != kOfficialRussianHash) {
            log("the existing RPU translation is not recognized; it was left untouched");
            return false;
        }
        log("  kept the unowned mods/rpu_russian.dat");
    }
    fs::path orderPath = root / "mods" / "mods_order.txt";
    if (fs::exists(orderPath, error)) {
        bool removed = false;
        std::string order = launcher_uninstall::removeModLine(readWholeFile(orderPath), "rpu_russian.dat", &removed);
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

// The translation transaction only owns these names. A journal in the game
// folder lets the next launcher run restore originals after an interrupted swap.
const std::vector<std::string>& translationTargets()
{
    static const std::vector<std::string> names = {
        "fallout2.cfg", "mods/mods_order.txt", "mods/f2r-text-choice.txt",
        "mods/steam-1c-text.dat", "mods/fio-fargus-text.dat", "mods/fio-levy-text.dat",
        "mods/shared-legacy-fonts.dat", "mods/rpu_russian.dat",
        "mods/f2r-voice-choice.txt", "mods/f2r-voice-vanilla-1c.dat", "mods/f2r-voice-rp-1c.dat",
        "mods/f2r-voice-rp-levy.dat", "mods/f2r-voice-vanilla-fargus.dat", "mods/f2r-voice-vanilla-levy.dat",
        "mods/f2r-voice-manual-order.txt"
    };
    return names;
}

bool safeTranslationTarget(const fs::path& path)
{
    DWORD attr = GetFileAttributesW(path.c_str());
    if (attr == INVALID_FILE_ATTRIBUTES) {
        DWORD error = GetLastError();
        return error == ERROR_FILE_NOT_FOUND || error == ERROR_PATH_NOT_FOUND;
    }
    return !(attr & (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_REPARSE_POINT));
}

bool recoverTextTransaction(const fs::path& root)
{
    fs::path work = root / ".f2r-text-transaction";
    std::error_code error;
    if (!fs::exists(work, error)) return !error;
    for (const auto& name : translationTargets()) if (!safeTranslationTarget(root / name)) return false;
    if (fs::exists(work / "committed", error)) {
        fs::remove_all(work, error);
        return !error;
    }
    if (!fs::exists(work / "journal", error)) {
        if (!fs::exists(work / "started", error) && !fs::is_empty(work, error)) return false;
        if (error) return false;
        fs::remove_all(work, error);
        return !error;
    }
    std::string journal = readWholeFile(work / "journal");
    if (journal.size() != 8 && journal.size() != 14 && journal.size() != translationTargets().size()) return false;
    for (size_t i = 0; i < journal.size(); ++i) {
        if (journal[i] == '2') continue;
        fs::path dest = root / translationTargets()[i];
        fs::path backup = work / ("old" + std::to_string(i));
        fs::path staged = work / ("new" + std::to_string(i));
        if (!safeTranslationTarget(backup) || !safeTranslationTarget(staged)) return false;
        if (journal[i] == '1' && fs::exists(backup, error)) {
            fs::remove(dest, error);
            if (error) return false;
            fs::rename(backup, dest, error);
            if (error) return false;
        } else if (journal[i] == '0' && !fs::exists(staged, error)) {
            fs::remove(dest, error);
            if (error) return false;
        } else if (journal[i] != '0' && journal[i] != '1') return false;
        if (error) return false;
    }
    fs::remove_all(work, error);
    return !error;
}

bool writeTextFile(const fs::path& path, const std::string& value)
{
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    out.write(value.data(), static_cast<std::streamsize>(value.size()));
    out.close();
    return !!out;
}

std::string insertAfterMod(const std::string& input, const char* target, const std::string& addition, bool prependIfMissing)
{
    for (size_t begin = 0; begin < input.size();) {
        size_t end = input.find('\n', begin);
        size_t next = end == std::string::npos ? input.size() : end + 1;
        if (launcher_uninstall::normalizedModName(input.substr(begin, next - begin)) == target) {
            std::string prefix = input.substr(0, next);
            if (!prefix.empty() && prefix.back() != '\n') prefix += "\r\n";
            return prefix + addition + input.substr(next);
        }
        begin = next;
    }
    return prependIfMissing ? addition + input : input;
}

std::string insertOfficialOrder(const std::string& input)
{
    return insertAfterMod(input, "rpu.dat", "rpu_russian.dat\r\n", true);
}

std::string insertTextAfterOfficial(const std::string& input, const char* fonts, const char* text)
{
    return insertAfterMod(input, "rpu_russian.dat", std::string(fonts) + "\r\n" + text + "\r\n", false);
}

bool effectiveModOrder(const fs::path& root, std::string* order)
{
    fs::path path = root / "mods/mods_order.txt";
    std::error_code error;
    if (fs::exists(path, error)) {
        std::ifstream in(path, std::ios::binary);
        if (!in) return false;
        order->assign(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
        return !in.bad();
    }
    if (error) return false;
    std::vector<std::string> names;
    fs::path mods = root / "mods";
    if (!fs::exists(mods, error)) return !error;
    for (fs::directory_iterator it(mods, error), end; !error && it != end; it.increment(error)) {
        if (!it->is_regular_file(error)) continue;
        if (_wcsicmp(it->path().extension().c_str(), L".dat") == 0) names.push_back(wideToUtf8(it->path().filename().wstring()));
    }
    if (error) return false;
    std::sort(names.begin(), names.end(), [](const std::string& a, const std::string& b) { return _stricmp(a.c_str(), b.c_str()) < 0; });
    for (const auto& name : names) *order += name + "\r\n";
    return true;
}

bool translationRpOrderValid(const fs::path& root)
{
    if (!rpOnDisk(root)) return true;
    std::string order;
    if (!effectiveModOrder(root, &order)) return false;
    std::stringstream lines(order);
    std::string line;
    while (std::getline(lines, line)) if (launcher_uninstall::normalizedModName(line) == "rpu.dat") return true;
    return false;
}

std::string installedVoiceId(const fs::path& root)
{
    std::string receipt = readWholeFile(root / "mods/f2r-voice-choice.txt");
    return trim(receipt.substr(0, receipt.find('\n')));
}

bool fetchVoice(const launcher_translations::VoicePackage& voice, const fs::path& path)
{
    std::string message, hash;
    std::error_code error;
    if (!fetchRoutes({ { "voice", voice.url, "" } }, nullptr, &path, &message)) {
        logf("voice download failed: %s", message.c_str());
        return false;
    }
    auto size = fs::file_size(path, error);
    if (error || size != std::stoull(voice.size)) {
        log("voice download has an incorrect size");
        return false;
    }
    if (!sha256File(path, &hash) || hash != voice.hash) {
        log("voice download failed SHA-256 verification");
        return false;
    }
    return true;
}

bool changeTextTranslation(const fs::path& root, const std::string& choice, const std::string& previousOrder = {}, const std::string& voiceChoice = {}, bool restoreManualVoice = false, const fs::path& preflight = {}, const fs::path& textPreflight = {})
{
    using namespace launcher_translations;
    for (const auto& name : translationTargets()) if (!safeTranslationTarget(root / name)) {
        log("translation target is not a regular file; it was left untouched");
        return false;
    }
    if (!recoverTextTransaction(root)) return false;
    bool rp = rpOnDisk(root);
    const Package* selected = find(choice, rp);
    const char* url = textUrl(choice, rp);
    if ((!choice.empty() && (!available(choice, rp, url, urlFonts)
            || (selected && !translationRpOrderValid(root))
            || (selected && rp && !supportedVoiceRp(root))))
        || (!voiceChoice.empty() && (!voiceAvailable(voiceChoice, rpOnDisk(root))
            || (findVoice(voiceChoice) && findVoice(voiceChoice)->rp && !supportedVoiceRp(root))))) {
        log("this translation is unavailable for the current game or has no download URL");
        return false;
    }
    std::error_code error;
    fs::create_directories(root / "mods", error);
    if (error) return false;
    fs::path work = root / ".f2r-text-transaction";
    if (!fs::create_directory(work, error)) return false;
    auto abandon = [&]() { std::error_code ignored; fs::remove_all(work, ignored); return false; };
    if (!writeTextFile(work / "started", "f2r text staging\n")) return abandon();
    std::string order;
    if (!effectiveModOrder(root, &order)) return abandon();
    std::string initialOrder = order;
    std::string textReceipt = readWholeFile(root / "mods/f2r-text-choice.txt");
    std::string textId = trim(textReceipt.substr(0, textReceipt.find('\n')));
    const Package* ownedText = find(textId);
    std::string textHash, fontsHash;
    bool textOwned = ownedText && sha256File(root / "mods" / ownedText->filename, &textHash)
        && (textHash == ownedText->sha256 || textHash == find(textId, true)->sha256);
    bool fontsOwned = textOwned && sha256File(root / "mods" / fonts.filename, &fontsHash) && fontsHash == fonts.sha256;
    auto withoutOwnedText = [&](std::string value) {
        if (textOwned) value = launcher_uninstall::removeModLine(value, ownedText->filename);
        if (fontsOwned) value = launcher_uninstall::removeModLine(value, fonts.filename);
        return value;
    };
    std::vector<bool> replace(translationTargets().size(), false);
    auto stageText = [&](size_t i, const std::string& content) {
        replace[i] = true;
        return writeTextFile(work / ("new" + std::to_string(i)), content);
    };
    if (!choice.empty()) {
    std::string cfg = readWholeFile(root / kCfg);
    bool found = false;
    cfg = launcher_uninstall::withLanguage(cfg, choice == "original" ? "english" : "russian", &found);
    if (!found) return abandon();
    order = launcher_rp::restoredOrder(order, "", previousOrder);
    std::string previousMarker = readWholeFile(root / "mods/f2r-text-choice.txt");
    previousMarker.erase(std::remove(previousMarker.begin(), previousMarker.end(), '\r'), previousMarker.end());
    std::string previous = trim(previousMarker.substr(0, previousMarker.find('\n')));
    for (const Package& item : texts) {
        fs::path path = root / "mods" / item.filename;
        if (fs::exists(path, error)) {
            std::string hash;
            if (previous != item.id || !sha256File(path, &hash)
                || (hash != item.sha256 && hash != find(item.id, true)->sha256)) return abandon();
        }
        if (previous == item.id) order = launcher_uninstall::removeModLine(order, item.filename);
    }
    fs::path fontPath = root / "mods" / fonts.filename;
    if (fs::exists(fontPath, error)) {
        std::string hash;
        if (!find(previous) || !sha256File(fontPath, &hash) || hash != fonts.sha256) return abandon();
    }
    if (find(previous)) order = launcher_uninstall::removeModLine(order, fonts.filename);
    fs::path officialPath = root / "mods/rpu_russian.dat";
    bool officialExists = fs::exists(officialPath, error);
    bool officialOwned = previousMarker.find("\nofficial-owned=1\n") != std::string::npos;
    std::string officialHash;
    bool officialKnown = officialExists && sha256File(officialPath, &officialHash) && officialHash == kOfficialRussianHash;
    bool officialActive = false;
    launcher_uninstall::removeModLine(order, "rpu_russian.dat", &officialActive);
    bool needsOfficial = choice == "official" || (rp && selected);
    if ((officialOwned && !officialKnown) || (needsOfficial && officialExists && !officialKnown)) return abandon();
    if (!needsOfficial && officialExists && !officialKnown && officialActive) return abandon();
    if (!needsOfficial && officialKnown) order = launcher_uninstall::removeModLine(order, "rpu_russian.dat");
    if (error) return abandon();
    if (!needsOfficial && officialOwned) replace[7] = true;
    if (selected) {
        size_t index = choice == "1c" ? 3 : choice == "fargus" ? 4 : 5;
        auto download = [&](const char* source, const Package& item, size_t i) {
            std::string message, hash;
            fs::path path = work / ("new" + std::to_string(i));
            bool fetched = i == index && !textPreflight.empty()
                ? fs::copy_file(textPreflight, path, error) && !error
                : i == 6 && !textPreflight.empty()
                ? fs::copy_file(root / "mods" / fonts.filename, path, error) && !error
                : fetchRoutes({ { "translation", source, "" } }, nullptr, &path, &message);
            if (!fetched
                || fs::file_size(path, error) != item.size || error
                || !sha256File(path, &hash) || hash != item.sha256) return false;
            replace[i] = true;
            return true;
        };
        if (!download(url, *selected, index) || !download(urlFonts, fonts, 6)) return abandon();
        if (!rp) order = std::string(selected->filename) + "\r\n" + fonts.filename + "\r\n" + order;
    }
    if (needsOfficial) {
        // Reuse the existing official mirror and RPU fallback in an isolated root.
        fs::path scratch = work / "official";
        fs::create_directories(scratch / "mods", error);
        if (error || !writeTextFile(scratch / kCfg, "[system]\r\nlanguage=english\r\n")) return abandon();
        if (!officialExists) {
            if (!installRussianTranslation(scratch)) return abandon();
            fs::rename(scratch / "mods/rpu_russian.dat", work / "new7", error);
            if (error) return abandon();
            replace[7] = true;
            officialOwned = true;
        }
        order = launcher_uninstall::removeModLine(order, "rpu_russian.dat");
        order = insertOfficialOrder(order);
        if (selected) order = insertTextAfterOfficial(order, fonts.filename, selected->filename);
    }
    std::string marker = choice + "\n";
    if (needsOfficial && officialOwned) marker += "official-owned=1\n";
    if (rpOnDisk(root)) {
        size_t at = previousMarker.find("\nretired=");
        size_t end = at == std::string::npos ? at : previousMarker.find('\n', at + 9);
        std::string retired = find(previous) ? previous : at == std::string::npos ? "" : trim(previousMarker.substr(at + 9, end - (at + 9)));
        if (find(retired)) marker += "retired=" + retired + "\n";
        if (choice != "official" && (previous == "official" || officialKnown
                || previousMarker.find("\nsuppress-official=1\n") != std::string::npos))
            marker += "suppress-official=1\n";
    }
    if (!stageText(0, cfg) || !stageText(2, marker)) return abandon();
    // Old owned DATs are removed by moving them to the transaction backup.
    for (size_t i = 3; i <= 6; ++i) {
        if (!replace[i] && fs::exists(root / translationTargets()[i], error)) replace[i] = true;
    }
    }
    if (!voiceChoice.empty()) {
        bool manualActive = false;
        auto withoutManual = launcher_uninstall::removeLine(order, "upu_russian_sound.dat", &manualActive);
        auto manualPath = root / "mods/upu_russian_sound.dat";
        std::string manualHash;
        bool manualExists = fs::exists(manualPath, error);
        if (error) return abandon();
        bool knownManual = manualExists && safeTranslationTarget(manualPath) && sha256File(manualPath, &manualHash)
            && (manualHash == "042414243fdd3dd55a81916f974d120f0292aee954d77a2d3397b2265e0edd2a"
                || manualHash == "41f45e4b60fb3e1350f375fc325da575167266f64503e80e36591ca221e6d5e9"
                || manualHash == "11b008f4f4eb511fa0b9337881364333622b7073ed488b4486a3fff418bc8ea1");
        auto snapshot = root / translationTargets()[14];
        if (manualActive && knownManual && !restoreManualVoice) {
            if (fs::exists(snapshot, error)) {
                bool saved = false;
                launcher_uninstall::removeLine(readWholeFile(snapshot), "upu_russian_sound.dat", &saved);
                if (!saved) return abandon();
            } else if (!stageText(14, withoutOwnedText(initialOrder))) return abandon();
            order = withoutManual;
        }
        if (restoreManualVoice && fs::exists(snapshot, error)) {
            if (knownManual && !manualActive) {
                std::string old = withoutOwnedText(readWholeFile(snapshot));
                size_t row = 0, end = std::string::npos;
                while (row < old.size()) {
                    end = old.find('\n', row);
                    std::string line = old.substr(row, end == std::string::npos ? end : end - row);
                    if (launcher_uninstall::lowered(launcher_uninstall::trimmed(line)) == "upu_russian_sound.dat") break;
                    if (end == std::string::npos) { row = old.size(); break; }
                    row = end + 1;
                }
                if (row == old.size()) return abandon();
                std::string manualLine = old.substr(row, end == std::string::npos ? end : end + 1 - row);
                std::string base = launcher_uninstall::removeLine(old, "upu_russian_sound.dat");
                bool samePrefix = order.compare(0, base.size(), base) == 0
                    && (order.size() == base.size() || base.empty() || base.back() == '\n');
                if (samePrefix) {
                    std::string extra = order.substr(base.size());
                    order = old;
                    if (!extra.empty() && !order.empty() && order.back() != '\n')
                        order += old.find("\r\n") != std::string::npos ? "\r\n" : "\n";
                    order += extra;
                } else {
                    auto matches = [](const std::string& text, const std::string& key, size_t* position) {
                        int count = 0;
                        for (size_t at = 0; at < text.size();) {
                            size_t lineEnd = text.find('\n', at);
                            std::string line = text.substr(at, lineEnd == std::string::npos ? lineEnd : lineEnd - at);
                            if (launcher_uninstall::lowered(launcher_uninstall::trimmed(line)) == key) { ++count; *position = at; }
                            if (lineEnd == std::string::npos) break;
                            at = lineEnd + 1;
                        }
                        return count;
                    };
                    size_t position = std::string::npos;
                    for (size_t at = end == std::string::npos ? old.size() : end + 1; at < old.size();) {
                        size_t lineEnd = old.find('\n', at);
                        std::string key = launcher_uninstall::lowered(launcher_uninstall::trimmed(old.substr(at, lineEnd == std::string::npos ? lineEnd : lineEnd - at)));
                        size_t oldPosition = 0, currentPosition = 0;
                        if (!key.empty() && key[0] != '#' && key[0] != ';' && key.rfind("//", 0) != 0
                            && matches(base, key, &oldPosition) == 1 && matches(order, key, &currentPosition) == 1) {
                            position = currentPosition;
                            break;
                        }
                        if (lineEnd == std::string::npos) break;
                        at = lineEnd + 1;
                    }
                    if (position == std::string::npos) {
                        for (size_t at = 0; at < row;) {
                            size_t lineEnd = old.find('\n', at);
                            std::string key = launcher_uninstall::lowered(launcher_uninstall::trimmed(old.substr(at, lineEnd == std::string::npos ? lineEnd : lineEnd - at)));
                            size_t oldPosition = 0, currentPosition = 0;
                            if (!key.empty() && key[0] != '#' && key[0] != ';' && key.rfind("//", 0) != 0
                                && matches(base, key, &oldPosition) == 1 && matches(order, key, &currentPosition) == 1) {
                                size_t currentEnd = order.find('\n', currentPosition);
                                position = currentEnd == std::string::npos ? order.size() : currentEnd + 1;
                            }
                            if (lineEnd == std::string::npos) break;
                            at = lineEnd + 1;
                        }
                    }
                    if (position == std::string::npos) {
                        if (!order.empty() && order.back() != '\n') order += old.find("\r\n") != std::string::npos ? "\r\n" : "\n";
                        order += manualLine;
                    } else {
                        if (position > 0 && order[position - 1] != '\n') {
                            std::string ending = old.find("\r\n") != std::string::npos ? "\r\n" : "\n";
                            order.insert(position, ending);
                            position += ending.size();
                        }
                        order.insert(position, manualLine);
                    }
                }
            }
            replace[14] = true;
        }
        if (error) return abandon();
        std::string receipt = readWholeFile(root / "mods/f2r-voice-choice.txt");
        std::string previous = installedVoiceId(root);
        size_t hashAt = receipt.find("\nhash=");
        std::string ownedHash = hashAt == std::string::npos ? "" : trim(receipt.substr(hashAt + 6));
        for (size_t i = 0; i < std::size(voices); ++i) {
            const auto& item = voices[i];
            fs::path path = root / "mods" / item.filename;
            if (fs::exists(path, error) && (previous == item.id || voiceChoice == item.id)) {
                std::string hash;
                if (previous != item.id || ownedHash.size() != 64 || !sha256File(path, &hash) || hash != ownedHash) return abandon();
                replace[9 + i] = true;
            }
            if (previous == item.id) order = launcher_uninstall::removeModLine(order, item.filename);
            if (error) return abandon();
        }
        if (const auto* selectedVoice = findVoice(voiceChoice)) {
            size_t index = static_cast<size_t>(selectedVoice - voices) + 9;
            fs::path path = work / ("new" + std::to_string(index));
            if (preflight.empty()) {
                if (!fetchVoice(*selectedVoice, path)) return abandon();
            } else {
                fs::copy_file(preflight, path, error);
                std::string hash;
                if (error || fs::file_size(path, error) != std::stoull(selectedVoice->size) || error
                    || !sha256File(path, &hash) || hash != selectedVoice->hash) return abandon();
            }
            replace[index] = true;
            if (!order.empty() && order.back() != '\n') order += "\r\n";
            order += std::string(selectedVoice->filename) + "\r\n";
        }
        std::string voiceReceipt = voiceChoice + "\n";
        if (const auto* selectedVoice = findVoice(voiceChoice)) voiceReceipt += std::string("hash=") + selectedVoice->hash + "\n";
        if (!stageText(8, voiceReceipt)) return abandon();
    }
    if (!stageText(1, order)) return abandon();
    std::string journal;
    for (size_t i = 0; i < replace.size(); ++i) {
        if (!replace[i]) { journal += '2'; continue; }
        journal += fs::exists(root / translationTargets()[i], error) ? '1' : '0';
        if (error) return abandon();
    }
    if (!writeTextFile(work / "journal.tmp", journal)) return abandon();
    fs::rename(work / "journal.tmp", work / "journal", error);
    if (error) return abandon();
    for (size_t i = 0; i < replace.size(); ++i) {
        if (!replace[i]) continue;
        fs::path dest = root / translationTargets()[i];
        if (journal[i] == '1') fs::rename(dest, work / ("old" + std::to_string(i)), error);
        if (!error && fs::exists(work / ("new" + std::to_string(i))))
            fs::rename(work / ("new" + std::to_string(i)), dest, error);
        if (error) { recoverTextTransaction(root); return false; }
    }
    fs::rename(work / "journal", work / "committed", error);
    if (error) { recoverTextTransaction(root); return false; }
    fs::remove_all(work, error);
    std::string finalText = choice.empty() ? textId : choice;
    if (finalText.empty()) finalText = launcher_uninstall::languageOf(readWholeFile(root / kCfg)) == "russian" ? "official" : "original";
    std::string finalVoice = voiceChoice.empty() ? installedVoiceId(root) : voiceChoice;
    if ((finalVoice.empty() || finalVoice == "original") && knownActiveManualVoice(root)) finalVoice = "manual-bgforge";
    if (voiceTextMayDiffer(finalText, finalVoice)) log("Warning: voice lines may differ from the text.");
    return true;
}

// Gives the game back as it was before the mod.  Saves (data/), the original game files
// and fallout2.cfg are never touched.  The folder f2reimagined-backups only holds older
// mod builds, so it goes with the mod.
bool uninstallMod(const fs::path& root)
{
    if ((fs::exists(root / "mods/f2r-text-choice.txt") || fs::exists(root / "mods/f2r-voice-choice.txt"))
        && !changeTextTranslation(root, fs::exists(root / "mods/f2r-text-choice.txt") ? "original" : "", {},
            fs::exists(root / "mods/f2r-voice-choice.txt") ? "original" : "", true)) return false;
    if (!removeRestorationProject(root)) return false;
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
            log("  --remove-restoration-project  remove installer-managed RP and restore original files");
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
        } else if (arg == "--text-translation" && i + 1 < argc) {
            options.textTranslation = argv[++i];
        } else if (arg == "--voice-translation" && i + 1 < argc) {
            options.voiceTranslation = argv[++i];
        } else if (arg == "--text-with-install") {
            options.textWithInstall = true;
        } else if (arg == "--recover-text") {
            options.recoverTextOnly = true;
        } else if (arg == "--remove-restoration-project") {
            options.removeRp = true;
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

bool pinnedRpDat(const std::filesystem::path& root)
{
    std::error_code error;
    auto path = root / "mods/rpu.dat";
    if (std::filesystem::file_size(path, error) != 129661747 || error) return false;
    std::string hash;
    return sha256File(path, &hash) && hash == "27db830e0fb67d02b60e166e2acb772a92292f5f4dd88f733b740b406a0864ee";
}

bool supportedVoiceRp(const std::filesystem::path& root)
{
    return pinnedRpDat(root) && translationRpOrderValid(root);
}

bool translationRecoveryPending(const std::filesystem::path& root)
{
    std::error_code error;
    if (fs::exists(root / ".f2r-text-transaction", error) || error) return true;
    const auto* voice = launcher_translations::findVoice(installedVoiceId(root));
    if (voice && voice->rp && !supportedVoiceRp(root)) return true;
    std::string receipt = readWholeFile(root / "mods/f2r-text-choice.txt");
    std::string id = trim(receipt.substr(0, receipt.find('\n')));
    bool rp = rpOnDisk(root);
    const auto* package = launcher_translations::find(id, rp);
    if (!package) return false;
    if (!translationRpOrderValid(root) || (rp && !supportedVoiceRp(root))) return true;
    std::string hash;
    return !sha256File(root / "mods" / package->filename, &hash) || hash != package->sha256;
}

bool knownActiveManualVoice(const std::filesystem::path& root)
{
    auto path = root / "mods/upu_russian_sound.dat";
    if (!safeTranslationTarget(path)) return false;
    std::string hash, order;
    if (!sha256File(path, &hash) || (hash != "042414243fdd3dd55a81916f974d120f0292aee954d77a2d3397b2265e0edd2a"
        && hash != "41f45e4b60fb3e1350f375fc325da575167266f64503e80e36591ca221e6d5e9"
        && hash != "11b008f4f4eb511fa0b9337881364333622b7073ed488b4486a3fff418bc8ea1")
        || !effectiveModOrder(root, &order)) return false;
    bool active = false;
    launcher_uninstall::removeLine(order, "upu_russian_sound.dat", &active);
    return active;
}

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
        std::error_code error;
        root = fs::canonical(root, error);
        if (error) { log("cannot resolve the game folder"); return 1; }
    }
    // Other launcher instances must not recover or mutate this root mid-swap.
    std::wstring rootKey = root.wstring();
    CharLowerBuffW(rootKey.data(), static_cast<DWORD>(rootKey.size()));
    unsigned char rootDigest[32];
    std::string rootBytes = wideToUtf8(rootKey);
    if (!sha256Data(reinterpret_cast<const unsigned char*>(rootBytes.data()), rootBytes.size(), rootDigest)) return 1;
    std::wstring mutexName = L"Local\\F2R-" + utf8ToWide(hashToHex(rootDigest));
    HANDLE mutex = CreateMutexW(nullptr, FALSE, mutexName.c_str());
    if (!mutex) return 1;
    DWORD lockResult = WaitForSingleObject(mutex, 0);
    if (lockResult != WAIT_OBJECT_0 && lockResult != WAIT_ABANDONED) {
        CloseHandle(mutex);
        log("another launcher is already working on this game folder");
        return 1;
    }
    struct RootLock {
        HANDLE handle;
        ~RootLock() { ReleaseMutex(handle); CloseHandle(handle); }
    } rootLock{ mutex };
    if (!options.installRp && !options.removeRp && !options.uninstall) {
        fs::path receiptPath = root / kRpManifest;
        DWORD attributes = GetFileAttributesW(receiptPath.c_str());
        DWORD attributeError = attributes == INVALID_FILE_ATTRIBUTES ? GetLastError() : ERROR_SUCCESS;
        bool absent = attributeError == ERROR_FILE_NOT_FOUND || attributeError == ERROR_PATH_NOT_FOUND;
        std::error_code statusError;
        bool regularReceipt = attributes != INVALID_FILE_ATTRIBUTES && fs::is_regular_file(receiptPath, statusError);
        bool valid = absent;
        if (!absent && regularReceipt && !statusError) {
            RpManifest receipt;
            fs::path rpDat = root / "mods/rpu.dat";
            std::error_code datError;
            valid = readRpManifest(root, &receipt) && !receipt.pending && !receipt.restored
                && fs::is_regular_file(rpDat, datError) && !datError;
        }
        if (!valid) {
            log("finish Restoration Project recovery before continuing");
            return 1;
        }
    }
    {
        std::error_code ignored;
        fs::path old = root / kRootLauncher;
        old += ".old";
        fs::remove(old, ignored);
    }
    if (!recoverTextTransaction(root)) { log("cannot recover the previous translation change"); return 1; }
    if (rpPresent(root / kVoiceRpTransition)) {
        RpManifest receipt;
        bool ready = !rpPresent(root / kRpManifest)
            || (readRpManifest(root, &receipt) && !receipt.pending && !receipt.restored);
        if (ready) {
            const auto* voice = launcher_translations::findVoice(installedVoiceId(root));
            if (voice && (voice->rp != rpOnDisk(root) || (voice->rp && !supportedVoiceRp(root)))) {
                std::string mapped = launcher_translations::voiceForMode(voice->id, rpOnDisk(root));
                std::string sourceHash;
                const auto* mappedVoice = launcher_translations::findVoice(mapped);
                if (!launcher_translations::voiceAvailable(mapped, rpOnDisk(root))
                    || !sha256File(root / "mods" / voice->filename, &sourceHash) || sourceHash != voice->hash
                    || !mappedVoice || std::string(mappedVoice->hash) != voice->hash || std::string(mappedVoice->size) != voice->size
                    || (rpOnDisk(root) && !supportedVoiceRp(root))) mapped = "original";
                if (!changeTextTranslation(root, "", {}, mapped, false,
                        mapped == "original" ? fs::path{} : root / "mods" / voice->filename)) return 1;
            }
            std::error_code markerError;
            if (!fs::remove(root / kVoiceRpTransition, markerError) || markerError) return 1;
        } else if (!options.installRp && !options.removeRp && !options.uninstall) {
            log("finish Restoration Project recovery before continuing");
            return 1;
        }
    }
    if (!options.installRp && !options.removeRp && !options.uninstall) {
        const auto* voice = launcher_translations::findVoice(installedVoiceId(root));
        if (voice && voice->rp && !supportedVoiceRp(root)) {
            if (!options.voiceTranslation.empty() && options.voiceTranslation != "original") {
                log("The RP voice is unavailable for the current RP; select original voice.");
                return 1;
            }
            log("The RP voice is incompatible with the current RP; switching to original voice.");
            if (!changeTextTranslation(root, "", {}, "original", true)) return 1;
        }
    }
    std::string textReceipt = readWholeFile(root / "mods/f2r-text-choice.txt");
    std::string textId = trim(textReceipt.substr(0, textReceipt.find('\n')));
    if (!options.installRp && !options.removeRp && launcher_translations::find(textId)) {
        bool rpReady = true;
        if (rpPresent(root / kRpManifest)) {
            RpManifest receipt;
            rpReady = readRpManifest(root, &receipt) && !receipt.pending && !receipt.restored;
        }
        const auto* package = launcher_translations::find(textId, rpOnDisk(root));
        std::string hash;
        if (rpReady && !translationRpOrderValid(root)) {
            if ((!options.textTranslation.empty() && options.textTranslation != "original")
                || (!options.voiceTranslation.empty() && options.voiceTranslation != "original")) {
                log("RP is excluded from mods_order.txt; enable it or select original text.");
                return 1;
            }
            log("RP is excluded from mods_order.txt; switching to English.");
            if (!changeTextTranslation(root, "original")) return 1;
        } else if (rpReady && rpOnDisk(root) && !supportedVoiceRp(root)) {
            log("The installed RP version does not match the text package; switching to English.");
            if (!changeTextTranslation(root, "original")) return 1;
        } else
        if (rpReady && (!sha256File(root / "mods" / package->filename, &hash) || hash != package->sha256)
            && !changeTextTranslation(root, textId)) return 1;
        if (!rpReady && options.recoverTextOnly) { log("finish Restoration Project recovery before switching the text"); return 1; }
    }
    if (options.recoverTextOnly) return 0;
    if ((!options.textTranslation.empty() || !options.voiceTranslation.empty()) && !options.textWithInstall)
        return changeTextTranslation(root, options.textTranslation, {}, options.voiceTranslation) ? 0 : 1;
    if (options.removeRp) {
        if (rpOnDisk(root) && !rpPresent(root / kRpManifest))
            return removeRestorationProject(root, true) ? 0 : 1;
        fs::path textPreflight;
        fs::path voicePreflight;
        struct TextPreflightCleanup {
            fs::path* path;
            ~TextPreflightCleanup() { if (!path->empty()) { std::error_code ignored; fs::remove(*path, ignored); } }
        } textCleanup{ &textPreflight };
        TextPreflightCleanup voiceCleanup{ &voicePreflight };
        const auto* baseText = launcher_translations::find(textId);
        if (baseText) {
            std::string installedHash, fontHash;
            const auto* currentText = launcher_translations::find(textId, true);
            if (!sha256File(root / "mods" / currentText->filename, &installedHash)) return 1;
            if (!rpOnDisk(root) && installedHash == baseText->sha256) baseText = nullptr;
            else if ((rpOnDisk(root) && !pinnedRpDat(root)) || installedHash != currentText->sha256
                || !sha256File(root / "mods" / launcher_translations::fonts.filename, &fontHash) || fontHash != launcher_translations::fonts.sha256) return 1;
        }
        if (baseText) {
            textPreflight = fs::temp_directory_path() / ("f2r-text-" + std::to_string(GetCurrentProcessId()) + ".dat");
            if (fs::exists(textPreflight)) { textPreflight.clear(); return 1; }
            std::string message, hash;
            std::error_code error;
            if (!fetchRoutes({ { "translation", launcher_translations::textUrl(textId, false), "" } }, nullptr, &textPreflight, &message)
                || fs::file_size(textPreflight, error) != baseText->size || error
                || !sha256File(textPreflight, &hash) || hash != baseText->sha256) return 1;
        }
        std::string voice = installedVoiceId(root);
        std::string mappedVoice = launcher_translations::voiceForMode(voice, false);
        if (mappedVoice != voice) {
            const auto* oldVoice = launcher_translations::findVoice(voice);
            const auto* newVoice = launcher_translations::findVoice(mappedVoice);
            std::string hash;
            if (!oldVoice || !newVoice || !launcher_translations::voiceAvailable(mappedVoice, false)
                || !sha256File(root / "mods" / oldVoice->filename, &hash) || hash != oldVoice->hash) return 1;
            voicePreflight = fs::temp_directory_path() / ("f2r-voice-" + std::to_string(GetCurrentProcessId()) + ".dat");
            if (fs::exists(voicePreflight)) { voicePreflight.clear(); return 1; }
            if (std::string(oldVoice->hash) == newVoice->hash && std::string(oldVoice->size) == newVoice->size) {
                std::error_code error;
                if (!fs::copy_file(root / "mods" / oldVoice->filename, voicePreflight, error) || error) return 1;
            } else if (!fetchVoice(*newVoice, voicePreflight)) return 1;
        }
        bool guard = launcher_translations::findVoice(voice) && launcher_translations::findVoice(voice)->rp && rpPresent(root / kRpManifest);
        if (guard && !writeTextFile(root / kVoiceRpTransition, "F2R voice RP transition\n")) return 1;
        if (!removeRestorationProject(root, true)) {
            RpManifest receipt;
            if (guard && readRpManifest(root, &receipt) && !receipt.pending && !receipt.restored && rpOnDisk(root)) {
                std::error_code ignored; fs::remove(root / kVoiceRpTransition, ignored);
            }
            return 1;
        }
        if ((baseText || mappedVoice != voice)
            && !changeTextTranslation(root, baseText ? textId : "", {}, mappedVoice != voice ? mappedVoice : "", false, voicePreflight, textPreflight)) return 1;
        if (const auto* package = launcher_translations::findVoice(voice); mappedVoice == voice && package && package->rp != rpOnDisk(root))
            if (!changeTextTranslation(root, "", {}, "original")) return 1;
        { std::error_code markerError; fs::remove(root / kVoiceRpTransition, markerError); if (markerError) return 1; }
        return 0;
    }
    if (options.uninstall) return uninstallMod(root) ? 0 : 1;
    if (options.removeRussian) {
        log("removing the Russian translation:");
        return changeTextTranslation(root, "original") ? 0 : 1;
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
        for (const char* name : { kGameExe, "ce.dat", kExporterExe, kVersionTxt }) {
            if (!fs::is_regular_file(stage / name, error)) {
                fs::remove_all(work, error);
                die("the downloaded package is damaged (required file %s is missing); nothing was changed", name);
            }
        }

        std::ifstream versionFile(stage / kVersionTxt, std::ios::binary);
        std::string packageVersion, extraLine;
        bool hasVersion = static_cast<bool>(std::getline(versionFile, packageVersion));
        bool hasExtraLine = static_cast<bool>(std::getline(versionFile, extraLine));
        packageVersion = trim(packageVersion);
        if (!hasVersion || hasExtraLine || !versionFile.eof() || packageVersion.empty()
            || packageVersion == "." || packageVersion == ".."
            || std::any_of(packageVersion.begin(), packageVersion.end(), [](unsigned char ch) { return ch < 32 || ch == 127; })) {
            versionFile.close();
            fs::remove_all(work, error);
            die("the downloaded package has an invalid version.txt; nothing was changed");
        }
        versionFile.close();

        if (!canReplaceBuildVersion(root)) {
            fs::remove_all(work, error);
            die("version.txt cannot be replaced; no game files were changed");
        }

        std::string keptBackup;
        // A refill of the wide art at the same version is no update: the older build stays the one to roll back to.
        if (installedGamePresent && packageVersion != installedVersion && !backupInstalledBuild(root,
            installedVersion.empty() ? std::string("manual") : installedVersion, &keptBackup)) {
            fs::remove_all(work, error);
            die("could not save the current build before updating; nothing was changed");
        }

        // --- into the game folder; the player's things stay theirs ---
        log("installing into the game folder:");
        auto stopUpdate = [&]() {
            fs::remove_all(work, error);
            die("the update could not be installed (see the lines above); the build version was not changed");
        };
        if (!copyIfPresent(stage, kGameExe, root, true, "installed")
            || !copyIfPresent(stage, "ce.dat", root, true, "installed")
            || !copyIfPresent(stage, kExporterExe, root, true, "installed")) stopUpdate();
        if (!fs::exists(stage / kZipLauncher)) {
            logf("  %s: not in the package, skipped", kRootLauncher);
        } else if (copyLauncher(stage / kZipLauncher, root / kRootLauncher)) {
            logf("  %s: installed", kRootLauncher);
        } else {
            stopUpdate();
        }
        if (!copyTreeIfPresent(stage, "user-assets", root)
            || !copyTreeIfPresent(stage, "data/art/splash", root)
            || !copyTreeIfPresent(stage, "data/art/russian/splash", root)) stopUpdate();
        if (fs::exists(root / kCfg)) {
            log("  fallout2.cfg: kept (the player's file is never overwritten)");
        } else {
            if (!copyIfPresent(stage, kCfg, root, false, "installed (default)")) stopUpdate();
        }
        if (!writeBuildVersion(root, packageVersion)) stopUpdate();
        logf("  version.txt: %s", packageVersion.c_str());
        fs::remove_all(work, error);
        cleanOlderBuildLeftovers(root, keptBackup);
        updated = true;
        installedVersion = packageVersion;
    } else if (!remoteVersion.empty()) {
        logf("the installed build matches the release: nothing to pull");
    }

    // --- the Restoration Project, before the scenes: its maps and art go into them ---
    bool rpInstalledNow = false;
    if (options.installRp) {
        fs::path preflight;
        struct PreflightCleanup {
            fs::path* path;
            ~PreflightCleanup() { if (!path->empty()) { std::error_code ignored; fs::remove(*path, ignored); } }
        } cleanup{ &preflight };
        bool changingMode = !rpOnDisk(root);
        if (changingMode && options.voiceTranslation.empty()) {
            std::string oldVoice = installedVoiceId(root);
            std::string mapped = launcher_translations::voiceForMode(oldVoice, true);
            if (mapped != oldVoice) options.voiceTranslation = mapped;
        }
        const auto* requestedVoice = launcher_translations::findVoice(options.voiceTranslation);
        if (changingMode && !options.voiceTranslation.empty()) {
            if (!launcher_translations::voiceAvailable(options.voiceTranslation, true)) {
                log("requested voice is unavailable for Restoration Project");
                return 1;
            }
            if (requestedVoice) {
                preflight = fs::temp_directory_path() / ("f2r-voice-" + std::to_string(GetCurrentProcessId()) + ".dat");
                if (fs::exists(preflight)) { preflight.clear(); return 1; }
                if (!fetchVoice(*requestedVoice, preflight)) return 1;
            }
        }
        if (changingMode && !writeTextFile(root / kVoiceRpTransition, "F2R voice RP transition\n")) return 1;
        if (!installRestorationProject(root, &rpInstalledNow)) {
            if (!rpOnDisk(root) && !rpPresent(root / kRpManifest)) { std::error_code ignored; fs::remove(root / kVoiceRpTransition, ignored); }
            log("the Restoration Project could not be installed (see the lines above)");
            return 1;
        }
        std::string currentVoice = installedVoiceId(root);
        const auto* voicePackage = launcher_translations::findVoice(currentVoice);
        bool retireVoice = voicePackage && voicePackage->rp != rpOnDisk(root);
        std::string currentTextMarker = readWholeFile(root / "mods/f2r-text-choice.txt");
        std::string currentText = trim(currentTextMarker.substr(0, currentTextMarker.find('\n')));
        bool retireText = rpOnDisk(root) && launcher_translations::find(currentText);
        std::string desiredText = changingMode ? (options.textTranslation.empty() && options.installRussian ? "official" : options.textTranslation) : "";
        if (desiredText.empty() && retireText) desiredText = currentText;
        if (retireVoice || !desiredText.empty() || (changingMode && !options.voiceTranslation.empty())) {
            std::string desiredVoice = changingMode && !options.voiceTranslation.empty() ? options.voiceTranslation : retireVoice ? "original" : "";
            if (!changeTextTranslation(root, desiredText, {}, desiredVoice, false, preflight)) {
                if (rpInstalledNow && !removeRestorationProject(root))
                    log("could not restore the previous game after the translation change failed");
                else { std::error_code ignored; fs::remove(root / kVoiceRpTransition, ignored); }
                return 1;
            }
            if (changingMode) { options.voiceTranslation.clear(); options.textTranslation.clear(); options.installRussian = false; }
        }
        if (changingMode) { std::error_code markerError; if (!fs::remove(root / kVoiceRpTransition, markerError) || markerError) return 1; }
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

    if ((!options.textTranslation.empty() || !options.voiceTranslation.empty() || options.installRussian)
        && !changeTextTranslation(root, options.installRussian && options.textTranslation.empty() ? "official" : options.textTranslation, {}, options.voiceTranslation)) {
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
