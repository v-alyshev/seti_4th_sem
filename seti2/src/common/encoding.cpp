#include "encoding.hpp"

#include <iconv.h>
#include <langinfo.h>

#include <cerrno>
#include <clocale>
#include <cstring>
#include <stdexcept>
#include <vector>

namespace {
bool isUtf8Name(const std::string& charset) {
    return charset == "UTF-8" || charset == "utf-8" || charset == "UTF8" || charset == "utf8";
}

class Converter {
public:
    Converter(const std::string& from, const std::string& to)
        : cd_(::iconv_open(to.c_str(), from.c_str())) {
        if (cd_ == reinterpret_cast<iconv_t>(-1))
            throw std::runtime_error("перекодировка " + from + " -> " + to + " не поддерживается");
    }
    ~Converter() { ::iconv_close(cd_); }
    Converter(const Converter&) = delete;
    Converter& operator=(const Converter&) = delete;

    std::string run(const std::string& text) {
        std::string out;
        std::vector<char> buf(4 * text.size() + 16);
        char* in = const_cast<char*>(text.data());
        std::size_t inLeft = text.size();
        while (inLeft > 0) {
            char* outPtr = buf.data();
            std::size_t outLeft = buf.size();
            if (::iconv(cd_, &in, &inLeft, &outPtr, &outLeft) == static_cast<std::size_t>(-1) &&
                errno != E2BIG)
                throw std::runtime_error("не удалось перекодировать имя файла (" +
                                         std::string(std::strerror(errno)) + ")");
            out.append(buf.data(), buf.size() - outLeft);
        }
        return out;
    }

private:
    iconv_t cd_;
};

std::string convert(const std::string& text, const std::string& from, const std::string& to) {
    if (isUtf8Name(from) && isUtf8Name(to)) return text;
    return Converter(from, to).run(text);
}
}

void initLocaleEncoding() {
    if (std::setlocale(LC_CTYPE, "") == nullptr) std::setlocale(LC_CTYPE, "C.UTF-8");
    std::setlocale(LC_MESSAGES, "");
}

std::string localeCharset() {
    const char* cs = ::nl_langinfo(CODESET);
    std::string charset = (cs != nullptr && *cs != '\0') ? cs : "UTF-8";
    if (charset == "ANSI_X3.4-1968" || charset == "ASCII" || charset == "US-ASCII")
        return "UTF-8";
    return charset;
}

std::string localToUtf8(const std::string& text) {
    return convert(text, localeCharset(), "UTF-8");
}

std::string utf8ToLocal(const std::string& text) {
    return convert(text, "UTF-8", localeCharset());
}
