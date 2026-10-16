#include "sequence_tracker.h"

namespace lan_copies {

namespace {
constexpr uint64_t kWindowSize = 64;
}

uint64_t SequenceTracker::expected() const {
    return started_ ? highest_ - first_ + 1 : 0;
}

SeqEvent SequenceTracker::accept(uint64_t seq) {
    lastGap_ = 0;

    if (!started_) {
        started_ = true;
        first_ = highest_ = seq;
        window_ = 1;
        received_ = 1;
        return SeqEvent::First;
    }

    if (seq > highest_) {
        const uint64_t shift = seq - highest_;
        window_ = shift >= kWindowSize ? 1 : (window_ << shift) | 1;
        highest_ = seq;
        ++received_;
        lastGap_ = shift - 1;
        return lastGap_ > 0 ? SeqEvent::Gap : SeqEvent::InOrder;
    }

    const uint64_t offset = highest_ - seq;
    if (seq < first_ || offset >= kWindowSize) {
        ++duplicates_;
        return SeqEvent::Duplicate;
    }

    const uint64_t bit = uint64_t{1} << offset;
    if (window_ & bit) {
        ++duplicates_;
        return SeqEvent::Duplicate;
    }

    window_ |= bit;
    ++received_;
    ++late_;
    return SeqEvent::Late;
}

}
