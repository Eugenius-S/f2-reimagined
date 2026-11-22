#ifndef FALLOUT_MAP_SCROLL_STEP_H_
#define FALLOUT_MAP_SCROLL_STEP_H_

namespace fallout {

struct MapScrollStep {
    int x;
    int y;
};

enum class MapScrollAttempt {
    Blocked,
    Blit,
    Redrawn,
};

struct MapScrollResult {
    MapScrollStep step;
    MapScrollAttempt attempt;
};

// Only a rejected move can try another axis. A full redraw has already
// moved the camera and must not reuse coordinates from the previous center.
template <typename TryStep>
MapScrollResult chooseMapScrollStep(MapScrollStep requested, bool slideAtEdge, TryStep tryStep)
{
    if (requested.x == 0 && requested.y == 0) return { requested, MapScrollAttempt::Blocked };
    const auto diagonal = tryStep(requested);
    if (diagonal != MapScrollAttempt::Blocked) return { requested, diagonal };
    if (slideAtEdge && requested.x != 0 && requested.y != 0) {
        const MapScrollStep horizontal { requested.x, 0 };
        const auto horizontalAttempt = tryStep(horizontal);
        if (horizontalAttempt != MapScrollAttempt::Blocked) return { horizontal, horizontalAttempt };
        const MapScrollStep vertical { 0, requested.y };
        const auto verticalAttempt = tryStep(vertical);
        if (verticalAttempt != MapScrollAttempt::Blocked) return { vertical, verticalAttempt };
    }
    return { { 0, 0 }, MapScrollAttempt::Blocked };
}

} // namespace fallout

#endif
