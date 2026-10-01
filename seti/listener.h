#pragma once

#include <string>

#include "common.h"
#include "multicast_socket.h"
#include "peer_table.h"

namespace lan_copies {

class Listener {
public:
    Listener(MulticastSocket& sock, PeerTable& peers, const std::string& myId);

    bool handleIncoming(Clock::time_point now);

private:
    MulticastSocket& sock_;
    PeerTable& peers_;
    const std::string myId_;
};

}
