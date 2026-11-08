#include "launcher_routes.h"

#include <cstdio>
#include <cstdlib>

using namespace launcher_routes;

static void check(bool ok, const char* what)
{
    if (!ok) {
        std::printf("FAILED: %s\n", what);
        std::exit(1);
    }
}

int main()
{
    std::vector<Route> routes = { { "github.com", "a", "" }, { "api", "b", "x.zip" }, { "mirror", "c", "" } };
    std::vector<std::string> seen;
    std::string worked;
    std::string error;

    // The first route fails: the second one serves, the third is never tried.
    bool ok = tryRoutes(routes, [&](const Route& r, std::string* why) {
        seen.push_back(r.label);
        *why = "504";
        return r.label == "api";
    }, &worked, &error);
    check(ok && worked == "api", "second route wins");
    check(seen.size() == 2 && seen[0] == "github.com" && seen[1] == "api", "tried in order, stopped at the winner");
    check(error == "github.com: 504", "error keeps only the failed routes");

    // All fail: every route was tried, every reason is kept.
    seen.clear();
    ok = tryRoutes(routes, [&](const Route& r, std::string* why) {
        seen.push_back(r.label);
        *why = "down";
        return false;
    }, &worked, &error);
    check(!ok && seen.size() == 3, "all routes tried");
    check(error == "github.com: down; api: down; mirror: down", "all reasons joined");

    // The first route works: nothing else is touched.
    seen.clear();
    ok = tryRoutes(routes, [&](const Route& r, std::string*) { seen.push_back(r.label); return true; }, &worked, &error);
    check(ok && worked == "github.com" && seen.size() == 1 && error.empty(), "first route wins alone");

    // The asset's own url, not the uploader's.
    std::string json = "{\"assets\":[{\"url\":\"https://api.example/assets/1\",\"id\":1,\"name\":\"a.zip\","
                       "\"uploader\":{\"url\":\"https://api.example/users/u\"}},"
                       "{\"url\":\"https://api.example/assets/2\",\"id\":2,\"name\":\"b.zip\","
                       "\"uploader\":{\"url\":\"https://api.example/users/u\"}}]}";
    check(assetApiUrl(json, "b.zip") == "https://api.example/assets/2", "asset url by name");
    check(assetApiUrl(json, "c.zip").empty(), "missing asset gives nothing");

    std::printf("launcher_routes_tests: ok\n");
    return 0;
}
