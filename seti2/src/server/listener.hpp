#pragma once

#include "../common/unique_fd.hpp"

#include <cstdint>
#include <string>

class Listener {
public:
    explicit Listener(std::uint16_t port);

    UniqueFd accept(int timeoutMs, std::string& peer);

private:
    UniqueFd fd_;
};
