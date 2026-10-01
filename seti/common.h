#pragma once

#include <chrono>
#include <string>

namespace lan_copies {

    using Clock = std::chrono::steady_clock;

    std::string sysError(const std::string& what);
    std::string timeStamp();

}