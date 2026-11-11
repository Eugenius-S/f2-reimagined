#ifndef FALLOUT_INTERFACE_BAR_STYLE_H
#define FALLOUT_INTERFACE_BAR_STYLE_H

namespace fallout {

bool interfaceBarStyleRpAvailable(int barWidth);
void interfaceBarStyleNormalizeSelection(int barWidth);
const unsigned char* interfaceBarStyleImage(bool rp, int barWidth, int* width, int* height);
const unsigned char* interfaceBarStyleCurrentImage(int barWidth, int* width, int* height);
bool interfaceBarStyleCopyBackground(unsigned char* destination, int barWidth, bool preserveEndButtons = false);

} // namespace fallout

#endif /* FALLOUT_INTERFACE_BAR_STYLE_H */
