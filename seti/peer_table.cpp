#include "peer_table.h"

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <utility>
#include <vector>

#include "protocol.h"

namespace lan_copies {

namespace {
const std::string kListIndent = "    ";
constexpr double kPercent = 100.0;
constexpr int kPercentPrecision = 1;
}

PeerTable::PeerTable() : startTime_(Clock::now()) {}

uint64_t PeerTable::expectedFirst(uint64_t seq, Clock::time_point now) const {
    const auto intervalsListened = static_cast<uint64_t>((now - startTime_) / kSendInterval);
    return seq <= intervalsListened ? 0 : seq;
}

bool PeerTable::onHello(const std::string& id, const std::string& ip, uint64_t seq,
                        Clock::time_point now) {
    auto [it, inserted] = peers_.try_emplace(id, Peer{ip, now, SequenceTracker{}});
    Peer& peer = it->second;

    bool changed = inserted || peer.ip != ip;
    peer.ip = ip;
    peer.lastSeen = now;

    const SeqEvent event = peer.seq.accept(seq, expectedFirst(seq, now));
    reportSeqEvent(id, peer, event, seq);
    return changed;
}

bool PeerTable::onBye(const std::string& id, uint64_t seq) {
    auto it = peers_.find(id);
    if (it == peers_.end()) return false;

    const uint64_t tail = it->second.seq.finish(seq);
    std::cout << "[" << timeStamp() << "] Копия " << describe(it->first, it->second)
              << " завершила работу";
    if (tail > 0) std::cout << " (не дошло последних сообщений: " << tail << ")";
    std::cout << ". " << formatStats(it->second.seq) << "\n";
    peers_.erase(it);
    return true;
}

bool PeerTable::expire(Clock::time_point now) {
    bool changed = false;
    for (auto it = peers_.begin(); it != peers_.end();) {
        if (now - it->second.lastSeen > kPeerTimeout) {
            std::cout << "[" << timeStamp() << "] Копия " << describe(it->first, it->second)
                      << " пропала по таймауту. " << formatStats(it->second.seq) << "\n";
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

void PeerTable::printStatistics(const std::string& title) const {
    std::cout << "[" << timeStamp() << "] " << title << ":\n";
    if (peers_.empty()) std::cout << kListIndent << "нет копий\n";
    for (const auto& [id, peer] : peers_)
        std::cout << kListIndent << describe(id, peer) << ": " << formatStats(peer.seq) << "\n";
    std::cout << std::flush;
}

std::string PeerTable::describe(const std::string& id, const Peer& peer) {
    return peer.ip + " (id " + id + ")";
}

std::string PeerTable::formatStats(const SequenceTracker& seq) {
    const double lossPercent =
        seq.expected() == 0 ? 0.0 : kPercent * static_cast<double>(seq.lost()) / seq.expected();

    std::ostringstream oss;
    oss << "Получено " << seq.received() << " из " << seq.expected() << ", потеряно "
        << seq.lost() << " (" << std::fixed << std::setprecision(kPercentPrecision) << lossPercent
        << "%), с опозданием " << seq.late() << ", повторов " << seq.duplicates() << ".";
    return oss.str();
}

void PeerTable::reportSeqEvent(const std::string& id, const Peer& peer, SeqEvent event,
                               uint64_t seq) {
    std::ostringstream oss;
    switch (event) {
        case SeqEvent::First:
        case SeqEvent::InOrder:
            return;
        case SeqEvent::Gap:
            oss << "пропущено сообщений: " << peer.seq.lastGap() << " (пришло #" << seq << ")";
            break;
        case SeqEvent::Late:
            oss << "сообщение #" << seq << " пришло с опозданием, потеря отменена";
            break;
        case SeqEvent::Duplicate:
            oss << "повтор сообщения #" << seq << ", пропущен";
            break;
        case SeqEvent::BeforeStart:
            oss << "сообщение #" << seq << " отправлено до начала наблюдения, пропущено";
            break;
    }
    std::cout << "[" << timeStamp() << "] " << describe(id, peer) << ": " << oss.str() << ". "
              << formatStats(peer.seq) << "\n" << std::flush;
}

}
