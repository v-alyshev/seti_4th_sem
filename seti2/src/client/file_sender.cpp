#include "file_sender.hpp"

#include "../common/encoding.hpp"
#include "../common/errors.hpp"
#include "../common/protocol.hpp"
#include "../common/unique_fd.hpp"
#include "net.hpp"

#include <fcntl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>

#include <algorithm>
#include <stdexcept>
#include <vector>

namespace {
constexpr std::size_t BUF_SIZE = 256 * 1024;

UniqueFd openFile(const std::string& path, std::uint64_t& size) {
    UniqueFd file(::open(path.c_str(), O_RDONLY));
    if (!file.valid()) throwErrno("не удалось открыть '" + path + "'");

    struct stat st {};
    if (::fstat(file.get(), &st) != 0) throwErrno("не удалось получить размер файла");
    if (!S_ISREG(st.st_mode)) throw std::runtime_error("'" + path + "' не является обычным файлом");

    size = static_cast<std::uint64_t>(st.st_size);
    return file;
}

std::string baseName(const std::string& path) {
    std::string name = path;
    while (name.size() > 1 && name.back() == '/') name.pop_back();
    std::size_t slash = name.find_last_of('/');
    if (slash != std::string::npos) name.erase(0, slash + 1);
    return name;
}

void sendHeader(int sock, const std::vector<unsigned char>& header) {
    sendAll(sock, header.data(), header.size());
}

void sendBody(int sock, int file, std::uint64_t size) {
    std::vector<char> buf(BUF_SIZE);
    std::uint64_t sent = 0;
    while (sent < size) {
        std::size_t want = std::min<std::uint64_t>(buf.size(), size - sent);
        ssize_t n = read(file, buf.data(), want);
        if (n < 0) {
            if (errno == EINTR) continue;
            throwErrno("ошибка чтения файла");
        }
        if (n == 0) throw std::runtime_error("файл уменьшился во время передачи");
        sendAll(sock, buf.data(), static_cast<std::size_t>(n));
        sent += static_cast<std::uint64_t>(n);
    }
}
}

FileSender::FileSender(std::string host, std::uint16_t port)
    : host_(std::move(host)), port_(port) {}

SendResult FileSender::send(const std::string& path) const noexcept {
    SendResult result;
    try {
        transfer(path, result);
    } catch (const std::exception& e) {
        result.ok = false;
        result.error = e.what();
    } catch (...) {
        result.ok = false;
        result.error = "неизвестная ошибка";
    }
    return result;
}

void FileSender::transfer(const std::string& path, SendResult& result) const {
    proto::Header header;
    UniqueFd file = openFile(path, header.size);
    header.name = localToUtf8(baseName(path));
    std::vector<unsigned char> encoded = proto::encodeHeader(header);
    result.name = header.name;
    result.size = header.size;

    UniqueFd sock = connectTo(host_, port_);

    std::string sendError;
    try {
        sendHeader(sock.get(), encoded);
        sendBody(sock.get(), file.get(), header.size);
    } catch (const std::exception& e) {
        sendError = e.what();
    }
    file.reset();

    ::shutdown(sock.get(), SHUT_WR);
    std::uint8_t status = proto::STATUS_FAIL;
    bool gotStatus = recvStatus(sock.get(), status);

    if (gotStatus && status != proto::STATUS_OK)
        throw std::runtime_error("сервер сообщил об ошибке" +
                                 (sendError.empty() ? std::string() : " (" + sendError + ")"));
    if (!sendError.empty()) throw std::runtime_error(sendError);
    if (!gotStatus) throw std::runtime_error("сервер закрыл соединение, не прислав ответ");
    result.ok = true;
}