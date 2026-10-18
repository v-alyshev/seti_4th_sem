#include "server.hpp"

#include "client_session.hpp"
#include "log.hpp"

#include <chrono>
#include <exception>
#include <thread>

namespace {
constexpr int ACCEPT_TIMEOUT_MS = 500;
}

Server::Server(std::uint16_t port, const std::string& uploadDir)
    : storage_(uploadDir), listener_(port) {
    logLine("Сервер слушает порт " + std::to_string(port) + ", файлы сохраняются в ./" + uploadDir);
}

void Server::run(const volatile std::sig_atomic_t& stop) {
    while (!stop) {
        try {
            std::string peer;
            UniqueFd sock = listener_.accept(ACCEPT_TIMEOUT_MS, peer);
            if (sock.valid()) startClient(std::move(sock), peer);
        } catch (const std::exception& e) {
            logLine(std::string("Ошибка приёма соединения: ") + e.what());
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }
    }
    waitForClients();
}

void Server::startClient(UniqueFd sock, const std::string& peer) {
    {
        std::lock_guard<std::mutex> lock(activeMutex_);
        ++active_;
    }
    try {
        std::thread(&Server::clientThread, this, sock.get(), peer).detach();
        sock.release();
    } catch (const std::exception& e) {
        clientFinished();
        logLine("[" + peer + "] ОШИБКА: не удалось создать поток: " + e.what());
    }
}

void Server::clientThread(int rawFd, std::string peer) {
    {
        UniqueFd sock(rawFd);
        try {
            ClientSession(sock.get(), std::move(peer), storage_).run();
        } catch (...) {
        }
    }
    clientFinished();
}

void Server::clientFinished() {
    std::lock_guard<std::mutex> lock(activeMutex_);
    --active_;
    activeCv_.notify_all();
}

void Server::waitForClients() {
    std::unique_lock<std::mutex> lock(activeMutex_);
    if (active_ > 0)
        logLine("Остановка: ожидание завершения активных клиентов (" + std::to_string(active_) + ")...");
    activeCv_.wait(lock, [this] { return active_ == 0; });
}
