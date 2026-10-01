#include "common.h"

#include <cerrno>
#include <cstring>
#include <ctime>
#include <iomanip>
#include <sstream>

namespace lan_copies {

namespace {
const char* const kTimeFormat = "%H:%M:%S";
}

std::string sysError(const std::string& what) {
    return what + ": " + std::strerror(errno);
}

std::string timeStamp() {
    std::time_t t = std::time(nullptr);
    std::tm tm{};
    localtime_r(&t, &tm);
    std::ostringstream oss;
    oss << std::put_time(&tm, kTimeFormat);
    return oss.str();
}

}
