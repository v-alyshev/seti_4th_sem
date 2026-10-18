#pragma once

#include <cstdint>

class ServerApp {
public:
    int run(int argc, char** argv);

private:
    bool parseArgs(int argc, char** argv);
    static void setupSignals();

    std::uint16_t port_ = 0;
};
