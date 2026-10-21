#pragma once

#include <cstdint>
#include <set>

namespace lan_copies {

enum class SeqEvent { First, InOrder, Gap, Late, Duplicate, BeforeStart };

class SequenceTracker {
public:
    SeqEvent accept(uint64_t seq, uint64_t expectedFirst);
    uint64_t finish(uint64_t nextSeq);

    uint64_t lastGap() const { return lastGap_; }
    uint64_t expected() const;
    uint64_t received() const { return received_; }
    uint64_t lost() const { return expected() - received_; }
    uint64_t late() const { return late_; }
    uint64_t duplicates() const { return duplicates_; }

private:
    bool started_ = false;
    uint64_t first_ = 0;
    uint64_t highest_ = 0;
    uint64_t received_ = 0;
    uint64_t late_ = 0;
    uint64_t duplicates_ = 0;
    uint64_t lastGap_ = 0;
    std::set<uint64_t> missing_;

    void markMissing(uint64_t from, uint64_t to);
};

}
