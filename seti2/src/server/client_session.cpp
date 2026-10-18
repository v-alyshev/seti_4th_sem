#include "client_session.hpp"

#include "../common/errors.hpp"
#include "../common/units.hpp"
#include "file_name.hpp"
#include "log.hpp"
#include "upload_storage.hpp"

#include <unistd.h>

#include <algorithm>
#include <stdexcept>
#include <vector>

namespace {
constexpr std::size_t BUF_SIZE = 256 * 1024;

void writeAll(int fd, const char* data, std::size_t len) {
    while (len > 0) {
        ssize_t n = ::write(fd, data, len);
        if (n < 0) {
            if (errno == EINTR) continue;
            throwErrno("ошибка записи в файл");
        }
        data += n;
        len -= static_cast<std::size_t>(n);
    }
}
}

ClientSession::ClientSession(int fd, std::string peer, const UploadStorage& storage)
    : conn_(fd, peer), peer_(std::move(peer)), storage_(storage) {}

void ClientSession::run() noexcept {
    try {
        logLine("[" + peer_ + "] подключился");
        bool received = false;
        try {
            receiveFile();
            received = true;
        } catch (const std::exception& e) {
            logLine("[" + conn_.label() + "] ОШИБКА: " + e.what());
        }
        finish(received);
    } catch (...) {
        if (fileCreated_) storage_.remove(savedName_);
    }
}

void ClientSession::receiveFile() {
    proto::Header header = readHeader();
    size_ = header.size;

    UniqueFd file = storage_.createUnique(sanitizeFileName(header.name), savedName_);
    fileCreated_ = true;
    conn_.setFileName(savedName_);

    std::uint64_t received = receiveBody(file.get(), size_);
    if (file.close() != 0) throwErrno("ошибка закрытия файла");

    if (received != size_)
        throw std::runtime_error("размер не совпадает: заявлено " + std::to_string(size_) +
                                 " байт, получено " + std::to_string(received));
    expectEndOfData();
}

proto::Header ClientSession::readHeader() {
    unsigned char prefix[proto::PREFIX_SIZE];
    conn_.readExact(prefix, sizeof prefix);
    std::size_t nameLen = proto::decodePrefix(prefix);

    proto::Header h;
    h.name.resize(nameLen);
    conn_.readExact(&h.name[0], nameLen);
    proto::validateName(h.name);

    unsigned char sizeField[proto::SIZE_FIELD_SIZE];
    conn_.readExact(sizeField, sizeof sizeField);
    h.size = proto::decodeSize(sizeField);
    return h;
}

std::uint64_t ClientSession::receiveBody(int fileFd, std::uint64_t size) {
    std::vector<char> buf(BUF_SIZE);
    std::uint64_t received = 0;
    while (received < size) {
        std::size_t want = std::min<std::uint64_t>(buf.size(), size - received);
        std::size_t n = conn_.readSome(buf.data(), want);
        if (n == 0) break;
        writeAll(fileFd, buf.data(), n);
        received += n;
    }
    return received;
}

void ClientSession::expectEndOfData() {
    char extra;
    if (conn_.readSome(&extra, 1) != 0)
        throw std::runtime_error("размер не совпадает: клиент прислал больше заявленных " +
                                 std::to_string(size_) + " байт");
}

void ClientSession::finish(bool received) {
    conn_.finalReport();

    bool delivered = conn_.sendStatus(received ? proto::STATUS_OK : proto::STATUS_FAIL);
    bool ok = received && delivered;

    if (!ok && fileCreated_) storage_.remove(savedName_);

    if (ok)
        logLine("[" + conn_.label() + "] сохранён в " + storage_.dir() + "/" + savedName_ + " (" +
                formatSize(size_) + "), соединение закрыто");
    else if (received)
        logLine("[" + conn_.label() + "] ОШИБКА: не удалось отправить клиенту подтверждение, "
                "файл удалён, соединение закрыто");
    else
        logLine("[" + conn_.label() + "] передача не удалась, соединение закрыто");
}
