#include "multicast_socket.h"

#include <arpa/inet.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <netdb.h>
#include <netinet/in.h>
#include <unistd.h>

#include <algorithm>
#include <cerrno>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <vector>

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

struct InterfaceInfo {
    std::string name;
    unsigned int flags = 0;
    bool hasIPv4 = false;
    bool hasIPv6 = false;
    in_addr ipv4{};
};

std::vector<InterfaceInfo> listInterfaces() {
    ifaddrs* list = nullptr;
    if (getifaddrs(&list) < 0) throw std::runtime_error(sysError("getifaddrs"));

    std::vector<InterfaceInfo> result;
    for (ifaddrs* it = list; it != nullptr; it = it->ifa_next) {
        auto pos = std::find_if(result.begin(), result.end(),
                                [&](const InterfaceInfo& i) { return i.name == it->ifa_name; });
        if (pos == result.end()) {
            result.push_back(InterfaceInfo{});
            pos = result.end() - 1;
            pos->name = it->ifa_name;
        }
        pos->flags |= it->ifa_flags;
        if (it->ifa_addr == nullptr) continue;
        if (it->ifa_addr->sa_family == AF_INET && !pos->hasIPv4) {
            pos->hasIPv4 = true;
            pos->ipv4 = reinterpret_cast<sockaddr_in*>(it->ifa_addr)->sin_addr;
        } else if (it->ifa_addr->sa_family == AF_INET6) {
            pos->hasIPv6 = true;
        }
    }
    freeifaddrs(list);
    return result;
}

const char* familyName(int family) { return family == AF_INET ? "IPv4" : "IPv6"; }

bool hasAddress(const InterfaceInfo& i, int family) {
    return family == AF_INET ? i.hasIPv4 : i.hasIPv6;
}

void requireUsable(const InterfaceInfo& i, int family) {
    const std::string prefix = "интерфейс '" + i.name + "' ";
    if (!(i.flags & IFF_UP)) throw std::runtime_error(prefix + "не поднят");
    if (!(i.flags & IFF_MULTICAST)) throw std::runtime_error(prefix + "не поддерживает multicast");
    if (!hasAddress(i, family))
        throw std::runtime_error(prefix + "не имеет " + familyName(family) + "-адреса");
}

bool isAutoCandidate(const InterfaceInfo& i, int family) {
    return (i.flags & IFF_UP) && (i.flags & IFF_RUNNING) && (i.flags & IFF_MULTICAST) &&
           !(i.flags & IFF_LOOPBACK) && hasAddress(i, family);
}

InterfaceInfo selectInterface(int family, const std::string& requested) {
    const std::vector<InterfaceInfo> all = listInterfaces();

    if (!requested.empty()) {
        auto pos = std::find_if(all.begin(), all.end(),
                                [&](const InterfaceInfo& i) { return i.name == requested; });
        if (pos == all.end()) throw std::runtime_error("интерфейс '" + requested + "' не найден");
        requireUsable(*pos, family);
        return *pos;
    }

    std::vector<InterfaceInfo> candidates;
    for (const auto& i : all)
        if (isAutoCandidate(i, family)) candidates.push_back(i);

    if (candidates.empty())
        throw std::runtime_error("не найден подходящий сетевой интерфейс. Укажите интерфейс третьим аргументом.");
    if (candidates.size() > 1) {
        std::string names;
        for (const auto& i : candidates) names += (names.empty() ? "" : ", ") + i.name;
        std::cerr << "Предупреждение: подходящие интерфейсы - " << names << ", выбран "
                  << candidates.front().name << ". Указать явно его можно третьим аргументом.\n";
    }
    return candidates.front();
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

bool MulticastSocket::send(const std::string& msg) {
    ssize_t n = ::sendto(fd_, msg.data(), msg.size(), 0,
                         reinterpret_cast<const sockaddr*>(&group_), groupLen_);
    return n >= 0;
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
            throw std::runtime_error("Адрес " + group + " не является multicast-адресом IPv4");
        a->sin_port = htons(port);
    } else if (family_ == AF_INET6) {
        auto* a = reinterpret_cast<sockaddr_in6*>(&group_);
        if (!IN6_IS_ADDR_MULTICAST(&a->sin6_addr))
            throw std::runtime_error("Адрес " + group + " не является multicast-адресом IPv6");
        a->sin6_port = htons(port);
    } else {
        throw std::runtime_error("Неподдерживаемое семейство адресов");
    }
}

std::string MulticastSocket::requestedInterface(const std::string& iface) const {
    if (family_ != AF_INET6) return iface;

    unsigned int zoneIndex = reinterpret_cast<const sockaddr_in6*>(&group_)->sin6_scope_id;
    if (zoneIndex == 0) return iface;

    char zoneName[IF_NAMESIZE];
    if (if_indextoname(zoneIndex, zoneName) == nullptr)
        throw std::runtime_error(sysError("if_indextoname"));
    if (!iface.empty() && iface != zoneName)
        throw std::runtime_error("интерфейс в адресе (%зона) и в аргументе не совпадают");
    return zoneName;
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

    const std::string requested = requestedInterface(iface);
    ifaceAuto_ = requested.empty();
    const InterfaceInfo info = selectInterface(family_, requested);
    ifaceName_ = info.name;

    if (family_ == AF_INET)
        setupIPv4(port, info.ipv4);
    else
        setupIPv6(port);
}

void MulticastSocket::setupIPv4(uint16_t port, in_addr ifAddr) {
    sockaddr_in local{};
    local.sin_family = AF_INET;
    local.sin_addr.s_addr = htonl(INADDR_ANY);
    local.sin_port = htons(port);
    if (::bind(fd_, reinterpret_cast<sockaddr*>(&local), sizeof(local)) < 0)
        throw std::runtime_error(sysError("bind"));

    ip_mreq mreq{};
    mreq.imr_multiaddr = reinterpret_cast<sockaddr_in*>(&group_)->sin_addr;
    mreq.imr_interface = ifAddr;
    setOpt(IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq), "IP_ADD_MEMBERSHIP");
    setOpt(IPPROTO_IP, IP_MULTICAST_IF, &ifAddr, sizeof(ifAddr), "IP_MULTICAST_IF");

    unsigned char loop = kMulticastLoopV4;
    unsigned char ttl = kMulticastTtlV4;
    setOpt(IPPROTO_IP, IP_MULTICAST_LOOP, &loop, sizeof(loop), "IP_MULTICAST_LOOP");
    setOpt(IPPROTO_IP, IP_MULTICAST_TTL, &ttl, sizeof(ttl), "IP_MULTICAST_TTL");
}

void MulticastSocket::setupIPv6(uint16_t port) {
    int on = kSockOptOn;
    setOpt(IPPROTO_IPV6, IPV6_V6ONLY, &on, sizeof(on), "IPV6_V6ONLY");

    sockaddr_in6 local{};
    local.sin6_family = AF_INET6;
    local.sin6_addr = in6addr_any;
    local.sin6_port = htons(port);
    if (::bind(fd_, reinterpret_cast<sockaddr*>(&local), sizeof(local)) < 0)
        throw std::runtime_error(sysError("bind"));

    unsigned int ifIndex = if_nametoindex(ifaceName_.c_str());
    if (ifIndex == 0) throw std::runtime_error(sysError("if_nametoindex"));

    auto* group6 = reinterpret_cast<sockaddr_in6*>(&group_);
    group6->sin6_scope_id = ifIndex;

    ipv6_mreq mreq{};
    mreq.ipv6mr_multiaddr = group6->sin6_addr;
    mreq.ipv6mr_interface = ifIndex;
    setOpt(IPPROTO_IPV6, IPV6_JOIN_GROUP, &mreq, sizeof(mreq), "IPV6_JOIN_GROUP");

    unsigned int loop = kMulticastLoopV6;
    int hops = kMulticastHopsV6;
    setOpt(IPPROTO_IPV6, IPV6_MULTICAST_LOOP, &loop, sizeof(loop), "IPV6_MULTICAST_LOOP");
    setOpt(IPPROTO_IPV6, IPV6_MULTICAST_HOPS, &hops, sizeof(hops), "IPV6_MULTICAST_HOPS");
    setOpt(IPPROTO_IPV6, IPV6_MULTICAST_IF, &ifIndex, sizeof(ifIndex), "IPV6_MULTICAST_IF");
}

}