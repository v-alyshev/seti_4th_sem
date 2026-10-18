#pragma once

#include <cerrno>
#include <string>
#include <system_error>

[[noreturn]] inline void throwErrno(const std::string& what) {
    throw std::system_error(errno, std::generic_category(), what);
}
