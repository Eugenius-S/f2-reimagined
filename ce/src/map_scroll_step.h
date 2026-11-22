#ifndef FALLOUT_MAP_SCROLL_STEP_H_
#define FALLOUT_MAP_SCROLL_STEP_H_

namespace fallout {

struct MapScrollStep {
    int x;
    int y;
};

// A diagonal rejected at a local 3D boundary can still pan along the
// unobstructed axis. The accepted delta must also drive the buffer refresh.
template <typename TryStep>
MapScrollStep chooseMapScrollStep(MapScrollStep requested, bool slideAtEdge, TryStep tryStep)
{
    if ((requested.x == 0 && requested.y == 0) || tryStep(requested)) return requested;
    if (slideAtEdge && requested.x != 0 && requested.y != 0) {
        const MapScrollStep horizontal { requested.x, 0 };
        if (tryStep(horizontal)) return horizontal;
        const MapScrollStep vertical { 0, requested.y };
        if (tryStep(vertical)) return vertical;
    }
    return { 0, 0 };
}

} // namespace fallout

#endif
