#include "msgbuffer/MsgBufferPool.hpp"

#include <gtest/gtest.h>
#include <thread>
#include <vector>

using dispatcher::ExhaustionPolicy;
using dispatcher::MsgBufferPool;
using dispatcher::PoolExhaustedException;

TEST(MsgBufferPoolTest, InitialisesWithCorrectCount) {
    MsgBufferPool pool(4);
    EXPECT_EQ(pool.poolSize(), 4u);
    EXPECT_EQ(pool.available(), 4u);
}

TEST(MsgBufferPoolTest, AcquireDecreasesAvailable) {
    MsgBufferPool pool(2);
    auto buf = pool.acquire();
    EXPECT_EQ(pool.available(), 1u);
}

TEST(MsgBufferPoolTest, ReleaseReturnsBufferToPool) {
    MsgBufferPool pool(1);
    {
        auto buf = pool.acquire();
        EXPECT_EQ(pool.available(), 0u);
    } // buf released here
    EXPECT_EQ(pool.available(), 1u);
}

TEST(MsgBufferPoolTest, AcquiredBufferHasCorrectCapacity) {
    MsgBufferPool pool(1, 2048);
    auto buf = pool.acquire();
    EXPECT_EQ(buf->capacity(), 2048u);
}

TEST(MsgBufferPoolTest, ReleasedBufferIsCleared) {
    MsgBufferPool pool(1);
    {
        auto buf = pool.acquire();
        buf->setSize(100);
    }
    auto buf2 = pool.acquire();
    EXPECT_EQ(buf2->size(), 0u);
}

TEST(MsgBufferPoolTest, ThrowPolicyThrowsOnExhaustion) {
    MsgBufferPool pool(1, 4096, ExhaustionPolicy::Throw);
    auto buf = pool.acquire();
    EXPECT_THROW(pool.acquire(), PoolExhaustedException);
}

TEST(MsgBufferPoolTest, AllocatePolicyCreatesOverflowBuffer) {
    MsgBufferPool pool(1, 4096, ExhaustionPolicy::Allocate);
    auto buf1 = pool.acquire();
    auto buf2 = pool.acquire(); // should not throw
    EXPECT_NE(buf2, nullptr);
    EXPECT_EQ(buf2->capacity(), 4096u);
}

TEST(MsgBufferPoolTest, OverflowBufferDoesNotReturnToPool) {
    MsgBufferPool pool(1, 4096, ExhaustionPolicy::Allocate);
    auto buf1 = pool.acquire();
    {
        auto overflow = pool.acquire();
    } // overflow released — pool should still have 0 available (buf1 still held)
    EXPECT_EQ(pool.available(), 0u);
    buf1.reset();
    EXPECT_EQ(pool.available(), 1u); // only the original pool buffer returns
}

TEST(MsgBufferPoolTest, InvalidCapacityThrows) {
    EXPECT_THROW(MsgBufferPool(4, 3000), std::invalid_argument);
}

TEST(MsgBufferPoolTest, ThreadSafeAcquireAndRelease) {
    MsgBufferPool pool(8);
    std::vector<std::thread> threads;

    for (int i = 0; i < 8; ++i) {
        threads.emplace_back([&pool] {
            auto buf = pool.acquire();
            buf->setSize(10);
        });
    }
    for (auto& t : threads) t.join();

    EXPECT_EQ(pool.available(), 8u);
}
