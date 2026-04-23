#include "MsgBufferPool.hpp"

#include <stdexcept>

namespace dispatcher {

static bool isPowerOfTwo(std::size_t n) {
    return n > 0 && (n & (n - 1)) == 0;
}

MsgBufferPool::MsgBufferPool(std::size_t poolSize,
                             std::size_t bufferCapacity,
                             ExhaustionPolicy policy)
    : poolSize_(poolSize)
    , bufferCapacity_(bufferCapacity)
    , policy_(policy)
{
    if (!isPowerOfTwo(bufferCapacity)) {
        throw std::invalid_argument("MsgBufferPool: bufferCapacity must be a power of 2");
    }

    pool_.reserve(poolSize);
    for (std::size_t i = 0; i < poolSize; ++i) {
        pool_.push_back(std::make_unique<MsgBuffer>(bufferCapacity));
    }
}

std::shared_ptr<MsgBuffer> MsgBufferPool::acquire() {
    std::unique_lock lock(mutex_);

    if (!pool_.empty()) {
        auto buf = std::move(pool_.back());
        pool_.pop_back();
        lock.unlock();

        auto* raw = buf.release();
        return std::shared_ptr<MsgBuffer>(raw, [this](MsgBuffer* b) {
            release(b);
        });
    }

    if (policy_ == ExhaustionPolicy::Allocate) {
        lock.unlock();
        // Default deleter — overflow buffers are freed, not returned to the pool.
        return std::make_shared<MsgBuffer>(bufferCapacity_);
    }

    throw PoolExhaustedException("MsgBufferPool: pool exhausted");
}

void MsgBufferPool::release(MsgBuffer* buf) {
    buf->clear();
    std::lock_guard lock(mutex_);
    pool_.push_back(std::unique_ptr<MsgBuffer>(buf));
}

std::size_t MsgBufferPool::available() const {
    std::lock_guard lock(mutex_);
    return pool_.size();
}

std::size_t MsgBufferPool::poolSize() const noexcept {
    return poolSize_;
}

} // namespace dispatcher
