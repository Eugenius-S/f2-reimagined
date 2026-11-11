#include "sfall_perks.h"

#include <stdint.h>
#include <string.h>

#include <vector>

namespace fallout {

namespace {

// sfall's FakePerk record (Modules/Perks.cpp), byte for byte, so saves move
// between sfall and CE.
#pragma pack(push, 1)
struct FakePerk {
    int32_t level;
    int32_t image;
    char name[64];
    char description[512];
    char reserved[506];
    int32_t ownerId; // 0: the player; CE has no party-member fake perks yet
    int16_t id; // -1: a script's fake perk
};
#pragma pack(pop)
static_assert(sizeof(FakePerk) == 1096, "sfall's FakePerk is 1096 bytes");

// sfall's interface text limit for a description.
constexpr size_t kDescriptionLength = 256;

std::vector<FakePerk> fakeTraits;
std::vector<FakePerk> fakePerks;
// shortcut: selectable perks (set_selectable_perk) are not implemented; their
// records are read and kept so a sfall save round-trips.
std::vector<FakePerk> fakeSelectablePerks;

} // namespace

// sfall's Perks::SetFakePerk / SetFakeTrait: level 0 removes the entry, a
// perk's level is capped at 100 and a trait's at 1.
// shortcut: the character screen does not list fake perks and traits yet;
// scripts see them through has_fake_perk and has_fake_trait.
void sfallFakePerkSet(bool trait, const char* name, int level, int image, const char* description)
{
    if (name == nullptr || level < 0) {
        return;
    }
    if (level > (trait ? 1 : 100)) {
        level = trait ? 1 : 100;
    }

    std::vector<FakePerk>& list = trait ? fakeTraits : fakePerks;
    for (auto it = list.begin(); it != list.end(); ++it) {
        if (it->ownerId != 0 || strcmp(it->name, name) != 0) {
            continue;
        }
        if (level == 0) {
            list.erase(it);
            return;
        }
        it->level = level;
        it->image = image;
        strncpy(it->description, description != nullptr ? description : "", kDescriptionLength - 1);
        it->description[kDescriptionLength - 1] = '\0';
        return;
    }

    if (level == 0) {
        return;
    }

    FakePerk perk = {};
    perk.level = level;
    perk.image = image;
    strncpy(perk.name, name, sizeof(perk.name) - 1);
    strncpy(perk.description, description != nullptr ? description : "", sizeof(perk.description) - 1);
    perk.id = -1;
    list.push_back(perk);
}

int sfallFakePerkHas(bool trait, const char* name)
{
    if (name == nullptr || name[0] == '\0') {
        return 0;
    }
    for (const FakePerk& perk : trait ? fakeTraits : fakePerks) {
        if (perk.ownerId == 0 && strcmp(perk.name, name) == 0) {
            return trait ? 1 : perk.level;
        }
    }
    return 0;
}

void sfallFakePerksReset()
{
    fakeTraits.clear();
    fakePerks.clear();
    fakeSelectablePerks.clear();
}

static bool saveList(File* stream, const std::vector<FakePerk>& list)
{
    // Counts are little-endian like the rest of sfallgv.sav, not db's big-endian.
    int32_t count = static_cast<int32_t>(list.size());
    if (fileWrite(&count, sizeof(count), 1, stream) != 1) {
        return false;
    }
    return list.empty() || fileWrite(list.data(), sizeof(FakePerk), list.size(), stream) == list.size();
}

static bool loadList(File* stream, std::vector<FakePerk>& list)
{
    int32_t count;
    if (fileRead(&count, sizeof(count), 1, stream) != 1 || count < 0 || count > 10000) {
        return false;
    }
    list.resize(count);
    if (count != 0 && fileRead(list.data(), sizeof(FakePerk), count, stream) != static_cast<size_t>(count)) {
        list.clear();
        return false;
    }
    for (FakePerk& perk : list) {
        perk.name[sizeof(perk.name) - 1] = '\0';
        perk.description[sizeof(perk.description) - 1] = '\0';
    }
    return true;
}

// sfall's Perks::Save order: traits, perks, selectable perks.
bool sfallFakePerksSave(File* stream)
{
    return saveList(stream, fakeTraits) && saveList(stream, fakePerks) && saveList(stream, fakeSelectablePerks);
}

bool sfallFakePerksLoad(File* stream)
{
    sfallFakePerksReset();
    return loadList(stream, fakeTraits) && loadList(stream, fakePerks) && loadList(stream, fakeSelectablePerks);
}

} // namespace fallout
