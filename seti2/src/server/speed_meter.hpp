#pragma once

#include <chrono>
#include <cstdint>

class SpeedMeter {
public:
    using Clock = std::chrono::steady_clock;

    struct Speeds {
        double instant;
        double average;
    };

    SpeedMeter() : start_(Clock::now()), lastReport_(start_) {}

    void add(std::uint64_t bytes) { total_ += bytes; }

    Clock::time_point lastReport() const { return lastReport_; }

    bool hasUnreported() const { return !reported_ || total_ != lastTotal_; }

    Speeds takeReport(Clock::time_point now);

private:
    Clock::time_point start_;
    Clock::time_point lastReport_;
    std::uint64_t total_ = 0;
    std::uint64_t lastTotal_ = 0;
    bool reported_ = false;
};
