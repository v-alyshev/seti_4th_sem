#include "app.h"

#include <poll.h>
#include <signal.h>

#include <cerrno>
#include <csignal>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <stdexcept>

#include "announcer.h"
#include "listener.h"
#include "multicast_socket.h"
#include "peer_table.h"
#include "protocol.h"

namespace lan_copies {

namespace {
constexpr int kMinArgc = 2;
constexpr int kMaxArgc = 4;
constexpr int kGroupArgIndex = 1;
constexpr int kPortArgIndex = 2;
constexpr int kIfaceArgIndex = 3;
constexpr long kMinPort = 1;
constexpr long kMaxPort = 65535;
constexpr int kDecimalBase = 10;
constexpr nfds_t kPollFdCount = 1;

volatile std::sig_atomic_t g_stop = 0;

void onSignal(int) { g_stop = 1; }

std::string usage(const char* progName) {
    std::ostringstream oss;
    oss << "Использование: " << progName << " <адрес multicast-группы> [порт] [интерфейс]\n"
        << "Примеры:\n"
        << "  " << progName << " 239.192.0.1\n"
        << "  " << progName << " ff02::4321\n"
        << "  " << progName << " ff02::4321%eth0 40000\n"
        << "  " << progName << " 239.192.0.1 30000 eth0";
    return oss.str();
}

void installSignalHandlers() {
    struct sigaction sa{};
    sa.sa_handler = onSignal;
    sigemptyset(&sa.sa_mask);
    if (sigaction(SIGINT, &sa, nullptr) < 0 || sigaction(SIGTERM, &sa, nullptr) < 0)
        throw std::runtime_error(sysError("sigaction"));
}

void printStartupMessage(const Args& args, const MulticastSocket& sock, const std::string& myId) {
    std::cout << "Копия " << myId << " запущена. Группа " << args.group << ", порт " << args.port
              << ", протокол " << (sock.isIPv6() ? "IPv6" : "IPv4")
              << ", интерфейс " << sock.interfaceName()
              << (sock.interfaceAutoSelected() ? " (выбран автоматически)" : "") << ".\n"
              << "Для выхода нажмите Ctrl+C.\n";
}

bool waitReadable(int fd, Clock::time_point deadline) {
    auto waitMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                      deadline - Clock::now()).count();
    if (waitMs < 0) waitMs = 0;

    pollfd pfd{};
    pfd.fd = fd;
    pfd.events = POLLIN;
    int rc = ::poll(&pfd, kPollFdCount, static_cast<int>(waitMs));
    if (rc < 0) {
        if (errno == EINTR) return false;
        throw std::runtime_error(sysError("poll"));
    }
    return rc > 0 && (pfd.revents & POLLIN);
}

void runLoop(MulticastSocket& sock, const std::string& myId) {
    PeerTable peers;
    Announcer announcer(sock, myId);
    Listener listener(sock, peers, myId);

    peers.print();

    while (!g_stop) {
        auto nextSend = announcer.tick(Clock::now());

        bool changed = false;
        if (waitReadable(sock.fd(), nextSend))
            changed |= listener.handleIncoming(Clock::now());
        changed |= peers.expire(Clock::now());

        if (changed) peers.print();
    }

    announcer.sayBye();
    std::cout << "\nЗавершение работы.\n";
}
}

Args parseArgs(int argc, char* argv[]) {
    if (argc < kMinArgc || argc > kMaxArgc)
        throw std::runtime_error("неверное число аргументов\n" + usage(argv[0]));

    Args args;
    args.group = argv[kGroupArgIndex];

    if (argc > kPortArgIndex) {
        const char* portStr = argv[kPortArgIndex];
        char* end = nullptr;
        long p = std::strtol(portStr, &end, kDecimalBase);
        if (*end != '\0' || p < kMinPort || p > kMaxPort)
            throw std::runtime_error("неверный порт: " + std::string(portStr) + "\n" + usage(argv[0]));
        args.port = static_cast<uint16_t>(p);
    }
    if (argc > kIfaceArgIndex) args.iface = argv[kIfaceArgIndex];
    return args;
}

void run(const Args& args) {
    installSignalHandlers();
    MulticastSocket sock(args.group, args.port, args.iface);
    const std::string myId = randomId();
    printStartupMessage(args, sock, myId);
    runLoop(sock, myId);
}

}