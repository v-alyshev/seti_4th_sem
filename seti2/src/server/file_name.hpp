#pragma once

#include <cstddef>
#include <string>

std::string sanitizeFileName(std::string name);

std::string truncateUtf8(const std::string& s, std::size_t maxBytes);
