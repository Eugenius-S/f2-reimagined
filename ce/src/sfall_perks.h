#ifndef SFALL_PERKS_H
#define SFALL_PERKS_H

#include "db.h"

namespace fallout {

// sfall's fake perks and traits: set_fake_perk, set_fake_trait, has_fake_perk
// and has_fake_trait, kept in sfallgv.sav in sfall's own record layout.
void sfallFakePerkSet(bool trait, const char* name, int level, int image, const char* description);
int sfallFakePerkHas(bool trait, const char* name);
void sfallFakePerksReset();
bool sfallFakePerksSave(File* stream);
bool sfallFakePerksLoad(File* stream);

} // namespace fallout

#endif /* SFALL_PERKS_H */
