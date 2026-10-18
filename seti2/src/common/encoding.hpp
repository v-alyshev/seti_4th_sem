#pragma once

#include <string>

void initLocaleEncoding();

std::string localeCharset();

std::string localToUtf8(const std::string& text);

std::string utf8ToLocal(const std::string& text);
