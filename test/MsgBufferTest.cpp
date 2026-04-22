#include "msgbuffer/MsgBuffer.hpp"

#include <gtest/gtest.h>

using dispatcher::MsgBuffer;

TEST(MsgBufferTest, DefaultCapacityIs4096) {
    MsgBuffer buf;
    EXPECT_EQ(buf.capacity(), 4096u);
    EXPECT_EQ(buf.size(), 0u);
}

TEST(MsgBufferTest, CustomCapacity) {
    MsgBuffer buf(1024);
    EXPECT_EQ(buf.capacity(), 1024u);
}

TEST(MsgBufferTest, DataPointerIsNonNull) {
    MsgBuffer buf;
    EXPECT_NE(buf.data(), nullptr);
}

TEST(MsgBufferTest, SetSizeUpdatesSize) {
    MsgBuffer buf;
    buf.setSize(128);
    EXPECT_EQ(buf.size(), 128u);
}

TEST(MsgBufferTest, SetSizeThrowsOnOverflow) {
    MsgBuffer buf(512);
    EXPECT_THROW(buf.setSize(513), std::out_of_range);
}

TEST(MsgBufferTest, ClearResetsSize) {
    MsgBuffer buf;
    buf.setSize(100);
    buf.clear();
    EXPECT_EQ(buf.size(), 0u);
}

TEST(MsgBufferTest, DataIsWritableAndReadable) {
    MsgBuffer buf(64);
    buf.data()[0] = 'A';
    buf.data()[1] = 'B';
    EXPECT_EQ(buf.data()[0], 'A');
    EXPECT_EQ(buf.data()[1], 'B');
}

TEST(MsgBufferTest, MoveConstruct) {
    MsgBuffer a(256);
    a.setSize(10);
    MsgBuffer b(std::move(a));
    EXPECT_EQ(b.capacity(), 256u);
    EXPECT_EQ(b.size(), 10u);
}

TEST(MsgBufferTest, MoveAssign) {
    MsgBuffer a(256);
    a.setSize(10);
    MsgBuffer b(128);
    b = std::move(a);
    EXPECT_EQ(b.capacity(), 256u);
    EXPECT_EQ(b.size(), 10u);
}
