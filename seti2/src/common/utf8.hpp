#pragma once

#include <cstdint>

namespace utf8 {

inline constexpr unsigned char ASCII_LIMIT = 0b1000'0000;

inline constexpr unsigned char TWO_BYTE_PREFIX_MASK = 0b1110'0000;
inline constexpr unsigned char TWO_BYTE_PREFIX      = 0b1100'0000;
inline constexpr unsigned char TWO_BYTE_DATA_MASK   = 0b0001'1111;

inline constexpr unsigned char THREE_BYTE_PREFIX_MASK = 0b1111'0000;
inline constexpr unsigned char THREE_BYTE_PREFIX      = 0b1110'0000;
inline constexpr unsigned char THREE_BYTE_DATA_MASK   = 0b0000'1111;

inline constexpr unsigned char FOUR_BYTE_PREFIX_MASK = 0b1111'1000;
inline constexpr unsigned char FOUR_BYTE_PREFIX      = 0b1111'0000;
inline constexpr unsigned char FOUR_BYTE_DATA_MASK   = 0b0000'0111;

inline constexpr unsigned char CONTINUATION_PREFIX_MASK = 0b1100'0000;
inline constexpr unsigned char CONTINUATION_PREFIX      = 0b1000'0000;
inline constexpr unsigned char CONTINUATION_DATA_MASK   = 0b0011'1111;
inline constexpr int CONTINUATION_DATA_BITS = 6;

inline constexpr std::uint32_t TWO_BYTE_MIN = 0x80;
inline constexpr std::uint32_t THREE_BYTE_MIN = 0x800;
inline constexpr std::uint32_t FOUR_BYTE_MIN = 0x10000;
inline constexpr std::uint32_t UNICODE_MAX = 0x10FFFF;

inline constexpr std::uint32_t UTF16_RESERVED_FIRST = 0xD800;
inline constexpr std::uint32_t UTF16_RESERVED_LAST = 0xDFFF;

inline bool isContinuationByte(unsigned char c) {
    return (c & CONTINUATION_PREFIX_MASK) == CONTINUATION_PREFIX;
}

}
