#pragma once

#include "../common/unique_fd.hpp"

#include <cstddef>
#include <cstdint>
#include <string>

UniqueFd connectTo(const std::string& host, std::uint16_t port);

void sendAll(int fd, const void* data, std::size_t len);

bool recvStatus(int fd, std::uint8_t& status);
