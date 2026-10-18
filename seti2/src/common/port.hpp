#pragma once

#include <cstdint>

inline bool parsePort(const char* text, std::uint16_t& port) {
    if (text == nullptr || *text == '\0') return false;
    unsigned long v = 0;
    for (const char* p = text; *p != '\0'; ++p) {
        if (*p < '0' || *p > '9') return false;
        v = v * 10 + static_cast<unsigned long>(*p - '0');
        if (v > 65535) return false;
    }
    if (v == 0) return false;
    port = static_cast<std::uint16_t>(v);
    return true;
}
