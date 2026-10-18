#include "file_name.hpp"

#include "../common/utf8.hpp"

namespace {
constexpr unsigned char FIRST_PRINTABLE_CHAR = 0x20;
constexpr unsigned char DELETE_CHAR = 0x7F;
constexpr char REPLACEMENT_CHAR = '_';

bool isControlChar(unsigned char c) { return c < FIRST_PRINTABLE_CHAR || c == DELETE_CHAR; }
}

std::string sanitizeFileName(std::string name) {
    std::size_t pos = name.find_last_of('/');
    if (pos != std::string::npos) name.erase(0, pos + 1);

    for (char& ch : name) {
        if (isControlChar(static_cast<unsigned char>(ch))) ch = REPLACEMENT_CHAR;
    }
    if (name.empty() || name == "." || name == "..") name = "upload";
    return name;
}

std::string truncateUtf8(const std::string& s, std::size_t maxBytes) {
    if (s.size() <= maxBytes) return s;
    std::size_t cut = maxBytes;
    while (cut > 0 && utf8::isContinuationByte(static_cast<unsigned char>(s[cut]))) --cut;
    return s.substr(0, cut);
}
