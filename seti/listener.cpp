#include "listener.h"

#include "protocol.h"

namespace lan_copies {

Listener::Listener(MulticastSocket& sock, PeerTable& peers, const std::string& myId)
    : sock_(sock), peers_(peers), myId_(myId) {}

bool Listener::handleIncoming(Clock::time_point now) {
    auto datagram = sock_.receive();
    if (!datagram) return false;

    auto msg = parseMessage(datagram->data);
    if (!msg || msg->id == myId_) return false;

    if (msg->type == MessageType::Hello)
        return peers_.onHello(msg->id, datagram->fromIp, now);
    return peers_.onBye(msg->id);
}

}
