#pragma once

#include "../common/unique_fd.hpp"
#include "listener.hpp"
#include "upload_storage.hpp"

#include <condition_variable>
#include <csignal>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <string>

class Server {
public:
    Server(std::uint16_t port, const std::string& uploadDir);

    void run(const volatile std::sig_atomic_t& stop);

private:
    void startClient(UniqueFd sock, const std::string& peer);
    void clientThread(int rawFd, std::string peer);
    void clientFinished();
    void waitForClients();

    UploadStorage storage_;
    Listener listener_;

    std::mutex activeMutex_;
    std::condition_variable activeCv_;
    std::size_t active_ = 0;
};
