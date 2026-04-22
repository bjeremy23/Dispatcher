#include "MsgBuffer.hpp"

#include <stdexcept>

namespace dispatcher {

MsgBuffer::MsgBuffer(std::size_t capacity)
    : data_(std::make_unique<char[]>(capacity))
    , capacity_(capacity)
    , size_(0)
{}

char* MsgBuffer::data() noexcept {
    return data_.get();
}

const char* MsgBuffer::data() const noexcept {
    return data_.get();
}

std::size_t MsgBuffer::capacity() const noexcept {
    return capacity_;
}

std::size_t MsgBuffer::size() const noexcept {
    return size_;
}

void MsgBuffer::setSize(std::size_t n) {
    if (n > capacity_) {
        throw std::out_of_range("MsgBuffer::setSize: n exceeds capacity");
    }
    size_ = n;
}

void MsgBuffer::clear() noexcept {
    size_ = 0;
}

} // namespace dispatcher
