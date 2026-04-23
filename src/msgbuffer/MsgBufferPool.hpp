#pragma once

#include "MsgBuffer.hpp"

#include <cstddef>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <vector>

namespace dispatcher {

enum class ExhaustionPolicy {
    Throw,    // Throw PoolExhaustedException when the pool is empty.
    Allocate  // Allocate a new buffer outside the pool; it is freed on release.
};

class PoolExhaustedException : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

class MsgBufferPool {
public:
    // bufferCapacity must be a power of 2.
    explicit MsgBufferPool(std::size_t     poolSize,
                           std::size_t     bufferCapacity = 4096,
                           ExhaustionPolicy policy        = ExhaustionPolicy::Throw);

    ~MsgBufferPool() = default;

    MsgBufferPool(const MsgBufferPool&) = delete;
    MsgBufferPool& operator=(const MsgBufferPool&) = delete;

    // Returns a buffer with a custom deleter that returns it to this pool.
    // Overflow buffers (Allocate policy) are simply freed on release.
    // Thread-safe.
    std::shared_ptr<MsgBuffer> acquire();

    std::size_t available() const;
    std::size_t poolSize()  const noexcept;

private:
    void release(MsgBuffer* buf);

    mutable std::mutex               mutex_;
    std::vector<std::unique_ptr<MsgBuffer>> pool_;
    const std::size_t                poolSize_;
    const std::size_t                bufferCapacity_;
    const ExhaustionPolicy           policy_;
};

} // namespace dispatcher
