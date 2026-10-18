#include "speed_meter.hpp"

#include <algorithm>

SpeedMeter::Speeds SpeedMeter::takeReport(Clock::time_point now) {
    using Seconds = std::chrono::duration<double>;
    double dt = std::max(Seconds(now - lastReport_).count(), 1e-6);
    double total = std::max(Seconds(now - start_).count(), 1e-6);

    Speeds s{static_cast<double>(total_ - lastTotal_) / dt, static_cast<double>(total_) / total};

    lastReport_ = now;
    lastTotal_ = total_;
    reported_ = true;
    return s;
}
