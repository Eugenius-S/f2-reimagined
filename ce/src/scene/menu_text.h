#ifndef FALLOUT_SCENE_MENU_TEXT_H
#define FALLOUT_SCENE_MENU_TEXT_H

#include <string>
#include <vector>
#include "../settings.h"

namespace fallout {
namespace scene {

// Returns legacy CP1251 bytes, matching the game's Russian fonts.
// Unknown languages and missing strings use the original English text.
const char* menuText(const char* english, const std::string& language);
bool hasMenuTranslation(const char* english);
std::string menuStatusText(const std::string& status, const std::string& language);

// The 3D toggle's refusal line (211): `reason` is one of "disabled",
// "no-map", "no-scene", "load-failed"; the map and elevation name what
// failed. Legacy CP1251 bytes, like every catalog string.
std::string toggleRefusalText(const char* reason, const std::string& map, int elevation,
    const std::string& language);
void localizeMenuDescriptors(std::vector<SettingDescriptor>& descriptors, const std::string& language);

} // namespace scene
} // namespace fallout
#endif
