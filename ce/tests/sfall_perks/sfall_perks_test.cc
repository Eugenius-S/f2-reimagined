#include "../../src/sfall_perks.h"

#include <cstdio>
#include <cstring>
#include <vector>

// A memory stand-in for db's raw reads and writes, so the test needs no DAT.
namespace fallout {

static std::vector<unsigned char> gBuffer;
static size_t gReadPosition = 0;

size_t fileWrite(const void* buf, size_t size, size_t count, File* stream)
{
    const unsigned char* bytes = static_cast<const unsigned char*>(buf);
    gBuffer.insert(gBuffer.end(), bytes, bytes + size * count);
    return count;
}

size_t fileRead(void* buf, size_t size, size_t count, File* stream)
{
    if (gReadPosition + size * count > gBuffer.size()) {
        return 0;
    }
    memcpy(buf, gBuffer.data() + gReadPosition, size * count);
    gReadPosition += size * count;
    return count;
}

} // namespace fallout

using namespace fallout;

static int failures = 0;

static void check(bool condition, const char* what)
{
    if (!condition) {
        printf("FAIL: %s\n", what);
        failures++;
    }
}

int main()
{
    File* stream = nullptr;

    sfallFakePerkSet(false, "Toaster", 1, 154, "repaired");
    sfallFakePerkSet(false, "Capped", 250, 1, "");
    sfallFakePerkSet(true, "Odd", 5, 2, "");
    check(sfallFakePerkHas(false, "Toaster") == 1, "a set perk has its level");
    check(sfallFakePerkHas(false, "Capped") == 100, "a perk level caps at 100");
    check(sfallFakePerkHas(true, "Odd") == 1, "a trait is on or off");
    check(sfallFakePerkHas(true, "Toaster") == 0, "perks and traits are separate");
    check(sfallFakePerkHas(false, "") == 0, "an empty name finds nothing");

    sfallFakePerkSet(false, "Toaster", 3, 154, "changed");
    check(sfallFakePerkHas(false, "Toaster") == 3, "setting again changes the level");

    check(sfallFakePerksSave(stream), "save");
    // sfall's layout: three lists, each a 4-byte count and 1096-byte records.
    check(gBuffer.size() == 4 + 1096 + 4 + 2 * 1096 + 4, "sfall's record layout");

    sfallFakePerksReset();
    check(sfallFakePerkHas(false, "Toaster") == 0, "reset clears");

    check(sfallFakePerksLoad(stream), "load");
    check(sfallFakePerkHas(false, "Toaster") == 3, "load restores a perk");
    check(sfallFakePerkHas(true, "Odd") == 1, "load restores a trait");

    sfallFakePerkSet(false, "Toaster", 0, 0, "");
    check(sfallFakePerkHas(false, "Toaster") == 0, "level 0 removes the perk");

    gBuffer.resize(6);
    gReadPosition = 0;
    check(!sfallFakePerksLoad(stream), "a short save fails to load");

    if (failures == 0) {
        printf("sfall_perks_tests: ok\n");
    }
    return failures == 0 ? 0 : 1;
}
