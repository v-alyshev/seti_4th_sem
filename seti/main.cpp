#include <cstdlib>
#include <exception>
#include <iostream>

#include "app.h"

int main(int argc, char* argv[]) {
    try {
        lan_copies::run(lan_copies::parseArgs(argc, argv));
        return EXIT_SUCCESS;
    } catch (const std::exception& e) {
        std::cerr << "Ошибка: " << e.what() << "\n";
        return EXIT_FAILURE;
    }
}
