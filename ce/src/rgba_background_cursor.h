#ifndef FALLOUT_RGBA_BACKGROUND_CURSOR_H_
#define FALLOUT_RGBA_BACKGROUND_CURSOR_H_

#include <cstddef>

namespace fallout {

struct RgbaBackgroundCursor {
    const unsigned char* pixels = nullptr;
    int x = 0;
    int y = 0;
    int width = 0;
    int height = 0;
    int pitch = 0;
    unsigned char transparent = 0;

    bool covers(int screenX, int screenY) const
    {
        const int localX = screenX - x;
        const int localY = screenY - y;
        return pixels != nullptr && localX >= 0 && localX < width
            && localY >= 0 && localY < height
            && pixels[static_cast<std::size_t>(localY) * pitch + localX] != transparent;
    }
};

} // namespace fallout

#endif
