#ifndef FALLOUT_SCENE_DIALOGUE_FADE_H
#define FALLOUT_SCENE_DIALOGUE_FADE_H

#include <algorithm>
#include <cstdint>

namespace fallout::scene {

struct DialogueFade {
    float value = 0.0f;
    std::uint32_t tick = 0;
    bool opening = false;

    void advance(std::uint32_t now)
    {
        const float delta = static_cast<float>(now - tick) / 200.0f;
        tick = now;
        value = std::clamp(value + (opening ? delta : -delta), 0.0f, 1.0f);
    }

    void setOpening(bool target, std::uint32_t now)
    {
        advance(now);
        opening = target;
    }

    float opacity() const { return value * value * (3.0f - 2.0f * value); }
};

} // namespace fallout::scene

#endif
