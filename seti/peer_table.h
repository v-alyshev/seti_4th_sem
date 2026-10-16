#pragma once

#include <cstdint>
#include <map>
#include <string>

#include "common.h"
#include "sequence_tracker.h"

namespace lan_copies {

class PeerTable {
public:
    bool onHello(const std::string& id, const std::string& ip, uint64_t seq, Clock::time_point now);
    bool onBye(const std::string& id);
    bool expire(Clock::time_point now);
    void print() const;
    void printStatistics(const std::string& title) const;

private:
    struct Peer {
        std::string ip;
        Clock::time_point lastSeen;
        SequenceTracker seq;
    };
    std::map<std::string, Peer> peers_;

    static std::string describe(const std::string& id, const Peer& peer);
    static std::string formatStats(const SequenceTracker& seq);
    static void reportSeqEvent(const std::string& id, const Peer& peer, SeqEvent event, uint64_t seq);
};

}
