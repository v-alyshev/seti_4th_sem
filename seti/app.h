#pragma once

#include <cstdint>
#include <string>

namespace lan_copies {

    constexpr uint16_t kDefaultPort = 30000;

    struct Args {
        std::string group;
        uint16_t port = kDefaultPort;
        std::string iface;
    };

    Args parseArgs(int argc, char* argv[]);
    void run(const Args& args);

}