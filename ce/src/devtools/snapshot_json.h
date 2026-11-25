#pragma once

#include <cstdio>
#include <string>

namespace fallout::snapshot_json {

inline void jsonEscape(const char* in, std::string& out)
{
    for (const char* p = in; *p != '\0'; p++) {
        const unsigned char ch = static_cast<unsigned char>(*p);
        if (ch == '"' || ch == '\\') {
            out += '\\';
            out += static_cast<char>(ch);
        } else if (ch < 0x20) {
            char piece[8];
            snprintf(piece, sizeof(piece), "\\u%04x", ch);
            out += piece;
        } else {
            out += static_cast<char>(ch);
        }
    }
}

// Game text is CP1251; byte 0x98 is undefined and becomes U+FFFD.
inline void cp1251ToUtf8(const char* in, std::string& out)
{
    static constexpr unsigned int upper[] = {
        0x0402, 0x0403, 0x201A, 0x0453, 0x201E, 0x2026, 0x2020, 0x2021,
        0x20AC, 0x2030, 0x0409, 0x2039, 0x040A, 0x040C, 0x040B, 0x040F,
        0x0452, 0x2018, 0x2019, 0x201C, 0x201D, 0x2022, 0x2013, 0x2014,
        0xFFFD, 0x2122, 0x0459, 0x203A, 0x045A, 0x045C, 0x045B, 0x045F,
        0x00A0, 0x040E, 0x045E, 0x0408, 0x00A4, 0x0490, 0x00A6, 0x00A7,
        0x0401, 0x00A9, 0x0404, 0x00AB, 0x00AC, 0x00AD, 0x00AE, 0x0407,
        0x00B0, 0x00B1, 0x0406, 0x0456, 0x0491, 0x00B5, 0x00B6, 0x00B7,
        0x0451, 0x2116, 0x0454, 0x00BB, 0x0458, 0x0405, 0x0455, 0x0457
    };
    for (const unsigned char* p = reinterpret_cast<const unsigned char*>(in); *p != 0; p++) {
        unsigned int code = *p;
        if (code >= 0xC0) code = 0x410 + (code - 0xC0);
        else if (code >= 0x80) code = upper[code - 0x80];
        if (code < 0x80) {
            out += static_cast<char>(code);
        } else if (code < 0x800) {
            out += static_cast<char>(0xC0 | (code >> 6));
            out += static_cast<char>(0x80 | (code & 0x3F));
        } else {
            out += static_cast<char>(0xE0 | (code >> 12));
            out += static_cast<char>(0x80 | ((code >> 6) & 0x3F));
            out += static_cast<char>(0x80 | (code & 0x3F));
        }
    }
}

inline std::string gameText(const char* text)
{
    std::string utf8;
    std::string escaped;
    cp1251ToUtf8(text, utf8);
    jsonEscape(utf8.c_str(), escaped);
    return escaped;
}

} // namespace fallout::snapshot_json
