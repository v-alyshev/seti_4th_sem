#include "upload_storage.hpp"

#include "../common/encoding.hpp"
#include "../common/errors.hpp"
#include "file_name.hpp"

#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include <stdexcept>

namespace {
constexpr std::size_t NAME_MAX_BYTES = 255;

std::string toDiskName(const std::string& utf8Name) {
    try {
        std::string local = utf8ToLocal(utf8Name);
        if (!local.empty() && local.find('/') == std::string::npos &&
            local.find('\0') == std::string::npos)
            return local;
    } catch (const std::exception&) {
    }
    return utf8Name;
}
}

UploadStorage::UploadStorage(std::string dir) : dir_(std::move(dir)) {
    if (::mkdir(dir_.c_str(), 0755) != 0 && errno != EEXIST) throwErrno("не удалось создать каталог " + dir_);
    dirFd_ = UniqueFd(::open(dir_.c_str(), O_RDONLY | O_DIRECTORY));
    if (!dirFd_.valid()) throwErrno("не удалось открыть каталог " + dir_);
}

UniqueFd UploadStorage::createUnique(const std::string& name, std::string& savedName) const {
    std::string stem = name, ext;
    std::size_t dot = name.rfind('.');
    if (dot != std::string::npos && dot > 0 && name.size() - dot <= 16) {
        stem = name.substr(0, dot);
        ext = name.substr(dot);
    }

    for (unsigned attempt = 0; attempt < 100000; ++attempt) {
        std::string suffix = attempt ? "_" + std::to_string(attempt) : "";
        std::string candidate =
            truncateUtf8(stem, NAME_MAX_BYTES - suffix.size() - ext.size()) + suffix + ext;

        int fd = ::openat(dirFd_.get(), toDiskName(candidate).c_str(),
                          O_WRONLY | O_CREAT | O_EXCL | O_NOFOLLOW, 0644);
        if (fd >= 0) {
            savedName = candidate;
            return UniqueFd(fd);
        }
        if (errno != EEXIST) throwErrno("не удалось создать файл в " + dir_);
    }
    throw std::runtime_error("не удалось подобрать свободное имя файла");
}

void UploadStorage::remove(const std::string& name) const noexcept {
    try {
        ::unlinkat(dirFd_.get(), toDiskName(name).c_str(), 0);
    } catch (...) {
    }
}
