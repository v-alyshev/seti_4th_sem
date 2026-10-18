#include "client_app.hpp"

#include "../common/encoding.hpp"
#include "../common/port.hpp"
#include "../common/units.hpp"
#include "file_sender.hpp"

#include <iostream>

namespace {
constexpr int EXIT_OK = 0;
constexpr int EXIT_FAILED = 1;
constexpr int EXIT_USAGE = 2;

std::string utf8ToLocalOrSame(const std::string& name) {
    try {
        return utf8ToLocal(name);
    } catch (...) {
        return name;
    }
}
}

int ClientApp::run(int argc, char** argv) {
    if (!parseArgs(argc, argv)) return EXIT_USAGE;
    initLocaleEncoding();
    return report(FileSender(host_, port_).send(path_));
}

bool ClientApp::parseArgs(int argc, char** argv) {
    if (argc != 4) {
        std::cerr << "Использование: " << argv[0] << " <файл> <хост> <порт>\n";
        return false;
    }
    if (!parsePort(argv[3], port_)) {
        std::cerr << "Ошибка: порт должен быть числом от 1 до 65535, получено '" << argv[3] << "'\n";
        return false;
    }
    path_ = argv[1];
    host_ = argv[2];
    if (path_.empty() || host_.empty()) {
        std::cerr << "Ошибка: путь к файлу и хост не должны быть пустыми\n";
        return false;
    }
    return true;
}

int ClientApp::report(const SendResult& result) {
    if (result.ok) {
        std::cout << "Файл '" << utf8ToLocalOrSame(result.name) << "' (" << formatSize(result.size)
                  << ") успешно передан\n";
        return EXIT_OK;
    }
    std::cout << "Передача файла НЕ УДАЛАСЬ: " << result.error << "\n";
    return EXIT_FAILED;
}
