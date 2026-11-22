#include "../../src/devtools/snapshot_rows.h"
#include "../../src/devtools/snapshot_palette.h"
#include "../../src/devtools/mark_repeat_guard.h"
#include "../../src/devtools/mark_rect.h"
#include "../../src/rgba_background_cursor.h"

#include <cstdio>
#include <vector>

using fallout::devtools::copySnapshotRowsTightly;

int main()
{
    const unsigned char shape[] = { 9, 0, 99, 0, 9, 99 };
    const fallout::RgbaBackgroundCursor cursor { shape, -1, 10, 2, 2, 3, 0 };
    if (!cursor.covers(-1, 10) || cursor.covers(0, 10)
        || cursor.covers(-1, 11) || !cursor.covers(0, 11)
        || cursor.covers(1, 10) || cursor.covers(0, 12)
        || fallout::RgbaBackgroundCursor {}.covers(0, 0)) return 23;
    fallout::owner_shot::MarkRepeatGuard<4> guard;
    const unsigned char pressed[] = { 1, 0, 0, 0 };
    guard.begin(pressed);
    guard.press(1, 1, true);
    guard.press(2, 2, true);
    if (!guard.blocks(0, false) || !guard.blocks(1, false) || !guard.blocks(2, false)) return 6;
    guard.release(0); guard.release(1); guard.release(2);
    if (guard.blocks(0, false) || guard.blocks(1, false) || guard.blocks(2, false)) return 7;
    if (!guard.blocks(3, true) || guard.blocks(3, false)) return 8;
    guard.press(0, 1, false);
    const unsigned char mappedHeld[] = { 0, 1, 0, 0 };
    guard.begin(mappedHeld);
    if (!guard.blocks(0, false) || !guard.blocks(1, false)) return 11;
    if (guard.release(0) != 1 || guard.blocks(0, false) || guard.blocks(1, false)) return 12;
    guard.press(2, 3, true);
    if (!guard.blocks(2, false) || !guard.blocks(3, false)) return 13;
    if (guard.release(2) != 3 || guard.blocks(2, false) || guard.blocks(3, false)) return 14;
    guard.press(0, 3, true); guard.press(1, 3, true);
    if (guard.release(0) != 3 || !guard.blocks(1, false) || !guard.blocks(3, false)) return 15;
    if (guard.release(1) != 3 || guard.blocks(3, false)) return 16;
    guard.press(0, 1, false);
    if (guard.release(0) != -1 || guard.blocks(1, false)) return 17;
    guard.press(0, 1, false);
    if (!guard.blocksNativeRepeat(0) || guard.blocksNativeRepeat(1)) return 18;
    if (guard.release(0) != -1 || guard.blocksNativeRepeat(0)) return 19;
    guard.press(2, decltype(guard)::Vetoed, true);
    if (!guard.blocksNativeRepeat(2) || !guard.blocks(2, false)) return 20;
    if (guard.release(2) != -1 || guard.blocksNativeRepeat(2) || guard.blocks(2, false)) return 21;
    guard.press(2, decltype(guard)::Vetoed, false);
    const unsigned char none[] = { 0, 0, 0, 0 };
    guard.begin(none);
    if (!guard.blocks(2, false)) return 22;
    guard.release(2);
    const auto rect = fallout::owner_shot::markDrawableRect(1366, 223, 1564, 389, 1720, 720, 0, 0, 3440, 1440, 3440, 1440);
    if (rect.x0 != 2732 || rect.y0 != 446 || rect.x1 - rect.x0 != 396 || rect.y1 - rect.y0 != 332) return 9;
    const auto clipped = fallout::owner_shot::markDrawableRect(-5, -5, 2000, 900, 1720, 720, 0, 0, 3440, 1440, 3440, 1440);
    if (clipped.x0 != 0 || clipped.y0 != 0 || clipped.x1 != 3439 || clipped.y1 != 1439) return 10;
    const auto colors = fallout::devtools::snapshotCyclePalette();
    bool hasBlueMonitor = false;
    for (int index = 233; index <= 237; ++index) {
        const int offset = (index - 229) * 3;
        if (colors[offset + 2] > colors[offset] && colors[offset + 2] > 30) hasBlueMonitor = true;
    }
    if (!hasBlueMonitor || colors[1] == 0) {
        std::fprintf(stderr, "frozen cycling palette left monitors or slime black\n");
        return 4;
    }
    for (auto channel : colors) {
        if (channel > 63) return 5;
    }
    constexpr int width = 1155;
    constexpr int height = 4;
    constexpr int pitch = 1160;
    std::vector<unsigned char> source(pitch * height, 0xE7);
    std::vector<unsigned char> tightlyPacked(width * height, 0xCD);

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            source[y * pitch + x] = static_cast<unsigned char>((x * 13 + y * 47) % 251);
        }
    }

    if (!copySnapshotRowsTightly(source.data(), width, height, pitch,
            tightlyPacked.data(), tightlyPacked.size())) {
        std::fprintf(stderr, "copySnapshotRowsTightly rejected a valid padded framebuffer\n");
        return 1;
    }
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            if (tightlyPacked[y * width + x] != source[y * pitch + x]) {
                std::fprintf(stderr, "row copy mismatch at (%d,%d)\n", x, y);
                return 2;
            }
        }
    }
    if (tightlyPacked[width] != source[pitch] || tightlyPacked[2 * width] != source[2 * pitch]
        || tightlyPacked[height * width - 1] == 0xE7) {
        std::fprintf(stderr, "row starts or final visible pixel do not match\n");
        return 3;
    }
    if (copySnapshotRowsTightly(source.data(), width, height, width - 1,
            tightlyPacked.data(), tightlyPacked.size())) {
        std::fprintf(stderr, "accepted a pitch smaller than row width\n");
        return 4;
    }
    return 0;
}
