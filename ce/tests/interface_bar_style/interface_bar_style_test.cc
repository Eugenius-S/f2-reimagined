#include "interface_bar_style_decode.h"

#include <cassert>
#include <cstdint>
#include <vector>

namespace {

void writeBigEndian16(std::vector<unsigned char>& bytes, size_t offset, uint16_t value)
{
    bytes[offset] = static_cast<unsigned char>(value >> 8);
    bytes[offset + 1] = static_cast<unsigned char>(value);
}

void writeBigEndian32(std::vector<unsigned char>& bytes, size_t offset, uint32_t value)
{
    bytes[offset] = static_cast<unsigned char>(value >> 24);
    bytes[offset + 1] = static_cast<unsigned char>(value >> 16);
    bytes[offset + 2] = static_cast<unsigned char>(value >> 8);
    bytes[offset + 3] = static_cast<unsigned char>(value);
}

std::vector<unsigned char> makeBarFrm(int width)
{
    const size_t pixels = static_cast<size_t>(width) * 99;
    std::vector<unsigned char> bytes(62 + 12 + pixels, 0x2A);
    writeBigEndian16(bytes, 8, 1);
    writeBigEndian32(bytes, 34, 0);
    writeBigEndian16(bytes, 62, static_cast<uint16_t>(width));
    writeBigEndian16(bytes, 64, 99);
    writeBigEndian32(bytes, 66, static_cast<uint32_t>(pixels));
    return bytes;
}

} // namespace

int main()
{
    using fallout::interfaceBarStyleDecodeFrm;
    using fallout::interfaceBarStyleResolveSelection;
    std::vector<unsigned char> pixels;

    for (int width : { 640, 800, 1024, 4320 }) {
        const std::vector<unsigned char> frm = makeBarFrm(width);
        assert(interfaceBarStyleDecodeFrm(frm.data(), frm.size(), width, pixels));
        assert(pixels.size() == static_cast<size_t>(width) * 99);
    }

    const std::vector<unsigned char> wideFrm = makeBarFrm(800);
    assert(!interfaceBarStyleDecodeFrm(wideFrm.data(), wideFrm.size(), 640, pixels));
    assert(!interfaceBarStyleDecodeFrm(wideFrm.data(), wideFrm.size(), 1024, pixels));
    assert(!interfaceBarStyleDecodeFrm(wideFrm.data(), wideFrm.size(), 639, pixels));
    assert(!interfaceBarStyleDecodeFrm(wideFrm.data(), wideFrm.size(), 4321, pixels));
    assert(!interfaceBarStyleDecodeFrm(wideFrm.data(), wideFrm.size() - 1, 800, pixels));
    assert(!interfaceBarStyleDecodeFrm(nullptr, 0, 800, pixels));

    assert(interfaceBarStyleResolveSelection(true, true, 0) == 0);
    assert(interfaceBarStyleResolveSelection(true, true, 1) == 1);
    assert(interfaceBarStyleResolveSelection(true, false, 1) == 0);
    assert(interfaceBarStyleResolveSelection(false, true, 0) == 1);
    assert(interfaceBarStyleResolveSelection(false, false, 1) == 0);

    std::vector<unsigned char> wrongHeight = wideFrm;
    writeBigEndian16(wrongHeight, 64, 98);
    assert(!interfaceBarStyleDecodeFrm(wrongHeight.data(), wrongHeight.size(), 800, pixels));
}
