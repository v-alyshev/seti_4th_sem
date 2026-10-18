#include "connection.hpp"

#include "../common/errors.hpp"
#include "../common/units.hpp"
#include "log.hpp"

#include <poll.h>
#include <sys/socket.h>

#include <algorithm>
#include <chrono>
#include <stdexcept>

#ifndef MSG_NOSIGNAL
#define MSG_NOSIGNAL 0
#endif

namespace {
constexpr auto REPORT_INTERVAL = std::chrono::seconds(3);
constexpr auto IDLE_TIMEOUT = std::chrono::seconds(60);
}

Connection::Connection(int fd, std::string peer)
    : fd_(fd), label_(std::move(peer)), lastActivity_(SpeedMeter::Clock::now()) {}

std::size_t Connection::readSome(void* buf, std::size_t len) {
    for (;;) {
        auto now = SpeedMeter::Clock::now();
        if (now - meter_.lastReport() >= REPORT_INTERVAL) {
            report(now);
            continue;
        }
        if (now - lastActivity_ >= IDLE_TIMEOUT)
            throw std::runtime_error("клиент слишком долго не присылает данные");

        auto deadline =
            std::min(meter_.lastReport() + REPORT_INTERVAL, lastActivity_ + IDLE_TIMEOUT);
        auto ms =
            std::chrono::duration_cast<std::chrono::milliseconds>(deadline - now).count() + 1;

        pollfd p{fd_, POLLIN, 0};
        int r = ::poll(&p, 1, static_cast<int>(ms));
        if (r < 0) {
            if (errno == EINTR) continue;
            throwErrno("ошибка ожидания данных (poll)");
        }
        if (r == 0) continue;

        ssize_t n = ::recv(fd_, buf, len, 0);
        if (n < 0) {
            if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK) continue;
            throwErrno("ошибка приёма данных");
        }
        lastActivity_ = SpeedMeter::Clock::now();
        meter_.add(static_cast<std::uint64_t>(n));
        return static_cast<std::size_t>(n);
    }
}

void Connection::readExact(void* buf, std::size_t len) {
    auto* p = static_cast<char*>(buf);
    while (len > 0) {
        std::size_t n = readSome(p, len);
        if (n == 0) throw std::runtime_error("соединение закрыто посреди заголовка");
        p += n;
        len -= n;
    }
}

void Connection::finalReport() {
    if (meter_.hasUnreported()) report(SpeedMeter::Clock::now());
}

bool Connection::sendStatus(std::uint8_t status) noexcept {
    for (;;) {
        ssize_t n = ::send(fd_, &status, 1, MSG_NOSIGNAL);
        if (n == 1) return true;
        if (n < 0 && errno == EINTR) continue;
        return false;
    }
}

void Connection::report(SpeedMeter::Clock::time_point now) {
    SpeedMeter::Speeds s = meter_.takeReport(now);
    logLine("[" + label_ + "] мгновенная скорость: " + formatSpeed(s.instant) +
            ", средняя: " + formatSpeed(s.average));
}
