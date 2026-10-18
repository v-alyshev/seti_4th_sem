#include "protocol.hpp"

#include "utf8.hpp"

#include <cstring>

namespace proto {
namespace {
void putU16(unsigned char* p, std::uint16_t v) {
    p[0] = static_cast<unsigned char>(v >> 8);
    p[1] = static_cast<unsigned char>(v);
}

std::uint16_t getU16(const unsigned char* p) {
    return static_cast<std::uint16_t>((p[0] << 8) | p[1]);
}

void putU64(unsigned char* p, std::uint64_t v) {
    for (int i = 7; i >= 0; --i) {
        p[i] = static_cast<unsigned char>(v);
        v >>= 8;
    }
}

std::uint64_t getU64(const unsigned char* p) {
    std::uint64_t v = 0;
    for (int i = 0; i < 8; ++i) v = (v << 8) | p[i];
    return v;
}
}

bool isValidUtf8(const std::string& s) {
    std::size_t i = 0, n = s.size();
    while (i < n) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c < utf8::ASCII_LIMIT) {
            ++i;
            continue;
        }

        std::size_t len;
        std::uint32_t cp;
        std::uint32_t minCp;
        if ((c & utf8::TWO_BYTE_PREFIX_MASK) == utf8::TWO_BYTE_PREFIX) {
            len = 2;
            cp = c & utf8::TWO_BYTE_DATA_MASK;
            minCp = utf8::TWO_BYTE_MIN;
        } else if ((c & utf8::THREE_BYTE_PREFIX_MASK) == utf8::THREE_BYTE_PREFIX) {
            len = 3;
            cp = c & utf8::THREE_BYTE_DATA_MASK;
            minCp = utf8::THREE_BYTE_MIN;
        } else if ((c & utf8::FOUR_BYTE_PREFIX_MASK) == utf8::FOUR_BYTE_PREFIX) {
            len = 4;
            cp = c & utf8::FOUR_BYTE_DATA_MASK;
            minCp = utf8::FOUR_BYTE_MIN;
        } else {
            return false;
        }
        if (i + len > n) return false;

        for (std::size_t k = 1; k < len; ++k) {
            unsigned char cc = static_cast<unsigned char>(s[i + k]);
            if (!utf8::isContinuationByte(cc)) return false;
            cp = (cp << utf8::CONTINUATION_DATA_BITS) | (cc & utf8::CONTINUATION_DATA_MASK);
        }

        bool overlong = cp < minCp;
        bool beyondUnicode = cp > utf8::UNICODE_MAX;
        bool utf16Reserved = cp >= utf8::UTF16_RESERVED_FIRST && cp <= utf8::UTF16_RESERVED_LAST;
        if (overlong || beyondUnicode || utf16Reserved) return false;
        i += len;
    }
    return true;
}

void validateName(const std::string& name) {
    if (name.empty() || name.size() > MAX_NAME_LEN)
        throw ProtocolError("длина имени файла должна быть от 1 до " + std::to_string(MAX_NAME_LEN) +
                            " байт, получено " + std::to_string(name.size()));
    if (!isValidUtf8(name)) throw ProtocolError("имя файла не является корректной строкой UTF-8");
}

void validateSize(std::uint64_t size) {
    if (size > MAX_FILE_SIZE)
        throw ProtocolError("размер файла " + std::to_string(size) + " байт превышает 1 ТБ");
}

std::vector<unsigned char> encodeHeader(const Header& header) {
    validateName(header.name);
    validateSize(header.size);

    std::vector<unsigned char> out(PREFIX_SIZE + header.name.size() + SIZE_FIELD_SIZE);
    unsigned char* p = out.data();
    std::memcpy(p, MAGIC, sizeof MAGIC);
    putU16(p + sizeof MAGIC, static_cast<std::uint16_t>(header.name.size()));
    p += PREFIX_SIZE;
    std::memcpy(p, header.name.data(), header.name.size());
    putU64(p + header.name.size(), header.size);
    return out;
}

std::size_t decodePrefix(const unsigned char (&prefix)[PREFIX_SIZE]) {
    if (std::memcmp(prefix, MAGIC, sizeof MAGIC) != 0)
        throw ProtocolError("неверная сигнатура протокола");
    std::size_t nameLen = getU16(prefix + sizeof MAGIC);
    if (nameLen == 0 || nameLen > MAX_NAME_LEN)
        throw ProtocolError("неверная длина имени файла: " + std::to_string(nameLen));
    return nameLen;
}

std::uint64_t decodeSize(const unsigned char (&field)[SIZE_FIELD_SIZE]) {
    std::uint64_t size = getU64(field);
    validateSize(size);
    return size;
}
}
