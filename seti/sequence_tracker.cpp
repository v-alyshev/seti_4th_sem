#include "sequence_tracker.h"

#include <algorithm>
#include <cstddef>
#include <iterator>

namespace lan_copies {

    namespace {
        constexpr uint64_t kMaxMissing = 100000;
    }

    uint64_t SequenceTracker::expected() const {
        return started_ ? highest_ - first_ + 1 : 0;
    }

    void SequenceTracker::markMissing(uint64_t from, uint64_t to) {
        if (to - from > kMaxMissing) from = to - kMaxMissing;
        for (uint64_t seq = from; seq < to; ++seq) missing_.insert(seq);
        if (missing_.size() > kMaxMissing) {
            const auto excess = static_cast<std::ptrdiff_t>(missing_.size() - kMaxMissing);
            missing_.erase(missing_.begin(), std::next(missing_.begin(), excess));
        }
    }

    SeqEvent SequenceTracker::accept(uint64_t seq, uint64_t expectedFirst) {
        lastGap_ = 0;

        if (!started_) {
            started_ = true;
            first_ = std::min(expectedFirst, seq);
            highest_ = seq;
            received_ = 1;
            markMissing(first_, seq);
            lastGap_ = seq - first_;
            return lastGap_ > 0 ? SeqEvent::Gap : SeqEvent::First;
        }

        if (seq > highest_) {
            markMissing(highest_ + 1, seq);
            lastGap_ = seq - highest_ - 1;
            highest_ = seq;
            ++received_;
            return lastGap_ > 0 ? SeqEvent::Gap : SeqEvent::InOrder;
        }

        if (seq < first_) return SeqEvent::BeforeStart;

        if (missing_.erase(seq) > 0) {
            ++received_;
            ++late_;
            return SeqEvent::Late;
        }

        ++duplicates_;
        return SeqEvent::Duplicate;
    }

    uint64_t SequenceTracker::finish(uint64_t nextSeq) {
        if (!started_ || nextSeq == 0 || nextSeq - 1 <= highest_) return 0;
        const uint64_t tail = nextSeq - 1 - highest_;
        markMissing(highest_ + 1, nextSeq);
        highest_ = nextSeq - 1;
        return tail;
    }

}