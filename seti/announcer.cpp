#include "announcer.h"

#include "protocol.h"

namespace lan_copies {

    Announcer::Announcer(MulticastSocket& sock, const std::string& myId)
        : sock_(sock),
          hello_(buildMessage(MessageType::Hello, myId)),
          bye_(buildMessage(MessageType::Bye, myId)) {}

    Clock::time_point Announcer::tick(Clock::time_point now) {
        if (now >= nextSend_) {
            sock_.send(hello_);
            nextSend_ = now + kSendInterval;
        }
        return nextSend_;
    }

    void Announcer::sayBye() { sock_.send(bye_); }

}