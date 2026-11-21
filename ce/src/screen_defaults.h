#ifndef FALLOUT_SCREEN_DEFAULTS_H
#define FALLOUT_SCREEN_DEFAULTS_H

namespace fallout {
struct FreshScreenDefaults {
    bool apply;
    int width;
    int height;
    double scale;
};
inline FreshScreenDefaults freshScreenDefaults(bool savedDisplay, int width, int height)
{
    const bool valid = width >= 640 && width <= 7680 && height >= 480 && height <= 4320;
    return { !savedDisplay, valid ? width : 640, valid ? height : 480, 2.0 };
}
}
#endif