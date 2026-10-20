// src/base/FileDescriptor.cpp
#include "base/FileDescriptor.hpp"

namespace chat::base {

FileDescriptor::~FileDescriptor() {
    if (fd_ >= 0) ::close(fd_);
}

FileDescriptor::FileDescriptor(FileDescriptor&& other) noexcept : fd_(other.fd_) {
    other.fd_ = -1;
}

FileDescriptor& FileDescriptor::operator=(FileDescriptor&& other) noexcept {
    if (this != &other) {
        if (fd_ >= 0) ::close(fd_);
        fd_       = other.fd_;
        other.fd_ = -1;
    }
    return *this;
}

int FileDescriptor::release() noexcept {
    int tmp = fd_;
    fd_ = -1;
    return tmp;
}

void FileDescriptor::reset(int fd) noexcept {
    if (fd_ >= 0 && fd_ != fd) ::close(fd_);
    fd_ = fd;
}

void FileDescriptor::swap(FileDescriptor& other) noexcept {
    int tmp   = fd_;
    fd_       = other.fd_;
    other.fd_ = tmp;
}

} // namespace base