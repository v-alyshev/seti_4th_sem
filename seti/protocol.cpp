#include "protocol.h"

#include <unistd.h>

#include <algorithm>
#include <cctype>
#include <charconv>
#include <cstddef>
#include <iomanip>
#include <random>
#include <sstream>

namespace lan_copies {

namespace {
const std::string kMagic = "LAN_COPIES_V2";
const std::string kMsgHello = "HELLO";
const std::string kMsgBye = "BYE";
const char kFieldSeparator = ' ';
constexpr std::size_t kIdHexDigits = 16;

bool isValidId(const std::string& id) {
    return id.size() == kIdHexDigits &&
           std::all_of(id.begin(), id.end(),
                       [](unsigned char c) { return std::isxdigit(c) != 0; });
}

std::optional<uint64_t> parseSeq(const std::string& text) {
    uint64_t value = 0;
    const char* end = text.data() + text.size();
    auto [ptr, ec] = std::from_chars(text.data(), end, value);
    if (ec != std::errc() || ptr != end) return std::nullopt;
    return value;
}
}

std::string randomId() {
    std::random_device rd;
    std::seed_seq seed{rd(), rd(), static_cast<unsigned>(::getpid())};
    std::mt19937_64 gen(seed);
    std::ostringstream oss;
    oss << std::hex << std::setw(kIdHexDigits) << std::setfill('0') << gen();
    return oss.str();
}

std::string buildMessage(MessageType type, const std::string& id, uint64_t seq) {
    const std::string& typeStr = (type == MessageType::Hello) ? kMsgHello : kMsgBye;
    return kMagic + kFieldSeparator + typeStr + kFieldSeparator + id + kFieldSeparator +
           std::to_string(seq);
}

std::optional<Message> parseMessage(const std::string& data) {
    std::istringstream iss(data);
    std::string magic, typeStr, id, seqStr;
    if (!(iss >> magic >> typeStr >> id >> seqStr)) return std::nullopt;
    if (magic != kMagic || !isValidId(id)) return std::nullopt;

    const auto seq = parseSeq(seqStr);
    if (!seq) return std::nullopt;

    MessageType type;
    if (typeStr == kMsgHello)
        type = MessageType::Hello;
    else if (typeStr == kMsgBye)
        type = MessageType::Bye;
    else
        return std::nullopt;

    if (data != buildMessage(type, id, *seq)) return std::nullopt;
    return Message{type, id, *seq};
}

}
