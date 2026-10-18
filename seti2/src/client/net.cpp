#include "net.hpp"

#include "../common/errors.hpp"

#include <netdb.h>
#include <sys/socket.h>

#include <memory>
#include <stdexcept>
#include <string>

UniqueFd connectTo(const std::string& host, std::uint16_t port) {
    addrinfo hints{};
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_NUMERICSERV;

    const std::string service = std::to_string(port);
    addrinfo* res = nullptr;
    int rc = ::getaddrinfo(host.c_str(), service.c_str(), &hints, &res);
    if (rc != 0) throw std::runtime_error("не удалось найти адрес " + host + ": " + ::gai_strerror(rc));
    std::unique_ptr<addrinfo, void (*)(addrinfo*)> guard(res, ::freeaddrinfo);

    int lastErr = 0;
    for (addrinfo* ai = res; ai != nullptr; ai = ai->ai_next) {
        UniqueFd fd(::socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol));
        if (!fd.valid()) {
            lastErr = errno;
            continue;
        }
        if (::connect(fd.get(), ai->ai_addr, ai->ai_addrlen) == 0) return fd;
        lastErr = errno;
    }
    errno = lastErr;
    throwErrno("не удалось подключиться к " + host + ":" + service);
}

void sendAll(int fd, const void* data, std::size_t len) {
    auto* p = static_cast<const char*>(data);
    while (len > 0) {
        ssize_t n = ::send(fd, p, len, MSG_NOSIGNAL);
        if (n < 0) {
            if (errno == EINTR) continue;
            throwErrno("ошибка отправки данных");
        }
        p += n;
        len -= static_cast<std::size_t>(n);
    }
}

bool recvStatus(int fd, std::uint8_t& status) {
    for (;;) {
        ssize_t n = ::recv(fd, &status, 1, 0);
        if (n == 1) return true;
        if (n < 0 && errno == EINTR) continue;
        return false;
    }
}
