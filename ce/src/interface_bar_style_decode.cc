#include "interface_bar_style_decode.h"

#include <cstdint>

namespace fallout {

namespace {

constexpr size_t kFrmHeaderSize = 62;
constexpr size_t kFrmFrameHeaderSize = 12;
constexpr int kInterfaceBarHeight = 99;

uint16_t readBigEndian16(const unsigned char* bytes)
{
    return static_cast<uint16_t>((static_cast<uint16_t>(bytes[0]) << 8) | bytes[1]);
}

uint32_t readBigEndian32(const unsigned char* bytes)
{
    return (static_cast<uint32_t>(bytes[0]) << 24)
        | (static_cast<uint32_t>(bytes[1]) << 16)
        | (static_cast<uint32_t>(bytes[2]) << 8)
        | static_cast<uint32_t>(bytes[3]);
}

} // namespace

bool interfaceBarStyleDecodeFrm(const unsigned char* bytes, size_t size, int expectedWidth, std::vector<unsigned char>& pixels)
{
    if (bytes == nullptr || expectedWidth < 640 || expectedWidth > 4320 || size < kFrmHeaderSize) return false;

    const int frameCount = static_cast<int>(readBigEndian16(bytes + 8));
    const uint32_t dataOffset = readBigEndian32(bytes + 34);
    if (frameCount < 1 || dataOffset > size - kFrmHeaderSize) return false;

    const size_t frameOffset = kFrmHeaderSize + dataOffset;
    if (frameOffset > size || size - frameOffset < kFrmFrameHeaderSize) return false;
    const unsigned char* frame = bytes + frameOffset;
    const int width = static_cast<int>(readBigEndian16(frame));
    const int height = static_cast<int>(readBigEndian16(frame + 2));
    const uint32_t dataSize = readBigEndian32(frame + 4);
    const size_t pixelCount = static_cast<size_t>(width) * static_cast<size_t>(height);
    if (width != expectedWidth || height != kInterfaceBarHeight
        || dataSize != pixelCount || pixelCount > size - frameOffset - kFrmFrameHeaderSize) {
        return false;
    }

    pixels.assign(frame + kFrmFrameHeaderSize, frame + kFrmFrameHeaderSize + pixelCount);
    return true;
}

int interfaceBarStyleResolveSelection(bool originalAvailable, bool rpAvailable, int selectedStyle)
{
    if (selectedStyle == 1 && rpAvailable) return 1;
    if (originalAvailable) return 0;
    if (rpAvailable) return 1;
    return 0;
}

} // namespace fallout
