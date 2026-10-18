#include "server_app.hpp"

#include "../common/encoding.hpp"
#include "../common/port.hpp"
#include "log.hpp"
#include "server.hpp"

#include <signal.h>

#include <csignal>
#include <exception>
#include <iostream>

namespace {
constexpr int EXIT_OK = 0;
constexpr int EXIT_FAILED = 1;
constexpr int EXIT_USAGE = 2;
constexpr const char* UPLOAD_DIR = "uploads";

volatile std::sig_atomic_t g_stop = 0;

void onStopSignal(int) { g_stop = 1; }
}

int ServerApp::run(int argc, char** argv) {
    if (!parseArgs(argc, argv)) return EXIT_USAGE;
    initLocaleEncoding();
    setupSignals();
    try {
        Server server(port_, UPLOAD_DIR);
        server.run(g_stop);
        logLine("Сервер остановлен");
        return EXIT_OK;
    } catch (const std::exception& e) {
        std::cerr << "Ошибка: " << e.what() << "\n";
        return EXIT_FAILED;
    }
}

bool ServerApp::parseArgs(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "Использование: " << argv[0] << " <порт>\n";
        return false;
    }
    if (!parsePort(argv[1], port_)) {
        std::cerr << "Ошибка: порт должен быть числом от 1 до 65535, получено '" << argv[1] << "'\n";
        return false;
    }
    return true;
}

void ServerApp::setupSignals() {
    ::signal(SIGPIPE, SIG_IGN);
    struct sigaction sa {};
    sa.sa_handler = onStopSignal;
    sa.sa_flags = SA_RESETHAND;
    sigemptyset(&sa.sa_mask);
    ::sigaction(SIGINT, &sa, nullptr);
    ::sigaction(SIGTERM, &sa, nullptr);
}
