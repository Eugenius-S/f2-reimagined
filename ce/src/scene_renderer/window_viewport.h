#ifndef FALLOUT_SCENE_RENDERER_WINDOW_VIEWPORT_H
#define FALLOUT_SCENE_RENDERER_WINDOW_VIEWPORT_H

#include <algorithm>
#include <cmath>

namespace fallout::scene_renderer {

struct LogicalOutputRect {
    int x = 0;
    int y = 0;
    int width = 0;
    int height = 0;
};

// Shared by the GL viewport and absolute mouse mapping. A small fractional
// DPI gap may stretch to fill the GL viewport; SDL keeps its own letterbox.
inline LogicalOutputRect fitLogicalOutput(int logicalWidth, int logicalHeight,
    int drawableWidth, int drawableHeight, bool expandNearFit)
{
    if (logicalWidth <= 0 || logicalHeight <= 0 || drawableWidth <= 0 || drawableHeight <= 0) return {};
    int width = drawableWidth;
    long long height = static_cast<long long>(drawableWidth) * logicalHeight / logicalWidth;
    if (height > drawableHeight) {
        height = drawableHeight;
        width = static_cast<int>(static_cast<long long>(drawableHeight) * logicalWidth / logicalHeight);
    }
    if (expandNearFit && drawableWidth - width <= 12 && drawableHeight - height <= 12) {
        width = drawableWidth;
        height = drawableHeight;
    }
    width = std::max(width, 1);
    const int finalHeight = std::max(static_cast<int>(height), 1);
    return { (drawableWidth - width) / 2, (drawableHeight - finalHeight) / 2, width, finalHeight };
}

struct WindowInputMapping {
    int logicalWidth = 0, logicalHeight = 0;
    int windowWidth = 0, windowHeight = 0;
    int drawableWidth = 0, drawableHeight = 0;
    LogicalOutputRect area;

    bool valid() const
    {
        return logicalWidth > 0 && logicalHeight > 0 && windowWidth > 0 && windowHeight > 0
            && drawableWidth > 0 && drawableHeight > 0 && area.width > 0 && area.height > 0;
    }

    // SDL window points can differ from framebuffer pixels at high DPI.
    // Return false inside the black bars, but keep the cursor clamped.
    bool toLogical(int& x, int& y) const
    {
        if (!valid()) return true;
        const double px = static_cast<double>(x) * drawableWidth / windowWidth - area.x;
        const double py = static_cast<double>(y) * drawableHeight / windowHeight - area.y;
        x = static_cast<int>(std::clamp(px * logicalWidth / area.width, 0.0, logicalWidth - 1.0));
        y = static_cast<int>(std::clamp(py * logicalHeight / area.height, 0.0, logicalHeight - 1.0));
        return px >= 0 && px < area.width && py >= 0 && py < area.height;
    }

    void toWindow(int x, int y, int& windowX, int& windowY) const
    {
        windowX = static_cast<int>(std::lround((area.x + static_cast<double>(x) * area.width / logicalWidth)
            * windowWidth / drawableWidth));
        windowY = static_cast<int>(std::lround((area.y + static_cast<double>(y) * area.height / logicalHeight)
            * windowHeight / drawableHeight));
    }
};

} // namespace fallout::scene_renderer

#endif
