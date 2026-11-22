#ifndef FALLOUT_MARK_RECT_H
#define FALLOUT_MARK_RECT_H

#include <algorithm>

namespace fallout::owner_shot {
struct DrawableMarkRect { int x0, y0, x1, y1; };
inline DrawableMarkRect markDrawableRect(int x0, int y0, int x1, int y1,
    int logicalWidth, int logicalHeight, int viewportX, int viewportY, int viewportWidth, int viewportHeight,
    int drawableWidth, int drawableHeight)
{
    if (logicalWidth <= 0 || logicalHeight <= 0 || drawableWidth <= 0 || drawableHeight <= 0) return {};
    const auto x = [&](int v) { return std::clamp(viewportX + v * viewportWidth / logicalWidth, 0, drawableWidth - 1); };
    const auto y = [&](int v) { return std::clamp(viewportY + v * viewportHeight / logicalHeight, 0, drawableHeight - 1); };
    return { x(std::min(x0, x1)), y(std::min(y0, y1)), x(std::max(x0, x1)), y(std::max(y0, y1)) };
}
} // namespace fallout::owner_shot
#endif
