#include "multicast_socket.h"

#include <arpa/inet.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <netdb.h>
#include <netinet/in.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>
#include <iostream>
#include <stdexcept>

#include "common.h"

namespace lan_copies {

namespace {
constexpr std::size_t kRecvBufferSize = 1500;
constexpr int kSockOptOn = 1;
constexpr unsigned char kMulticastTtlV4 = 1;
constexpr int kMulticastHopsV6 = 1;
constexpr unsigned char kMulticastLoopV4 = 1;
constexpr unsigned int kMulticastLoopV6 = 1;

std::string addrToString(const sockaddr* sa, socklen_t len) {
    char host[NI_MAXHOST];
    int rc = getnameinfo(sa, len, host, sizeof(host), nullptr, 0, NI_NUMERICHOST);
    if (rc != 0) return std::string("<") + gai_strerror(rc) + ">";
    return host;
}

in_addr ipv4AddrOfInterface(const std::string& name) {
    ifaddrs* list = nullptr;
    if (getifaddrs(&list) < 0) throw std::runtime_error(sysError("getifaddrs"));
    in_addr result{};
    bool found = false;
    for (ifaddrs* it = list; it != nullptr; it = it->ifa_next) {
        if (it->ifa_addr && it->ifa_addr->sa_family == AF_INET && name == it->ifa_name) {
            result = reinterpret_cast<sockaddr_in*>(it->ifa_addr)->sin_addr;
            found = true;
            break;
        }
    }
    freeifaddrs(list);
    if (!found) throw std::runtime_error("у интерфейса '" + name + "' нет IPv4-адреса");
    return result;
}
}

MulticastSocket::MulticastSocket(const std::string& group, uint16_t port, const std::string& iface) {
    resolveGroup(group, port);
    try {
        openSocket(port, iface);
    } catch (...) {
        if (fd_ >= 0) ::close(fd_);
        throw;
    }
}

MulticastSocket::~MulticastSocket() {
    if (fd_ >= 0) ::close(fd_);
}

bool MulticastSocket::isIPv6() const { return family_ == AF_INET6; }

void MulticastSocket::send(const std::string& msg) {
    ssize_t n = ::sendto(fd_, msg.data(), msg.size(), 0,
                         reinterpret_cast<const sockaddr*>(&group_), groupLen_);
    if (n < 0) {
        std::cerr << "[" << timeStamp() << "] sendto: " << std::strerror(errno) << "\n";
    }
}

std::optional<Datagram> MulticastSocket::receive() {
    char buf[kRecvBufferSize];
    sockaddr_storage from{};
    socklen_t fromLen = sizeof(from);
    ssize_t n = ::recvfrom(fd_, buf, sizeof(buf), 0,
                           reinterpret_cast<sockaddr*>(&from), &fromLen);
    if (n < 0) {
        if (errno != EINTR && errno != EAGAIN)
            std::cerr << "[" << timeStamp() << "] recvfrom: " << std::strerror(errno) << "\n";
        return std::nullopt;
    }
    return Datagram{std::string(buf, static_cast<size_t>(n)),
                    addrToString(reinterpret_cast<sockaddr*>(&from), fromLen)};
}

void MulticastSocket::resolveGroup(const std::string& group, uint16_t port) {
    addrinfo hints{};
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_NUMERICHOST;

    addrinfo* res = nullptr;
    int rc = getaddrinfo(group.c_str(), nullptr, &hints, &res);
    if (rc != 0)
        throw std::runtime_error("Неверный адрес группы '" + group + "': " + gai_strerror(rc));

    family_ = res->ai_family;
    std::memcpy(&group_, res->ai_addr, res->ai_addrlen);
    groupLen_ = res->ai_addrlen;
    freeaddrinfo(res);

    if (family_ == AF_INET) {
        auto* a = reinterpret_cast<sockaddr_in*>(&group_);
        if (!IN_MULTICAST(ntohl(a->sin_addr.s_addr)))
            throw std::runtime_error("Адрес " + group + " не является multicast-адресом IPv4 (224.0.0.0/4)");
        a->sin_port = htons(port);
    } else if (family_ == AF_INET6) {
        auto* a = reinterpret_cast<sockaddr_in6*>(&group_);
        if (!IN6_IS_ADDR_MULTICAST(&a->sin6_addr))
            throw std::runtime_error("Адрес " + group + " не является multicast-адресом IPv6 (ff00::/8)");
        a->sin6_port = htons(port);
    } else {
        throw std::runtime_error("Неподдерживаемое семейство адресов");
    }
}

unsigned int MulticastSocket::resolveIPv6Interface(unsigned int zoneIndex, const std::string& iface) {
    if (iface.empty()) return zoneIndex;
    unsigned int idx = if_nametoindex(iface.c_str());
    if (idx == 0) throw std::runtime_error("интерфейс '" + iface + "' не найден");
    if (zoneIndex != 0 && zoneIndex != idx)
        throw std::runtime_error("интерфейс в адресе (%зона) и в аргументе не совпадают");
    return idx;
}

void MulticastSocket::setOpt(int level, int name, const void* val, socklen_t len, const char* what) {
    if (::setsockopt(fd_, level, name, val, len) < 0)
        throw std::runtime_error(sysError(std::string("setsockopt(") + what + ")"));
}

void MulticastSocket::openSocket(uint16_t port, const std::string& iface) {
    fd_ = ::socket(family_, SOCK_DGRAM, 0);
    if (fd_ < 0) throw std::runtime_error(sysError("socket"));

    int on = kSockOptOn;
    setOpt(SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on), "SO_REUSEADDR");
#ifdef SO_REUSEPORT
    setOpt(SOL_SOCKET, SO_REUSEPORT, &on, sizeof(on), "SO_REUSEPORT");
#endif
    if (family_ == AF_INET)
        setupIPv4(port, iface);
    else
        setupIPv6(port, iface);
}

void MulticastSocket::setupIPv4(uint16_t port, const std::string& iface) {
    sockaddr_in local{};
    local.sin_family = AF_INET;
    local.sin_addr.s_addr = htonl(INADDR_ANY);
    local.sin_port = htons(port);
    if (::bind(fd_, reinterpret_cast<sockaddr*>(&local), sizeof(local)) < 0)
        throw std::runtime_error(sysError("bind"));

    in_addr ifAddr{};
    ifAddr.s_addr = htonl(INADDR_ANY);
    if (!iface.empty()) ifAddr = ipv4AddrOfInterface(iface);

    ip_mreq mreq{};
    mreq.imr_multiaddr = reinterpret_cast<sockaddr_in*>(&group_)->sin_addr;
    mreq.imr_interface = ifAddr;
    setOpt(IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq), "IP_ADD_MEMBERSHIP");
    if (!iface.empty())
        setOpt(IPPROTO_IP, IP_MULTICAST_IF, &ifAddr, sizeof(ifAddr), "IP_MULTICAST_IF");

    unsigned char loop = kMulticastLoopV4;
    unsigned char ttl = kMulticastTtlV4;
    setOpt(IPPROTO_IP, IP_MULTICAST_LOOP, &loop, sizeof(loop), "IP_MULTICAST_LOOP");
    setOpt(IPPROTO_IP, IP_MULTICAST_TTL, &ttl, sizeof(ttl), "IP_MULTICAST_TTL");
}

void MulticastSocket::setupIPv6(uint16_t port, const std::string& iface) {
    int on = kSockOptOn;
    setOpt(IPPROTO_IPV6, IPV6_V6ONLY, &on, sizeof(on), "IPV6_V6ONLY");

    sockaddr_in6 local{};
    local.sin6_family = AF_INET6;
    local.sin6_addr = in6addr_any;
    local.sin6_port = htons(port);
    if (::bind(fd_, reinterpret_cast<sockaddr*>(&local), sizeof(local)) < 0)
        throw std::runtime_error(sysError("bind"));

    auto* group6 = reinterpret_cast<sockaddr_in6*>(&group_);
    unsigned int ifIndex = resolveIPv6Interface(group6->sin6_scope_id, iface);
    group6->sin6_scope_id = ifIndex;
    if (ifIndex == 0 && IN6_IS_ADDR_MC_LINKLOCAL(&group6->sin6_addr))
        std::cerr << "Предупреждение: для link-local группы не указан интерфейс, "
                     "будет выбран интерфейс по умолчанию. Укажите его: "
                     "ff02::4321%eth0 или третьим аргументом.\n";

    ipv6_mreq mreq{};
    mreq.ipv6mr_multiaddr = group6->sin6_addr;
    mreq.ipv6mr_interface = ifIndex;
    setOpt(IPPROTO_IPV6, IPV6_JOIN_GROUP, &mreq, sizeof(mreq), "IPV6_JOIN_GROUP");

    unsigned int loop = kMulticastLoopV6;
    int hops = kMulticastHopsV6;
    setOpt(IPPROTO_IPV6, IPV6_MULTICAST_LOOP, &loop, sizeof(loop), "IPV6_MULTICAST_LOOP");
    setOpt(IPPROTO_IPV6, IPV6_MULTICAST_HOPS, &hops, sizeof(hops), "IPV6_MULTICAST_HOPS");
    if (ifIndex != 0)
        setOpt(IPPROTO_IPV6, IPV6_MULTICAST_IF, &ifIndex, sizeof(ifIndex), "IPV6_MULTICAST_IF");
}

}
