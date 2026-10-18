#pragma once

#include <cstdint>
#include <string>

struct SendResult {
    bool ok = false;
    std::string name;
    std::uint64_t size = 0;
    std::string error;
};

class FileSender {
public:
    FileSender(std::string host, std::uint16_t port);

    SendResult send(const std::string& path) const noexcept;

private:
    void transfer(const std::string& path, SendResult& result) const;

    std::string host_;
    std::uint16_t port_;
};
