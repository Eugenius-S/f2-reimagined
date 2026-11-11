#include "launcher_progress.h"
#include "launcher_ru.h"

#include <cstdio>
#include <cstdlib>
#include <string>

static void check(bool ok, const char* what)
{
    if (!ok) {
        std::printf("FAILED: %s\n", what);
        std::exit(1);
    }
}

// The conversions of a printf format in order, e.g. "%s %zu" -> "s zu"; "%%" is skipped.
static std::string conversions(const char* format)
{
    std::string out;
    for (const char* p = format; *p != '\0'; p++) {
        if (*p != '%') continue;
        p++;
        if (*p == '%') continue;
        while (*p != '\0' && std::string("sdlzu").find(*p) != std::string::npos) out += *p++;
        out += ' ';
        p--;
    }
    return out;
}

int main()
{
    // A Russian format must take the same arguments as its English one, or printf would crash.
    for (const launcher_ru::Entry& entry : launcher_ru::kTable) {
        check(conversions(entry.en) == conversions(entry.ru), entry.en);
    }
    check(std::string(launcher_ru::translate("unpacking")) == launcher_ru::kUnpackingRu, "unpacking is translated");
    check(std::string(launcher_ru::translate("no such line")) == "no such line", "an unknown line stays English");
    // Machine lines are never translated.
    for (const char* machine : { "RELEASE_VERSION:%s", "RELEASE_ERROR:%s", "  ... %zu / %lu MB" }) {
        check(std::string(launcher_ru::translate(machine)) == machine, machine);
    }
    // The tracker reads the Russian lines like the English ones.
    launcher_progress::Tracker t;
    t.feed(launcher_ru::kUnpackingRu);
    check(t.value == 10, "Russian unpack line moves the bar");
    launcher_progress::Tracker u;
    u.feed(std::string(launcher_ru::translate("making the 3D scene data for: %s")));
    check(u.value == 12, "Russian export line moves the bar");
    std::printf("launcher_ru_tests: ok\n");
    return 0;
}
