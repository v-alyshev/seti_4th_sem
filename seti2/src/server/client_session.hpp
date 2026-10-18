#pragma once

#include "../common/protocol.hpp"
#include "connection.hpp"

#include <cstdint>
#include <string>

class UploadStorage;

class ClientSession {
public:
    ClientSession(int fd, std::string peer, const UploadStorage& storage);

    void run() noexcept;

private:
    void receiveFile();
    proto::Header readHeader();
    std::uint64_t receiveBody(int fileFd, std::uint64_t size);
    void expectEndOfData();
    void finish(bool received);

    Connection conn_;
    std::string peer_;
    const UploadStorage& storage_;
    std::string savedName_;
    bool fileCreated_ = false;
    std::uint64_t size_ = 0;
};
