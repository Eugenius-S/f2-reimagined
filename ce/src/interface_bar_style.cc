#include "interface_bar_style.h"
#include "interface_bar_style_decode.h"

#include "dfile.h"
#include "draw.h"
#include "settings.h"

#include <array>
#include <cstdio>
#include <cstring>
#include <map>
#include <vector>

namespace fallout {

namespace {

constexpr int kInterfaceBarWidth = 640;
constexpr int kInterfaceBarHeight = 99;
constexpr char kInterfaceFrmPath[] = "art\\intrface\\iface.frm";

struct CachedImage {
    std::vector<unsigned char> pixels;
    bool attempted = false;
};

std::map<int, std::array<CachedImage, 2>> gImages;

bool readInterfaceBar(const char* archivePath, const char* frmPath, int expectedWidth, std::vector<unsigned char>& pixels)
{
    DBase* archive = dbaseOpen(archivePath);
    if (archive == nullptr) return false;

    DFile* file = dfileOpen(archive, frmPath, "rb");
    if (file == nullptr) {
        dbaseClose(archive);
        return false;
    }

    const long fileSize = dfileGetSize(file);
    std::vector<unsigned char> bytes(fileSize > 0 ? static_cast<size_t>(fileSize) : 0);
    const bool read = bytes.size() >= 62
        && dfileRead(bytes.data(), 1, bytes.size(), file) == bytes.size();
    dfileClose(file);
    dbaseClose(archive);
    return read && interfaceBarStyleDecodeFrm(bytes.data(), bytes.size(), expectedWidth, pixels);
}

CachedImage* imageForStyle(bool rp, int barWidth)
{
    if (barWidth < kInterfaceBarWidth || barWidth > 4320) return nullptr;
    CachedImage& image = gImages[barWidth][rp ? 1 : 0];
    if (!image.attempted) {
        image.attempted = true;
        char wideFrmPath[64];
        const char* frmPath = kInterfaceFrmPath;
        if (barWidth != kInterfaceBarWidth) {
            snprintf(wideFrmPath, sizeof(wideFrmPath), "art\\intrface\\HR_IFACE_%d.FRM", barWidth);
            frmPath = wideFrmPath;
        }
        bool loaded = false;
        if (rp) {
            loaded = readInterfaceBar("mods/rpu_russian.dat", frmPath, barWidth, image.pixels);
        } else if (barWidth != kInterfaceBarWidth) {
            const char* archiveCandidates[] = { "mods/f2_res.dat", settings.system.f2_res_dat_path.c_str(), "f2_res.dat" };
            for (const char* archivePath : archiveCandidates) {
                if (archivePath[0] != '\0' && readInterfaceBar(archivePath, frmPath, barWidth, image.pixels)) {
                    loaded = true;
                    break;
                }
            }
        } else {
            loaded = readInterfaceBar("master.dat", frmPath, barWidth, image.pixels);
        }
        if (!loaded) {
            image.pixels.clear();
        }
    }
    return &image;
}

} // namespace

bool interfaceBarStyleRpAvailable(int barWidth)
{
    CachedImage* image = imageForStyle(true, barWidth);
    return image != nullptr && !image->pixels.empty();
}

void interfaceBarStyleNormalizeSelection(int barWidth)
{
    const bool originalAvailable = interfaceBarStyleImage(false, barWidth, nullptr, nullptr) != nullptr;
    const bool rpAvailable = interfaceBarStyleRpAvailable(barWidth);
    settings.ui.hud_bar_style = interfaceBarStyleResolveSelection(originalAvailable, rpAvailable, settings.ui.hud_bar_style);
}

const unsigned char* interfaceBarStyleImage(bool rp, int barWidth, int* width, int* height)
{
    CachedImage* image = imageForStyle(rp, barWidth);
    if (image == nullptr || image->pixels.empty()) return nullptr;
    if (width != nullptr) *width = barWidth;
    if (height != nullptr) *height = kInterfaceBarHeight;
    return image->pixels.data();
}

const unsigned char* interfaceBarStyleCurrentImage(int barWidth, int* width, int* height)
{
    const unsigned char* image = interfaceBarStyleImage(settings.ui.hud_bar_style == 1, barWidth, width, height);
    if (image == nullptr) image = interfaceBarStyleImage(false, barWidth, width, height);
    return image;
}

bool interfaceBarStyleCopyBackground(unsigned char* destination, int barWidth, bool preserveEndButtons)
{
    int imageWidth = 0;
    int imageHeight = 0;
    const unsigned char* image = interfaceBarStyleCurrentImage(barWidth, &imageWidth, &imageHeight);
    if (image == nullptr || imageWidth != barWidth || imageHeight != 99 || destination == nullptr) return false;

    constexpr int kEndButtonsWidth = 57;
    constexpr int kEndButtonsHeight = 58;
    constexpr int kEndButtonsX = 580;
    constexpr int kEndButtonsY = 38;
    std::array<unsigned char, kEndButtonsWidth * kEndButtonsHeight> endButtons{};
    const int endButtonsX = kEndButtonsX + barWidth - 640;
    if (preserveEndButtons) {
        for (int y = 0; y < kEndButtonsHeight; y++) {
            memcpy(endButtons.data() + y * kEndButtonsWidth,
                destination + (kEndButtonsY + y) * barWidth + endButtonsX, kEndButtonsWidth);
        }
    }
    blitBufferToBuffer(image, imageWidth, imageHeight - 1, imageWidth, destination, barWidth);
    if (preserveEndButtons) {
        for (int y = 0; y < kEndButtonsHeight; y++) {
            memcpy(destination + (kEndButtonsY + y) * barWidth + endButtonsX,
                endButtons.data() + y * kEndButtonsWidth, kEndButtonsWidth);
        }
    }
    return true;
}

} // namespace fallout
