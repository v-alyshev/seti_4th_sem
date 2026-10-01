#include "peer_table.h"

#include <algorithm>
#include <iostream>
#include <utility>
#include <vector>

#include "protocol.h"

namespace lan_copies {

    namespace {
        const std::string kListIndent = "    ";
    }

    bool PeerTable::onHello(const std::string& id, const std::string& ip, Clock::time_point now) {
        auto [it, inserted] = peers_.try_emplace(id, Peer{ip, now});
        if (inserted) return true;
        bool changed = (it->second.ip != ip);
        it->second.ip = ip;
        it->second.lastSeen = now;
        return changed;
    }

    bool PeerTable::onBye(const std::string& id) { return peers_.erase(id) > 0; }

    bool PeerTable::expire(Clock::time_point now) {
        bool changed = false;
        for (auto it = peers_.begin(); it != peers_.end();) {
            if (now - it->second.lastSeen > kPeerTimeout) {
                it = peers_.erase(it);
                changed = true;
            } else {
                ++it;
            }
        }
        return changed;
    }

    void PeerTable::print() const {
        std::vector<std::pair<std::string, std::string>> entries;
        for (const auto& [id, peer] : peers_) entries.emplace_back(peer.ip, id);
        std::sort(entries.begin(), entries.end());

        std::cout << "[" << timeStamp() << "] Живых копий: " << entries.size() << "\n";
        for (const auto& [ip, id] : entries)
            std::cout << kListIndent << ip << "  (id " << id << ")\n";
        std::cout << std::flush;
    }

}