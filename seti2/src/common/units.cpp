#include "units.hpp"

#include <cstdio>

namespace {
std::string format(double bytes, const char* suffix) {
    double kb = bytes / 1024.0;
    char buf[64];
    if (kb < 1.0)
        std::snprintf(buf, sizeof buf, "%.3f КБ%s", kb, suffix);
    else if (kb < 1024.0)
        std::snprintf(buf, sizeof buf, "%.2f КБ%s", kb, suffix);
    else
        std::snprintf(buf, sizeof buf, "%.2f МБ%s", kb / 1024.0, suffix);
    return buf;
}
}

std::string formatSize(std::uint64_t bytes) { return format(static_cast<double>(bytes), ""); }

std::string formatSpeed(double bytesPerSec) { return format(bytesPerSec, "/с"); }
