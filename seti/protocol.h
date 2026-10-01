#pragma once

#include <chrono>
#include <optional>
#include <string>

namespace lan_copies {

    constexpr auto kSendInterval = std::chrono::seconds(1);
    constexpr auto kPeerTimeout = std::chrono::seconds(5);

    enum class MessageType { Hello, Bye };

    struct Message {
        MessageType type;
        std::string id;
    };

    std::string randomId();
    std::string buildMessage(MessageType type, const std::string& id);
    std::optional<Message> parseMessage(const std::string& data);

}