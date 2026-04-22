#pragma once

#include <cstddef>
#include <memory>

namespace dispatcher {

class MsgBuffer {
public:
    explicit MsgBuffer(std::size_t capacity = 4096);
    ~MsgBuffer() = default;

    MsgBuffer(MsgBuffer&&) noexcept = default;
    MsgBuffer& operator=(MsgBuffer&&) noexcept = default;
    MsgBuffer(const MsgBuffer&) = delete;
    MsgBuffer& operator=(const MsgBuffer&) = delete;

    char*       data() noexcept;
    const char* data() const noexcept;
    std::size_t capacity() const noexcept;
    std::size_t size() const noexcept;

    // Call after a read to record how many bytes were written into the buffer.
    void setSize(std::size_t n);

    // Reset the written-byte count; does not zero the memory.
    void clear() noexcept;

private:
    std::unique_ptr<char[]> data_;
    std::size_t             capacity_;
    std::size_t             size_{0};
};

} // namespace dispatcher
