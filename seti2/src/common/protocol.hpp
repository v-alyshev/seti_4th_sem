#pragma once

#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

namespace proto {
inline constexpr char MAGIC[4] = {'P', 'R', 'O', 'T'};
inline constexpr std::size_t MAX_NAME_LEN = 4096;
inline constexpr std::uint64_t MAX_FILE_SIZE = 1ULL << 40;

inline constexpr std::size_t PREFIX_SIZE = sizeof MAGIC + 2;
inline constexpr std::size_t SIZE_FIELD_SIZE = 8;

inline constexpr std::uint8_t STATUS_OK = 0;
inline constexpr std::uint8_t STATUS_FAIL = 1;

class ProtocolError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

struct Header {
    std::string name;
    std::uint64_t size = 0;
};

bool isValidUtf8(const std::string& s);

void validateName(const std::string& name);

void validateSize(std::uint64_t size);

std::vector<unsigned char> encodeHeader(const Header& header);

std::size_t decodePrefix(const unsigned char (&prefix)[PREFIX_SIZE]);
std::uint64_t decodeSize(const unsigned char (&field)[SIZE_FIELD_SIZE]);
}
