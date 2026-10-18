#pragma once

#include <cstdint>
#include <string>

struct SendResult;

class ClientApp {
public:
    int run(int argc, char** argv);

private:
    bool parseArgs(int argc, char** argv);
    static int report(const SendResult& result);

    std::string path_;
    std::string host_;
    std::uint16_t port_ = 0;
};
