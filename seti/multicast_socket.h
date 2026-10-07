#pragma once

#include <netinet/in.h>
#include <sys/socket.h>

#include <cstdint>
#include <optional>
#include <string>

namespace lan_copies {

    struct Datagram {
        std::string data;
        std::string fromIp;
    };

    class MulticastSocket {
    public:
        MulticastSocket(const std::string& group, uint16_t port, const std::string& iface);
        ~MulticastSocket();

        MulticastSocket(const MulticastSocket&) = delete;
        MulticastSocket& operator=(const MulticastSocket&) = delete;

        int fd() const { return fd_; }
        bool isIPv6() const;
        const std::string& interfaceName() const { return ifaceName_; }
        bool interfaceAutoSelected() const { return ifaceAuto_; }

        bool send(const std::string& msg);
        std::optional<Datagram> receive();

    private:
        int fd_ = -1;
        int family_ = AF_UNSPEC;
        sockaddr_storage group_{};
        socklen_t groupLen_ = 0;
        std::string ifaceName_;
        bool ifaceAuto_ = false;

        void resolveGroup(const std::string& group, uint16_t port);
        std::string requestedInterface(const std::string& iface) const;
        void setOpt(int level, int name, const void* val, socklen_t len, const char* what);
        void openSocket(uint16_t port, const std::string& iface);
        void setupIPv4(uint16_t port, in_addr ifAddr);
        void setupIPv6(uint16_t port);
    };

}