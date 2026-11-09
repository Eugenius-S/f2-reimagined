#include "text_present.h"

#include <algorithm>
#include <chrono>
#include <stdio.h>

#include <SDL.h>

#include "../color.h"
#include "../dev_output.h"
#include "../map.h"
#include "../scene/scene_overlay.h"
#include "../scene/scene_view.h"
#include "../scene/text_capture.h"
#include "../settings.h"
#include "../window_manager.h"
#include "context.h"
#include "post.h"
#include "scene_draw.h"

namespace fallout {
namespace scene_renderer {
namespace {

bool gSharpTextFailureLogged = false;
// 219: the keys of the two cached layers and whether one is stored.
// - The static layer (the masked game picture) changes only when the shown
//   pixels' indices or the non-cycling palette colors do.
// - The glyph layer changes only when the captured text records do.
// The palette's cycling entries (cycle.cc recolors 229-254, the bar's
// pulsing lights) change color every frame while their indices stay; those
// pixels ride the dynamic box instead of the keys.
unsigned long long gSharpStaticKey = 0;
bool gSharpStaticKeyValid = false;
unsigned long long gSharpGlyphKey = 0;
bool gSharpGlyphKeyValid = false;
int gSharpTextCacheHits = 0;
int gSharpTextCacheMisses = 0;
// The iso window's shown bytes, as this frame's scan read them (the mask
// rule the overlay draw applies).
std::vector<unsigned char> gSharpShownMask;
bool gSharpShownMaskValid = false;

PreparedSharpTextFrame& preparedFrameStorage()
{
    static PreparedSharpTextFrame value;
    return value;
}

// The palette entries color cycling drives (cycle.cc): slime, monitors,
// fire_slow, fire_fast, shoreline and the bobber.
bool sharpCyclingIndex(int index)
{
    return index >= 229 && index <= 254;
}

struct SharpFrameScan {
    unsigned long long staticKey = 0;
    bool maskAvailable = false;
    // The shown pixels whose palette entry cycles, one box around them.
    int dynamicX0 = 0;
    int dynamicY0 = 0;
    int dynamicX1 = -1;
    int dynamicY1 = -1;
    bool hasDynamic() const { return dynamicX1 >= dynamicX0; }
};

// The previous frame's raw rows and per-chunk hashes: a chunk whose mask and
// index bytes repeat the previous frame's reuses its stored hash, so an idle
// frame hashes only the chunks that changed (the fps counter, once a second)
// instead of every pixel. The hash stays a pure function of the content: the
// same bytes give the same chunk hashes, copied or recomputed.
std::vector<unsigned char> gSharpPrevIndices;
std::vector<unsigned char> gSharpPrevMask;
std::vector<unsigned long long> gSharpPrevChunkHashes;
bool gSharpPrevValid = false;
unsigned char gSharpPrevStaticPalette[229 * 3] = {};
unsigned long long gSharpPrevStaticKey = 0;
bool gSharpPrevStaticKeyValid = false;
// The cycling pixels' box, remembered: their positions do not move (the
// interface bar's pulsing lights), and chunks that repeat the previous frame
// never re-report them.
int gSharpDynamicBox[4] = { 0, 0, -1, -1 };
// The mouse cursor's chunk-aligned box (current and previous frame, padded):
// the game's cursor animates in the iso window's own bytes, so its chunks
// never repeat. They leave the key and the patch redraws the box instead.
int gSharpCursorBox[4] = { 0, 0, -1, -1 };
int gSharpPrevCursorBox[4] = { 0, 0, -1, -1 };

void sharpAlignChunkBox(int* box, int width, int height)
{
    box[0] = std::max(0, (box[0] / 8) * 8);
    box[1] = std::max(0, (box[1] / 8) * 8);
    box[2] = std::min(width - 1, box[2]);
    box[3] = std::min(height - 1, box[3]);
}

unsigned long long sharpChunkHash(const unsigned char* index, const unsigned char* mask, int count,
    int y, SharpFrameScan& scan)
{
    unsigned long long hash = 1469598103934665603ull;
    for (int x = 0; x < count; x++) {
        hash = (hash ^ mask[x]) * 1099511628211ull;
        if (mask[x] == 0) continue;
        const unsigned char value = index[x];
        if (sharpCyclingIndex(value)) {
            if (scan.dynamicX1 < scan.dynamicX0) {
                scan.dynamicX0 = scan.dynamicX1 = x;
                scan.dynamicY0 = scan.dynamicY1 = y;
            } else {
                if (x < scan.dynamicX0) scan.dynamicX0 = x;
                if (x > scan.dynamicX1) scan.dynamicX1 = x;
                if (y < scan.dynamicY0) scan.dynamicY0 = y;
                if (y > scan.dynamicY1) scan.dynamicY1 = y;
            }
        }
        hash = (hash ^ value) * 1099511628211ull;
    }
    return hash;
}

// The static layer's key: the sizes, the mask and index bytes the iso window
// shows (chunk-hashed, unchanged chunks reused), the non-cycling palette
// entries. The world's own pixels (tiles, critters) are never shown, so
// neither their bytes key it.
SharpFrameScan sharpScanFrame(const unsigned char* indices, int width, int height, int indexPitch,
    int physicalWidth, int physicalHeight, const unsigned char* paletteRgb, int paletteEntries)
{
    SharpFrameScan scan;
    unsigned long long hash = 1469598103934665603ull;
    auto mix = [&hash](const void* data, size_t size) {
        const unsigned char* bytes = static_cast<const unsigned char*>(data);
        for (size_t index = 0; index < size; index++) {
            hash = (hash ^ bytes[index]) * 1099511628211ull;
        }
    };
    const int dims[5] = { width, height, indexPitch, physicalWidth, physicalHeight };
    mix(dims, sizeof(dims));
    gSharpShownMask.resize(static_cast<size_t>(width) * height); // windowShownMask clears it
    gSharpShownMaskValid = false;
    if (!windowShownMask(gIsoWindow, gSharpShownMask.data(), width, height)) {
        // No mask: the overlay would show the whole frame (maskOverlay gives
        // up the same way), so key everything and never take the cached path.
        for (int y = 0; y < height; y++) {
            mix(indices + static_cast<size_t>(y) * indexPitch, static_cast<size_t>(width));
        }
        if (paletteRgb != nullptr) mix(paletteRgb, static_cast<size_t>(paletteEntries) * 3);
        scan.staticKey = hash;
        gSharpPrevValid = false;
        gSharpDynamicBox[3] = -1;
        return scan;
    }
    gSharpShownMaskValid = true;
    scan.maskAvailable = true;
    // The mouse cursor's box: chunk-aligned, the union of this frame's and
    // the previous one's, so a moved cursor patches both places.
    gSharpPrevCursorBox[0] = gSharpCursorBox[0];
    gSharpPrevCursorBox[1] = gSharpCursorBox[1];
    gSharpPrevCursorBox[2] = gSharpCursorBox[2];
    gSharpPrevCursorBox[3] = gSharpCursorBox[3];
    TextCursorSnapshot cursor;
    if (textCaptureGetCursorSnapshot(&cursor) && cursor.visible
        && cursor.x + cursor.width > 0 && cursor.y + cursor.height > 0
        && cursor.x < width && cursor.y < height) {
        gSharpCursorBox[0] = cursor.x;
        gSharpCursorBox[1] = cursor.y;
        gSharpCursorBox[2] = cursor.x + cursor.width - 1;
        gSharpCursorBox[3] = cursor.y + cursor.height - 1;
    } else {
        gSharpCursorBox[0] = 0;
        gSharpCursorBox[1] = 0;
        gSharpCursorBox[2] = -1;
        gSharpCursorBox[3] = -1;
    }
    int excludeBox[4];
    if (gSharpCursorBox[2] >= gSharpCursorBox[0] && gSharpPrevCursorBox[2] >= gSharpPrevCursorBox[0]) {
        excludeBox[0] = std::min(gSharpCursorBox[0], gSharpPrevCursorBox[0]);
        excludeBox[1] = std::min(gSharpCursorBox[1], gSharpPrevCursorBox[1]);
        excludeBox[2] = std::max(gSharpCursorBox[2], gSharpPrevCursorBox[2]);
        excludeBox[3] = std::max(gSharpCursorBox[3], gSharpPrevCursorBox[3]);
    } else if (gSharpCursorBox[2] >= gSharpCursorBox[0]) {
        excludeBox[0] = gSharpCursorBox[0];
        excludeBox[1] = gSharpCursorBox[1];
        excludeBox[2] = gSharpCursorBox[2];
        excludeBox[3] = gSharpCursorBox[3];
    } else {
        excludeBox[0] = 0;
        excludeBox[1] = 0;
        excludeBox[2] = -1;
        excludeBox[3] = -1;
    }
    if (excludeBox[2] >= excludeBox[0]) {
        const int pad = 2;
        excludeBox[0] = std::max(0, excludeBox[0] - pad);
        excludeBox[1] = std::max(0, excludeBox[1] - pad);
        excludeBox[2] = std::min(width - 1, excludeBox[2] + pad);
        excludeBox[3] = std::min(height - 1, excludeBox[3] + pad);
        sharpAlignChunkBox(excludeBox, width, height);
    }
    // The cycling pixels sit still (the bar's pulsing lights): their last box
    // stays the patch's target on frames whose chunks all repeat.
    scan.dynamicX0 = gSharpDynamicBox[0];
    scan.dynamicY0 = gSharpDynamicBox[1];
    scan.dynamicX1 = gSharpDynamicBox[2];
    scan.dynamicY1 = gSharpDynamicBox[3];
    const int chunkWidth = 8;
    const int chunksPerRow = (width + chunkWidth - 1) / chunkWidth;
    // The previous frame's rows are updated in place, chunk by chunk, only
    // where they differ: an idle frame copies nothing.
    const size_t indexBytes = static_cast<size_t>(indexPitch) * height;
    const size_t chunkCount = static_cast<size_t>(chunksPerRow) * height;
    if (gSharpPrevIndices.size() != indexBytes || gSharpPrevMask.size() != gSharpShownMask.size()
        || gSharpPrevChunkHashes.size() != chunkCount) {
        gSharpPrevIndices.assign(indices, indices + indexBytes);
        gSharpPrevMask = gSharpShownMask;
        gSharpPrevChunkHashes.assign(chunkCount, 0);
        gSharpPrevValid = false;
    }
    // The static palette bytes this key covers; when neither they nor any
    // chunk moved, the key is the previous frame's and the mixing is skipped.
    unsigned char staticPalette[229 * 3] = {};
    if (paletteRgb != nullptr) {
        std::memcpy(staticPalette, paletteRgb, sizeof(staticPalette));
    }
    const bool paletteSame = gSharpPrevValid
        && std::memcmp(staticPalette, gSharpPrevStaticPalette, sizeof(staticPalette)) == 0;
    bool anyChunkChanged = !gSharpPrevValid || !paletteSame;
    for (int y = 0; y < height; y++) {
        const unsigned char* index = indices + static_cast<size_t>(y) * indexPitch;
        unsigned char* prevIndex = gSharpPrevIndices.data() + static_cast<size_t>(y) * indexPitch;
        const unsigned char* mask = gSharpShownMask.data() + static_cast<size_t>(y) * width;
        unsigned char* prevMask = gSharpPrevMask.data() + static_cast<size_t>(y) * width;
        // A row whose bytes all repeat reuses every chunk hash, as the chunk
        // branches below would: two row compares instead of a chunk loop.
        const bool rowExcluded = excludeBox[2] >= excludeBox[0] && y >= excludeBox[1] && y <= excludeBox[3];
        if (gSharpPrevValid && !rowExcluded && std::memcmp(mask, prevMask, width) == 0
            && std::memcmp(index, prevIndex, width) == 0) {
            const unsigned long long* rowHashes = gSharpPrevChunkHashes.data() + static_cast<size_t>(y) * chunksPerRow;
            for (int chunk = 0; chunk < chunksPerRow; chunk++) hash = hash * 1099511628211ull ^ rowHashes[chunk];
            continue;
        }
        for (int chunk = 0; chunk < chunksPerRow; chunk++) {
            const int x0 = chunk * chunkWidth;
            const int count = std::min(chunkWidth, width - x0);
            unsigned long long chunkHash;
            // The cursor's chunks key a constant and patch instead: the
            // game's own cursor animates in these very bytes.
            const bool excluded = excludeBox[2] >= excludeBox[0]
                && x0 + count > excludeBox[0] && x0 <= excludeBox[2]
                && y >= excludeBox[1] && y <= excludeBox[3];
            const bool maskBytesSame = std::memcmp(mask + x0, prevMask + x0, count) == 0;
            const bool indexBytesSame = std::memcmp(index + x0, prevIndex + x0, count) == 0;
            const bool maskSame = !gSharpPrevValid || maskBytesSame;
            bool allHidden = true;
            for (int x = x0; allHidden && x < x0 + count; x++) allHidden = mask[x] == 0;
            if (excluded) {
                // A constant: the box is patched from the surface each frame.
                chunkHash = 1469598103934665603ull;
            } else if (allHidden && maskSame) {
                // Nothing shown here: the world's own bytes never reach the
                // overlay, the chunk keys the same whatever they are.
                chunkHash = gSharpPrevValid
                    ? gSharpPrevChunkHashes[static_cast<size_t>(y) * chunksPerRow + chunk]
                    : 1469598103934665603ull;
            } else if (maskSame && gSharpPrevValid && indexBytesSame) {
                chunkHash = gSharpPrevChunkHashes[static_cast<size_t>(y) * chunksPerRow + chunk];
            } else {
                anyChunkChanged = true;
                chunkHash = sharpChunkHash(index + x0, mask + x0, count, y, scan);
            }
            gSharpPrevChunkHashes[static_cast<size_t>(y) * chunksPerRow + chunk] = chunkHash;
            if (!maskBytesSame) std::memcpy(prevMask + x0, mask + x0, count);
            if (!indexBytesSame) std::memcpy(prevIndex + x0, index + x0, count);
            hash = hash * 1099511628211ull ^ chunkHash;
        }
    }
    std::memcpy(gSharpPrevStaticPalette, staticPalette, sizeof(gSharpPrevStaticPalette));
    gSharpPrevValid = true;
    // Nothing shown moved and the static palette is the same: the key is the
    // previous frame's, no mixing needed.
    scan.staticKey = anyChunkChanged
        ? hash * 1099511628211ull
        : (gSharpPrevStaticKeyValid ? gSharpPrevStaticKey : hash * 1099511628211ull);
    gSharpPrevStaticKey = scan.staticKey;
    gSharpPrevStaticKeyValid = true;
    gSharpDynamicBox[0] = scan.dynamicX0;
    gSharpDynamicBox[1] = scan.dynamicY0;
    gSharpDynamicBox[2] = scan.dynamicX1;
    gSharpDynamicBox[3] = scan.dynamicY1;
    return scan;
}

// The glyph layer's key: the static one plus everything the glyph renderer
// reads from the captured runs.
unsigned long long sharpGlyphKey(unsigned long long staticKey, const std::vector<scene::TextRecord>& records)
{
    unsigned long long hash = staticKey;
    auto mix = [&hash](const void* data, size_t size) {
        const unsigned char* bytes = static_cast<const unsigned char*>(data);
        for (size_t index = 0; index < size; index++) {
            hash = (hash ^ bytes[index]) * 1099511628211ull;
        }
    };
    const size_t recordCount = records.size();
    mix(&recordCount, sizeof(recordCount));
    for (const scene::TextRecord& record : records) {
        mix(record.run.text.data(), record.run.text.size());
        const int fields[16] = {
            record.run.font, static_cast<int>(record.run.sizeRole),
            static_cast<int>(record.run.colorAndFlags), static_cast<int>(record.run.colorRole),
            static_cast<int>(record.run.outlineColor), record.run.hasOutline ? 1 : 0,
            record.run.x, record.run.y, record.run.width, record.run.cellWidth,
            record.run.lineHeight, record.run.verticalAlignmentBoxY,
            record.run.verticalAlignmentBoxHeight, record.run.sharpTextOffsetY,
            record.run.sharpRightAlignX, record.run.sharpEligible ? 1 : 0,
        };
        mix(fields, sizeof(fields));
        mix(&record.bounds, sizeof(record.bounds));
        mix(record.run.characterX.data(), record.run.characterX.size() * sizeof(int));
        mix(record.run.characterCellX.data(), record.run.characterCellX.size() * sizeof(int));
    }
    return hash;
}

// The glyph layer from the captured runs: visibility masks, the render, and
// the frame keeps the runs and masks for the cycling-colors patch.
bool renderGlyphOverlay(const std::vector<scene::TextRecord>& records, PreparedSharpTextFrame& output,
    int physicalWidth, int physicalHeight, const unsigned char* paletteRgb, int paletteEntries)
{
    std::vector<scene::TextVisibilityMask> masks(records.size());
    for (std::size_t index = 0; index < records.size(); ++index) {
        if (!scene::textCaptureVisibilityMask(records[index], output.width, output.height, &masks[index])) return false;
    }
    if (!renderSharpText(records, output.width, output.height, physicalWidth, physicalHeight,
            paletteRgb, paletteEntries, masks, &output.overlay)) return false;
    output.records = records;
    output.masks = std::move(masks);
    output.runCount = static_cast<unsigned int>(records.size());
    output.glyphCount = 0;
    for (const scene::TextRecord& record : records) {
        output.glyphCount += static_cast<unsigned int>(record.run.text.size());
    }
    return true;
}

// The cycling colors' box, straight over the cached quad texture: expand,
// text-restore and mask just the box, then upload it at its offset. The
// cached layers and the text pixels inside the box come from the frame the
// cache was built from, which the keys just matched.
void patchCyclingRegion(SDL_Surface* indexedSurface, SDL_Surface* rgbaSurface,
    const std::vector<scene::TextRecord>& records, const std::vector<scene::TextVisibilityMask>& masks,
    const unsigned char* paletteRgb, int paletteEntries, const SharpFrameScan& scan)
{
    if (!gSharpShownMaskValid || scan.dynamicX1 < scan.dynamicX0) return;
    const int width = indexedSurface->w;
    const int height = indexedSurface->h;
    const int x0 = std::max(0, scan.dynamicX0);
    const int y0 = std::max(0, scan.dynamicY0);
    const int x1 = std::min(width - 1, scan.dynamicX1);
    const int y1 = std::min(height - 1, scan.dynamicY1);
    if (x1 < x0 || y1 < y0) return;
    SDL_Rect rect = { x0, y0, x1 - x0 + 1, y1 - y0 + 1 };
    if (SDL_BlitSurface(indexedSurface, &rect, rgbaSurface, &rect) != 0) return;
    const unsigned char* indices = static_cast<const unsigned char*>(indexedSurface->pixels);
    const int indexPitch = indexedSurface->pitch;
    unsigned char* rgba = static_cast<unsigned char*>(rgbaSurface->pixels);
    const int rgbaPitch = rgbaSurface->pitch;
    // The text restores inside the box: the same rule the full prepare
    // applies, restricted to it.
    for (size_t index = 0; index < records.size() && index < masks.size(); index++) {
        const scene::TextRecord& record = records[index];
        const scene::TextVisibilityMask& mask = masks[index];
        for (const scene::TextPixelRecord& pixel : record.pixels) {
            if (pixel.x < x0 || pixel.x > x1 || pixel.y < y0 || pixel.y > y1) continue;
            if (pixel.x < 0 || pixel.y < 0 || pixel.x >= width || pixel.y >= height
                || pixel.x >= mask.bounds.x + mask.bounds.width || pixel.y >= mask.bounds.y + mask.bounds.height
                || !mask.visibleAt(pixel.x, pixel.y)
                || indices[static_cast<size_t>(pixel.y) * indexPitch + pixel.x] != pixel.after
                || pixel.before >= paletteEntries) continue;
            unsigned char* rgbaPixel = rgba + static_cast<size_t>(pixel.y) * rgbaPitch + pixel.x * 4;
            const unsigned char* color = paletteRgb + pixel.before * 3;
            rgbaPixel[0] = color[0];
            rgbaPixel[1] = color[1];
            rgbaPixel[2] = color[2];
            rgbaPixel[3] = 255;
        }
    }
    // The overlay's own mask rule over the box.
    for (int y = y0; y <= y1; y++) {
        const unsigned char* index = indices + static_cast<size_t>(y) * indexPitch;
        const unsigned char* shown = gSharpShownMask.data() + static_cast<size_t>(y) * width;
        unsigned char* pixel = rgba + static_cast<size_t>(y) * rgbaPitch;
        for (int x = x0; x <= x1; x++, pixel += 4) {
            if (shown[x] != 0 && index[x] == COLOR_FIRST) {
                pixel[0] = pixel[1] = pixel[2] = pixel[3] = 0;
            } else {
                pixel[3] = 255;
            }
        }
    }
    updateSceneOverlayRegion(rgba + static_cast<size_t>(y0) * rgbaPitch + x0 * 4,
        rgbaPitch, x0, y0, rect.w, rect.h);
}

} // namespace

void disableSharpText(const char* message)
{
    settings.scene3d.sharp_text = false;
    preparedFrameStorage().active = false;
    preparedFrameStorage().drawn = false;
    gSharpStaticKeyValid = false;
    gSharpGlyphKeyValid = false;
    if (gSharpTextFailureLogged) return;
    gSharpTextFailureLogged = true;
    const char* reason = message != nullptr ? message : "rendering failed";
    fprintf(stderr, "sharp text: %s; using pixel text\n", reason);
    fflush(stderr);
    // The owner never sees stderr: the failure also goes to the logs folder.
    if (FILE* log = fopen((dev_output::outputDirectory("logs") / "scene.log").string().c_str(), "a")) {
        fprintf(log, "sharp text: %s; using pixel text\n", reason);
        fclose(log);
    }
}

bool prepareSharpTextForPresent(SDL_Surface* indexedSurface, SDL_Surface* rgbaSurface, bool enabled)
{
    const auto started = std::chrono::steady_clock::now();
    PreparedSharpTextFrame& output = preparedFrameStorage();
    output.active = false;
    output.drawn = false;
    output.cached = false;
    output.runCount = 0;
    output.glyphCount = 0;
    output.restoredPixelCount = 0;
    output.prepareMilliseconds = 0.0;
    if (settings.scene3d.sharp_text && !sharpTextLanguageSupported(settings.system.language)) {
        prepareSharpTextFrame(nullptr, 0, 0, 0, nullptr, 0, 0, 0, nullptr, 0, false);
        disableSharpText("unsupported language; using pixel text (English/Russian only)");
        return false;
    }
    if (!enabled || !scene::sharpTextWanted() || indexedSurface == nullptr || rgbaSurface == nullptr) {
        prepareSharpTextFrame(nullptr, 0, 0, 0, nullptr, 0, 0, 0, nullptr, 0, false);
        return false;
    }

    const scene::TextBufferId buffer = textCaptureBufferId(indexedSurface->pixels);
    if (buffer == 0) return false;
    // The restoration call prunes records whose pixels no longer match.
    scene::textCaptureSharpRestoration(buffer);
    const std::vector<scene::TextRecord> records = scene::textCaptureSharpRecords(buffer);
    if (records.empty()) {
        prepareSharpTextFrame(nullptr, 0, 0, 0, nullptr, 0, 0, 0, nullptr, 0, false);
        return false;
    }
    int physicalWidth = 0;
    int physicalHeight = 0;
    if (!sharpTextOutputSize(&physicalWidth, &physicalHeight)) {
        disableSharpText("could not determine output size");
        return false;
    }
    SDL_Palette* palette = indexedSurface->format != nullptr ? indexedSurface->format->palette : nullptr;
    if (palette == nullptr || palette->colors == nullptr || palette->ncolors <= 0) {
        disableSharpText("game palette unavailable");
        return false;
    }
    unsigned char paletteRgb[256 * 3] = {};
    const int paletteEntries = std::min(256, palette->ncolors);
    for (int index = 0; index < paletteEntries; ++index) {
        paletteRgb[index * 3] = palette->colors[index].r;
        paletteRgb[index * 3 + 1] = palette->colors[index].g;
        paletteRgb[index * 3 + 2] = palette->colors[index].b;
    }
    // 219: two cached layers. The static layer (the masked game picture in
    // the quad texture) and the glyph layer (the sharp text texture) rebuild
    // only when their own inputs change; the palette's cycling colors patch
    // their small region straight over the cached texture.
    const unsigned char* indicesData = static_cast<const unsigned char*>(indexedSurface->pixels);
    const SharpFrameScan scan = sharpScanFrame(indicesData,
        indexedSurface->w, indexedSurface->h, indexedSurface->pitch,
        physicalWidth, physicalHeight, paletteRgb, paletteEntries);
    const unsigned long long glyphKey = sharpGlyphKey(scan.staticKey, records);
    // The stored keys imply the layers exist at these dims: they are stored
    // only by the full build that left the textures in place.
    const bool sameFrame = output.width == indexedSurface->w && output.height == indexedSurface->h
        && output.indexPitch == indexedSurface->pitch;
    // 157 diagnostics: F2CE_NO_SHARP_CACHE forces the full rebuild each frame.
    const bool cacheAllowed = SDL_getenv("F2CE_NO_SHARP_CACHE") == nullptr;
    const bool staticHit = cacheAllowed && gSharpStaticKeyValid && gSharpStaticKey == scan.staticKey && sameFrame;
    const bool glyphHit = cacheAllowed && gSharpGlyphKeyValid && gSharpGlyphKey == glyphKey && sameFrame;
    if (staticHit && glyphHit) {
        // The picture is byte-for-byte the one the cached layers were built
        // from. The moving small parts patch straight over the cached
        // texture: the palette's cycling colors and the mouse cursor.
        if (scan.maskAvailable && scan.hasDynamic()) {
            patchCyclingRegion(indexedSurface, rgbaSurface, output.records, output.masks,
                paletteRgb, paletteEntries, scan);
        }
        if (scan.maskAvailable && gSharpCursorBox[2] >= gSharpCursorBox[0]) {
            SharpFrameScan cursorPatch;
            cursorPatch.dynamicX0 = gSharpCursorBox[0];
            cursorPatch.dynamicY0 = gSharpCursorBox[1];
            cursorPatch.dynamicX1 = gSharpCursorBox[2];
            cursorPatch.dynamicY1 = gSharpCursorBox[3];
            patchCyclingRegion(indexedSurface, rgbaSurface, output.records, output.masks,
                paletteRgb, paletteEntries, cursorPatch);
        }
        output.active = true;
        output.drawn = false;
        output.cached = true;
        output.prepareMilliseconds = 0.0;
        gSharpTextCacheHits++;
        return true;
    }
    gSharpTextCacheMisses++;
    {
        static int reports = 0;
        if (getenv("F2CE_PERF") != nullptr && ++reports % 120 == 0) {
            printf("perf sharp cache hits=%d misses=%d static=%llx (stored %llx valid=%d) glyph=%llx (stored %llx valid=%d) maskAv=%d iso=%d\n",
                gSharpTextCacheHits, gSharpTextCacheMisses,
                scan.staticKey, gSharpStaticKey, gSharpStaticKeyValid ? 1 : 0,
                glyphKey, gSharpGlyphKey, gSharpGlyphKeyValid ? 1 : 0,
                scan.maskAvailable ? 1 : 0, gIsoWindow);
            fflush(stdout);
        }
    }
    if (staticHit) {
        // The game's picture is unchanged; only the text records moved. The
        // frame copies and the restore are still good - rebuild the glyphs.
        gSharpGlyphKeyValid = true;
        gSharpGlyphKey = glyphKey;
        // 233: the top of this function cleared active; without this the draw
        // below found the frame inactive and switched sharp text off.
        output.active = true;
        output.cached = true;
        if (!renderGlyphOverlay(records, output, physicalWidth, physicalHeight, paletteRgb, paletteEntries)) {
            disableSharpText("capture or glyph rendering failed");
            return false;
        }
        const PreparedSharpTextFrame& frame = preparedSharpTextFrame();
        if (!uploadSharpTextOverlay(frame.overlay.pixels.data(), frame.overlay.width,
                frame.overlay.height, frame.overlay.pitch)) {
            disableSharpText("OpenGL texture upload failed");
            return false;
        }
        output.prepareMilliseconds = std::chrono::duration<double, std::milli>(
            std::chrono::steady_clock::now() - started).count();
        return true;
    }
    gSharpStaticKeyValid = true;
    gSharpStaticKey = scan.staticKey;
    gSharpGlyphKeyValid = true;
    gSharpGlyphKey = glyphKey;
    if (SDL_BlitSurface(indexedSurface, nullptr, rgbaSurface, nullptr) != 0) {
        disableSharpText("palette expansion failed");
        return false;
    }
    if (!prepareSharpTextFrame(indicesData,
            indexedSurface->w, indexedSurface->h, indexedSurface->pitch,
            static_cast<const unsigned char*>(rgbaSurface->pixels), rgbaSurface->pitch,
            physicalWidth, physicalHeight, paletteRgb, paletteEntries, true)) {
        disableSharpText("capture or glyph rendering failed");
        return false;
    }
    const PreparedSharpTextFrame& frame = preparedSharpTextFrame();
    if (!frame.active) return false;
    if (!uploadSharpTextOverlay(frame.overlay.pixels.data(), frame.overlay.width,
            frame.overlay.height, frame.overlay.pitch)) {
        disableSharpText("OpenGL texture upload failed");
        return false;
    }
    output.prepareMilliseconds = std::chrono::duration<double, std::milli>(
        std::chrono::steady_clock::now() - started).count();
    return true;
}

bool drawPreparedSharpTextOverlay()
{
    PreparedSharpTextFrame& output = preparedFrameStorage();
    if (!output.active || !drawSharpTextOverlay()) return false;
    output.drawn = true;
    return true;
}

bool prepareSharpTextFrame(const unsigned char* indices, int width, int height, int indexPitch,
    const unsigned char* rgba, int rgbaPitch, int physicalWidth, int physicalHeight,
    const unsigned char* paletteRgb, int paletteEntries, bool enabled)
{
    PreparedSharpTextFrame& output = preparedFrameStorage();
    output.active = false;
    output.drawn = false;
    output.cached = false;
    output.runCount = 0;
    output.glyphCount = 0;
    output.restoredPixelCount = 0;
    output.prepareMilliseconds = 0.0;
    if (!enabled) return true;
    if (indices == nullptr || rgba == nullptr || width <= 0 || height <= 0
        || indexPitch < width || rgbaPitch < width * 4 || physicalWidth <= 0 || physicalHeight <= 0
        || paletteRgb == nullptr || paletteEntries <= 0) return false;
    output.width = width;
    output.height = height;
    output.indexPitch = indexPitch;
    output.rgbaPitch = rgbaPitch;
    output.originalIndices.resize(static_cast<std::size_t>(indexPitch) * height);
    output.originalRgba.resize(static_cast<std::size_t>(rgbaPitch) * height);
    output.restoredIndices.resize(static_cast<std::size_t>(indexPitch) * height);
    output.restoredRgba.resize(static_cast<std::size_t>(rgbaPitch) * height);
    for (int y = 0; y < height; ++y) {
        std::copy(indices + static_cast<std::size_t>(y) * indexPitch,
            indices + static_cast<std::size_t>(y) * indexPitch + indexPitch,
            output.originalIndices.data() + static_cast<std::size_t>(y) * indexPitch);
        std::copy(rgba + static_cast<std::size_t>(y) * rgbaPitch,
            rgba + static_cast<std::size_t>(y) * rgbaPitch + rgbaPitch,
            output.originalRgba.data() + static_cast<std::size_t>(y) * rgbaPitch);
        std::copy(output.originalIndices.data() + static_cast<std::size_t>(y) * indexPitch,
            output.originalIndices.data() + static_cast<std::size_t>(y) * indexPitch + indexPitch,
            output.restoredIndices.data() + static_cast<std::size_t>(y) * indexPitch);
        std::copy(output.originalRgba.data() + static_cast<std::size_t>(y) * rgbaPitch,
            output.originalRgba.data() + static_cast<std::size_t>(y) * rgbaPitch + rgbaPitch,
            output.restoredRgba.data() + static_cast<std::size_t>(y) * rgbaPitch);
    }

    const scene::TextBufferId buffer = textCaptureBufferId(indices);
    if (buffer == 0) return false;
    scene::textCaptureSharpRestoration(buffer); // Validate all records, but restore only tagged green dialog runs.
    const std::vector<scene::TextRecord> records = scene::textCaptureSharpRecords(buffer);
    if (records.empty()) return true;
    if (!renderGlyphOverlay(records, output, physicalWidth, physicalHeight, paletteRgb, paletteEntries)) return false;

    for (std::size_t index = 0; index < records.size() && index < output.masks.size(); ++index) {
        const scene::TextRecord& record = records[index];
        const scene::TextVisibilityMask& mask = output.masks[index];
        for (const scene::TextPixelRecord& pixel : record.pixels) {
            if (pixel.x < 0 || pixel.y < 0 || pixel.x >= width || pixel.y >= height
                || pixel.x >= mask.bounds.x + mask.bounds.width || pixel.y >= mask.bounds.y + mask.bounds.height
                || !mask.visibleAt(pixel.x, pixel.y)
                || indices[static_cast<std::size_t>(pixel.y) * indexPitch + pixel.x] != pixel.after
                || pixel.before >= paletteEntries) continue;
            output.restoredIndices[static_cast<std::size_t>(pixel.y) * indexPitch + pixel.x] = pixel.before;
            const unsigned char* color = paletteRgb + pixel.before * 3;
            unsigned char* rgbaPixel = output.restoredRgba.data()
                + static_cast<std::size_t>(pixel.y) * rgbaPitch + pixel.x * 4;
            rgbaPixel[0] = color[0];
            rgbaPixel[1] = color[1];
            rgbaPixel[2] = color[2];
            rgbaPixel[3] = 255;
            ++output.restoredPixelCount;
        }
    }
    output.active = true;
    return true;
}

const PreparedSharpTextFrame& preparedSharpTextFrame()
{
    return preparedFrameStorage();
}

void drawSceneOverlayWithSharpText(SDL_Surface* indexedSurface, SDL_Surface* rgbaSurface, bool ready)
{
    if (indexedSurface == nullptr || rgbaSurface == nullptr) return;
    if (ready) {
        const PreparedSharpTextFrame& frame = preparedSharpTextFrame();
        if (frame.cached) {
            // 219: byte-for-byte the picture the cached layers were built
            // from - redraw both textures, nothing is rebuilt or uploaded.
            drawSceneOverlayCached();
        } else {
            std::vector<unsigned char> overlay = frame.restoredRgba;
            scene::maskOverlay(frame.restoredIndices.data(), frame.indexPitch,
                overlay.data(), frame.rgbaPitch, frame.width, frame.height);
            drawSceneOverlay(overlay.data(), frame.width, frame.height, frame.rgbaPitch);
        }
        if (drawPreparedSharpTextOverlay()) return;

        disableSharpText("OpenGL draw failed");
        const bool glow = beginSceneCapture();
        drawStaticHeroPreview();
        if (glow) compositeGlow();
    }

    SDL_BlitSurface(indexedSurface, nullptr, rgbaSurface, nullptr);
    scene::maskOverlay(static_cast<const unsigned char*>(indexedSurface->pixels), indexedSurface->pitch,
        static_cast<unsigned char*>(rgbaSurface->pixels), rgbaSurface->pitch,
        indexedSurface->w, indexedSurface->h);
    drawSceneOverlay(static_cast<const unsigned char*>(rgbaSurface->pixels),
        rgbaSurface->w, rgbaSurface->h, rgbaSurface->pitch);
}

void presentFrameWithSharpText(SDL_Surface* indexedSurface, SDL_Surface* rgbaSurface, bool ready)
{
    if (indexedSurface == nullptr || rgbaSurface == nullptr) return;
    // 219: the full-frame present replaces the quad texture the 3D cache hit
    // redraws; the next 3D frame rebuilds its layers.
    gSharpStaticKeyValid = false;
    gSharpGlyphKeyValid = false;
    {
        static int reports = 0;
        if (getenv("F2CE_PERF") != nullptr && ++reports % 120 == 0) {
            printf("perf sharp 2d present ran view=%d shouldPresent=%d\n",
                static_cast<int>(scene::activeView()), scene::shouldPresent() ? 1 : 0);
            fflush(stdout);
        }
    }
    if (ready) {
        const PreparedSharpTextFrame& frame = preparedSharpTextFrame();
        presentFrame(frame.restoredRgba.data(), frame.width, frame.height, frame.rgbaPitch);
        if (drawPreparedSharpTextOverlay()) return;
        disableSharpText("OpenGL draw failed");
    }

    SDL_BlitSurface(indexedSurface, nullptr, rgbaSurface, nullptr);
    presentFrame(static_cast<const unsigned char*>(rgbaSurface->pixels),
        rgbaSurface->w, rgbaSurface->h, rgbaSurface->pitch);
}

} // namespace scene_renderer
} // namespace fallout
