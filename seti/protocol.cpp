#include "protocol.h"

#include <unistd.h>

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <iomanip>
#include <random>
#include <sstream>

namespace lan_copies {

namespace {
const std::string kMagic = "LAN_COPIES_V1";
const std::string kMsgHello = "HELLO";
const std::string kMsgBye = "BYE";
const char kFieldSeparator = ' ';
constexpr std::size_t kIdHexDigits = 16;

bool isValidId(const std::string& id) {
    return id.size() == kIdHexDigits &&
           std::all_of(id.begin(), id.end(),
                       [](unsigned char c) { return std::isxdigit(c) != 0; });
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

std::string buildMessage(MessageType type, const std::string& id) {
    const std::string& typeStr = (type == MessageType::Hello) ? kMsgHello : kMsgBye;
    return kMagic + kFieldSeparator + typeStr + kFieldSeparator + id;
}

std::optional<Message> parseMessage(const std::string& data) {
    std::istringstream iss(data);
    std::string magic, typeStr, id;
    if (!(iss >> magic >> typeStr >> id)) return std::nullopt;
    if (magic != kMagic || !isValidId(id)) return std::nullopt;

    MessageType type;
    if (typeStr == kMsgHello)
        type = MessageType::Hello;
    else if (typeStr == kMsgBye)
        type = MessageType::Bye;
    else
        return std::nullopt;

    if (data != buildMessage(type, id)) return std::nullopt;
    return Message{type, id};
}

}
