#include "sfall_gaps.h"

#include <stdio.h>

#include <map>
#include <set>
#include <string>

#include "map.h"

namespace fallout {

namespace {

struct SfallGap {
    int count = 0;
    std::set<std::string> places;
};

std::map<std::string, SfallGap> gaps;

} // namespace

void sfallGapNote(const char* kind, const char* name, Program* program)
{
    std::string key = std::string(kind) + " " + name;
    std::string place = std::string(program != nullptr && program->name != nullptr ? program->name : "?") + "@" + gMapHeader.name;
    SfallGap& gap = gaps[key];
    gap.count++;
    if (gap.places.insert(place).second) {
        printf("sfall gap: %s in %s\n", key.c_str(), place.c_str());
        fflush(stdout);
    }
}

void sfallGapReport()
{
    for (const auto& [key, gap] : gaps) {
        std::string places;
        for (const auto& place : gap.places) {
            places += (places.empty() ? "" : ",") + place;
        }
        printf("sfall gap summary: %s count=%d at=%s\n", key.c_str(), gap.count, places.c_str());
    }
    fflush(stdout);
    gaps.clear();
}

} // namespace fallout
