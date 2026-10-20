// src/base/FileDescriptor.hpp
#pragma once
#include <unistd.h>

namespace chat::base {

class FileDescriptor {
public:
    explicit FileDescriptor(int fd = -1) noexcept : fd_(fd) {}
    ~FileDescriptor();                                        // 定义在 .cpp

    FileDescriptor(const FileDescriptor&)            = delete;
    FileDescriptor& operator=(const FileDescriptor&) = delete;

    FileDescriptor(FileDescriptor&& other) noexcept;          // 定义在 .cpp
    FileDescriptor& operator=(FileDescriptor&& other) noexcept;

    // 小访问器内联
    int  get()   const noexcept { return fd_; }
    bool valid() const noexcept { return fd_ >= 0; }
    explicit operator bool() const noexcept { return fd_ >= 0; }

    int  release() noexcept;                                  // 定义在 .cpp
    void reset(int fd = -1) noexcept;                         // 定义在 .cpp
    void swap(FileDescriptor& other) noexcept;                // 定义在 .cpp

private:
    int fd_;
};

} // namespace base