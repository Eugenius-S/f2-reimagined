#ifndef FALLOUT_SCENE_RENDERER_OUTLINE_MASK_H
#define FALLOUT_SCENE_RENDERER_OUTLINE_MASK_H

#include <algorithm>
#include <cmath>
#include <cstddef>

namespace fallout::scene_renderer {

struct OutlinePoint {
    double x;
    double y;
};

// Sample the existing palette mask at each covered destination pixel, rather
// than splatting one dot per source pixel. Corners are TL, TR, BR, BL, in
// screen coordinates, and describe the entire source image including padding.
inline bool drawProjectedOutlineMask(const unsigned char* source, int width, int height,
    const OutlinePoint (&p)[4], unsigned char* output, int stride,
    int left, int top, int right, int bottom)
{
    if (source == nullptr || output == nullptr || width <= 0 || height <= 0
        || left < 0 || top < 0 || right < left || bottom < top || right >= stride) return false;

    double minX = p[0].x, minY = p[0].y, maxX = p[0].x, maxY = p[0].y;
    for (const auto& v : p) {
        if (!std::isfinite(v.x) || !std::isfinite(v.y)) return false;
        minX = std::min(minX, v.x);
        minY = std::min(minY, v.y);
        maxX = std::max(maxX, v.x);
        maxY = std::max(maxY, v.y);
    }
    if (maxX < left || maxY < top || minX > right + 1.0 || minY > bottom + 1.0) return true;

    // Homography from the unit source rectangle to its projected quadrilateral.
    const double dx1 = p[1].x - p[2].x, dx2 = p[3].x - p[2].x;
    const double dy1 = p[1].y - p[2].y, dy2 = p[3].y - p[2].y;
    const double dx3 = p[0].x - p[1].x + p[2].x - p[3].x;
    const double dy3 = p[0].y - p[1].y + p[2].y - p[3].y;
    const double area = dx1 * dy2 - dx2 * dy1;
    if (std::abs(area) < 1e-9) return false;
    const double g = (dx3 * dy2 - dx2 * dy3) / area;
    const double h = (dx1 * dy3 - dx3 * dy1) / area;
    if (1.0 + g <= 0.0 || 1.0 + h <= 0.0 || 1.0 + g + h <= 0.0) return false;
    const double a = p[1].x - p[0].x + g * p[1].x;
    const double b = p[3].x - p[0].x + h * p[3].x;
    const double d = p[1].y - p[0].y + g * p[1].y;
    const double e = p[3].y - p[0].y + h * p[3].y;

    left = static_cast<int>(std::max<double>(left, std::floor(minX)));
    top = static_cast<int>(std::max<double>(top, std::floor(minY)));
    right = static_cast<int>(std::min<double>(right, std::ceil(maxX)));
    bottom = static_cast<int>(std::min<double>(bottom, std::ceil(maxY)));

    for (int y = top; y <= bottom; ++y) {
        const double sy = y + 0.5;
        const double m10 = d - sy * g, m11 = e - sy * h;
        const double rhs1 = sy - p[0].y;
        for (int x = left; x <= right; ++x) {
            const double sx = x + 0.5;
            const double m00 = a - sx * g, m01 = b - sx * h;
            const double rhs0 = sx - p[0].x;
            const double determinant = m00 * m11 - m01 * m10;
            if (std::abs(determinant) < 1e-9) continue;
            const double u = (rhs0 * m11 - m01 * rhs1) / determinant;
            const double v = (m00 * rhs1 - rhs0 * m10) / determinant;
            if (!(u >= 0.0 && u < 1.0 && v >= 0.0 && v < 1.0)) continue;
            const auto color = source[static_cast<std::size_t>(v * height) * width + static_cast<std::size_t>(u * width)];
            if (color != 0) output[static_cast<std::size_t>(y) * stride + x] = color;
        }
    }
    return true;
}

} // namespace fallout::scene_renderer

#endif /* FALLOUT_SCENE_RENDERER_OUTLINE_MASK_H */
