#pragma once

#include <map>
#include <string>

#include "common.h"

namespace lan_copies {

    class PeerTable {
    public:
        bool onHello(const std::string& id, const std::string& ip, Clock::time_point now);
        bool onBye(const std::string& id);
        bool expire(Clock::time_point now);
        void print() const;

    private:
        struct Peer {
            std::string ip;
            Clock::time_point lastSeen;
        };
        std::map<std::string, Peer> peers_;
    };

}