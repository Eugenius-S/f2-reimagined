#ifndef FALLOUT_MAP_EDGE_RECOVERY_H
#define FALLOUT_MAP_EDGE_RECOVERY_H

#include "geometry.h"
#include <algorithm>

namespace fallout {

inline bool mapEdgeRecoveryStep(const Rect& bounds, int oldX, int oldY, int newX, int newY,
    bool oldBottomAllowed, bool newBottomAllowed)
{
    const int oldDistance[] = { std::max(0, bounds.right - oldX), std::max(0, oldX - bounds.left),
        std::max(0, bounds.top - oldY), oldBottomAllowed ? 0 : std::max(0, oldY - bounds.bottom) };
    const int newDistance[] = { std::max(0, bounds.right - newX), std::max(0, newX - bounds.left),
        std::max(0, bounds.top - newY), newBottomAllowed ? 0 : std::max(0, newY - bounds.bottom) };
    bool closer = false;
    for (int i = 0; i < 4; ++i) {
        if (newDistance[i] > oldDistance[i]) return false;
        closer |= newDistance[i] < oldDistance[i];
    }
    return closer;
}

} // namespace fallout
#endif
