#pragma once

#include "../common/unique_fd.hpp"

#include <string>

class UploadStorage {
public:
    explicit UploadStorage(std::string dir);

    const std::string& dir() const { return dir_; }

    UniqueFd createUnique(const std::string& name, std::string& savedName) const;

    void remove(const std::string& name) const noexcept;

private:
    std::string dir_;
    UniqueFd dirFd_;
};
