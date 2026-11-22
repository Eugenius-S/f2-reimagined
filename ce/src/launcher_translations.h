#pragma once

#include <cstddef>
#include <string>

namespace launcher_translations {

struct Package {
    const char* id;
    const char* filename;
    std::size_t size;
    const char* sha256;
};

inline constexpr Package fonts = { "fonts", "shared-legacy-fonts.dat", 28695,
    "17cd8cc116083daa580421001bc9738ef1280221a8959f8c4cbdacd424e1666b" };
inline constexpr Package texts[] = {
    { "1c", "steam-1c-text.dat", 1783877, "a89666c64983553b55d042b8a24b46f07ebc8ab782e7e6b1514d902a71d9d5f6" },
    { "fargus", "fio-fargus-text.dat", 1717961, "f6ad417e5f78c6498c22458fed0d14720fefc2a5ded5edc2575b826aa1d439de" },
    { "levy", "fio-levy-text.dat", 1614114, "1b735481a1c82a1c4ef320ed34c25e1b937ecfe218ce6ace0389efad3a572b9b" },
};

inline const Package* find(const std::string& id)
{
    for (const auto& item : texts) if (id == item.id) return &item;
    return nullptr;
}

// Direct HTTPS only, with loopback HTTP reserved for local tests.
inline bool validUrl(const std::string& url)
{
    if (url.compare(0, 8, "https://") == 0) return url.size() > 8;
    const std::string ip = "http://127.0.0.1:";
    const std::string host = "http://localhost:";
    size_t start = url.compare(0, ip.size(), ip) == 0 ? ip.size()
        : url.compare(0, host.size(), host) == 0 ? host.size() : 0;
    if (!start) return false;
    size_t end = url.find('/', start);
    if (end == std::string::npos) end = url.size();
    if (end == start) return false;
    for (size_t i = start; i < end; ++i) if (url[i] < '0' || url[i] > '9') return false;
    return true;
}

inline bool available(const std::string& id, bool rp, const std::string& textUrl, const std::string& fontsUrl)
{
    if (id == "original" || id == "official") return true;
    return !rp && find(id) != nullptr && validUrl(textUrl) && validUrl(fontsUrl);
}

}
