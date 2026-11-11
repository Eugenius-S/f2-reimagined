#ifndef FALLOUT_INTERFACE_BAR_STYLE_DECODE_H_
#define FALLOUT_INTERFACE_BAR_STYLE_DECODE_H_

#include <cstddef>
#include <vector>

namespace fallout {

bool interfaceBarStyleDecodeFrm(const unsigned char* bytes, size_t size, int expectedWidth, std::vector<unsigned char>& pixels);
int interfaceBarStyleResolveSelection(bool originalAvailable, bool rpAvailable, int selectedStyle);

} // namespace fallout

#endif // FALLOUT_INTERFACE_BAR_STYLE_DECODE_H_
