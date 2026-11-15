#pragma once

// The launcher's download routes (task 235): a file is tried through each
// route in order and the next one is taken when a route fails.  No windows,
// no network here: the caller passes the function that tries one route.

#include <functional>
#include <string>
#include <vector>

namespace launcher_routes {

struct Route {
    std::string label; // "github.com", "api", "mirror": named in the log
    std::string url;
    // Not empty: url is a release JSON and this asset's download url is read from it.
    std::string apiAsset;
};

// A download is whole when the server named no length or every byte of it came.
// A dropped connection ends the read early and looks like the end of the file.
inline bool downloadComplete(size_t received, size_t declared)
{
    return received > 0 && (declared == 0 || received == declared);
}

// The first route `attempt` accepts wins; its label lands in *worked.  Every
// failure is reported through `attempt` itself (the caller logs it).  When
// all fail, *error holds the labels and reasons joined with "; ".
inline bool tryRoutes(const std::vector<Route>& routes,
    const std::function<bool(const Route&, std::string*)>& attempt, std::string* worked, std::string* error)
{
    error->clear();
    for (const Route& route : routes) {
        std::string reason;
        if (attempt(route, &reason)) {
            *worked = route.label;
            return true;
        }
        if (!error->empty()) *error += "; ";
        *error += route.label + ": " + reason;
    }
    return false;
}

// The api url of one asset in a release JSON (GitHub writes the asset's own
// "url" first and its "name" a few keys later, compact, no spaces).
inline std::string assetApiUrl(const std::string& json, const std::string& name)
{
    size_t at = json.find("\"name\":\"" + name + "\"");
    if (at == std::string::npos) return std::string();
    const std::string key = "\"url\":\"";
    size_t start = json.rfind(key, at);
    if (start == std::string::npos) return std::string();
    start += key.size();
    size_t end = json.find('"', start);
    return end == std::string::npos ? std::string() : json.substr(start, end - start);
}

} // namespace launcher_routes
