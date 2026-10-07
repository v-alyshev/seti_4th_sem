#pragma once

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
        const std::string hello_;
        const std::string bye_;
        Clock::time_point nextSend_{};
        int failures_ = 0;

        void sendHello();
    };

}