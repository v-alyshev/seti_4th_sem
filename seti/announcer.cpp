#include "announcer.h"

#include <cerrno>
#include <cstring>
#include <iostream>
#include <stdexcept>

#include "protocol.h"

namespace lan_copies {

    namespace {
        constexpr int kMaxSendFailures = kPeerTimeout / kSendInterval;
    }

    Announcer::Announcer(MulticastSocket& sock, const std::string& myId)
        : sock_(sock),
          hello_(buildMessage(MessageType::Hello, myId)),
          bye_(buildMessage(MessageType::Bye, myId)) {}

    Clock::time_point Announcer::tick(Clock::time_point now) {
        if (now >= nextSend_) {
            sendHello();
            nextSend_ = now + kSendInterval;
        }
        return nextSend_;
    }

    void Announcer::sendHello() {
        if (sock_.send(hello_)) {
            if (failures_ > 0)
                std::cerr << "[" << timeStamp() << "] Отправка восстановлена.\n";
            failures_ = 0;
            return;
        }

        const std::string reason = std::strerror(errno);
        ++failures_;
        std::cerr << "[" << timeStamp() << "] Не удалось отправить HELLO (" << failures_ << " раз из "
                  << kMaxSendFailures << "): " << reason << "\n";
        if (failures_ >= kMaxSendFailures)
            throw std::runtime_error("не удалось отправить " + std::to_string(failures_)
                + " раз подряд, отключение: " + reason);
    }

    void Announcer::sayBye() {
        if (!sock_.send(bye_))
            std::cerr << "[" << timeStamp() << "] Не удалось отправить BYE: " << std::strerror(errno) << "\n";
    }

}