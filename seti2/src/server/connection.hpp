#pragma once

#include "speed_meter.hpp"

#include <cstddef>
#include <cstdint>
#include <string>

class Connection {
public:
    Connection(int fd, std::string peer);

    void setFileName(const std::string& name) { label_ += " | " + name; }
    const std::string& label() const { return label_; }

    std::size_t readSome(void* buf, std::size_t len);

    void readExact(void* buf, std::size_t len);

    void finalReport();

    bool sendStatus(std::uint8_t status) noexcept;

private:
    void report(SpeedMeter::Clock::time_point now);

    int fd_;
    std::string label_;
    SpeedMeter meter_;
    SpeedMeter::Clock::time_point lastActivity_;
};
