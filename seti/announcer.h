#pragma once

#include <cstdint>
#include <string>

#include "common.h"
#include "multicast_socket.h"

namespace lan_copies {

class Announcer {
public:
    Announcer(MulticastSocket& sock, const std::string& myId);

    Clock::time_point tick(Clock::time_point now);
    void sayBye();

private:
    MulticastSocket& sock_;
    const std::string myId_;
    uint64_t seq_ = 0;
    Clock::time_point nextSend_{};
    int failures_ = 0;

    void sendHello();
};

}
