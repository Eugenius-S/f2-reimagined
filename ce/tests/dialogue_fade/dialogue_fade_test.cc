#include "dialogue_fade.h"

#include <cmath>

using fallout::scene::DialogueFade;

int main()
{
    DialogueFade fade;
    fade.setOpening(true, 1000);
    fade.advance(1100);
    if (std::abs(fade.opacity() - 0.5f) > 0.001f) return 1;
    fade.setOpening(false, 1100);
    fade.advance(1150);
    if (std::abs(fade.value - 0.25f) > 0.001f) return 2;
    fade.setOpening(true, 1150);
    fade.advance(1300);
    if (fade.value != 1.0f) return 3;
    fade.setOpening(false, 1300);
    fade.advance(1500);
    if (fade.value != 0.0f || fade.opacity() != 0.0f) return 4;
    fade.setOpening(true, 0xfffffff0u);
    fade.advance(0x54u);
    if (std::abs(fade.value - 0.5f) > 0.001f) return 5;
    return 0;
}
