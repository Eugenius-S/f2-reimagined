#ifndef FALLOUT_SCENE_ACTION_MENU_SELECTION_H
#define FALLOUT_SCENE_ACTION_MENU_SELECTION_H

#include <algorithm>

namespace fallout::scene {
inline int actionMenuIndexAtY(int y, int top, int itemHeight, int count)
{
    if (itemHeight <= 0 || count <= 0 || y <= top) return 0;
    return std::min((y - top) / itemHeight, count - 1);
}
}
#endif
