#include "listener.hpp"

#include "../common/errors.hpp"

#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <poll.h>
#include <sys/socket.h>

namespace {
UniqueFd tryListen(int family, std::uint16_t port) {
    UniqueFd fd(::socket(family, SOCK_STREAM, 0));
    if (!fd.valid()) return UniqueFd();

    int on = 1;
    ::setsockopt(fd.get(), SOL_SOCKET, SO_REUSEADDR, &on, sizeof on);

    int rc;
    if (family == AF_INET6) {
        int off = 0;
        ::setsockopt(fd.get(), IPPROTO_IPV6, IPV6_V6ONLY, &off, sizeof off);
        sockaddr_in6 addr{};
        addr.sin6_family = AF_INET6;
        addr.sin6_addr = in6addr_any;
        addr.sin6_port = htons(port);
        rc = ::bind(fd.get(), reinterpret_cast<sockaddr*>(&addr), sizeof addr);
    } else {
        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = htonl(INADDR_ANY);
        addr.sin_port = htons(port);
        rc = ::bind(fd.get(), reinterpret_cast<sockaddr*>(&addr), sizeof addr);
    }
    if (rc != 0 || ::listen(fd.get(), SOMAXCONN) != 0) return UniqueFd();

    int flags = ::fcntl(fd.get(), F_GETFL, 0);
    if (flags >= 0) ::fcntl(fd.get(), F_SETFL, flags | O_NONBLOCK);
    return fd;
}

std::string peerToString(const sockaddr_storage& addr, socklen_t len) {
    char host[NI_MAXHOST], serv[NI_MAXSERV];
    if (::getnameinfo(reinterpret_cast<const sockaddr*>(&addr), len, host, sizeof host, serv,
                      sizeof serv, NI_NUMERICHOST | NI_NUMERICSERV) != 0)
        return "неизвестный адрес";
    std::string h = host;
    if (h.compare(0, 7, "::ffff:") == 0) h.erase(0, 7);
    return h + ":" + serv;
}
}

Listener::Listener(std::uint16_t port) {
    fd_ = tryListen(AF_INET6, port);
    if (!fd_.valid()) fd_ = tryListen(AF_INET, port);
    if (!fd_.valid()) throwErrno("не удалось открыть порт " + std::to_string(port));
}

UniqueFd Listener::accept(int timeoutMs, std::string& peer) {
    pollfd p{fd_.get(), POLLIN, 0};
    int r = ::poll(&p, 1, timeoutMs);
    if (r < 0) {
        if (errno == EINTR) return UniqueFd();
        throwErrno("ошибка ожидания соединения (poll)");
    }
    if (r == 0) return UniqueFd();

    sockaddr_storage addr{};
    socklen_t len = sizeof addr;
    UniqueFd client(::accept(fd_.get(), reinterpret_cast<sockaddr*>(&addr), &len));
    if (!client.valid()) {
        if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK || errno == ECONNABORTED)
            return UniqueFd();
        throwErrno("ошибка accept");
    }

    int flags = ::fcntl(client.get(), F_GETFL, 0);
    if (flags >= 0) ::fcntl(client.get(), F_SETFL, flags & ~O_NONBLOCK);

    peer = peerToString(addr, len);
    return client;
}
