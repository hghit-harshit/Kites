#include <gtest/gtest.h>
#include "reorder/decoded_inst.h"

using namespace Kites::reorder;

TEST(ReorderDecoder, Load)
{
    auto d = decode(0x00052283);
    EXPECT_TRUE(d.is_load);
    EXPECT_EQ(d.rd, 5);
    EXPECT_EQ(d.rs1, 10);
    EXPECT_TRUE(d.reads_rs1);
    EXPECT_FALSE(d.reads_rs2);
}

TEST(ReorderDecoder, AddiDoesNotReadRs2)
{
    auto d = decode(0x00500313);
    EXPECT_EQ(d.rs2, 5);          // raw bits look like x5
    EXPECT_FALSE(d.reads_rs2);    // but it is really the immediate
}

TEST(ReorderDecoder, Store)
{
    auto d = decode(0x0020A023);          // sw x2,0(x1)
    EXPECT_TRUE(d.is_store);
    EXPECT_EQ(d.rs1, 1);
    EXPECT_EQ(d.rs2, 2);
    EXPECT_TRUE(d.reads_rs2);
    EXPECT_FALSE(d.writes_rd);
}

TEST(ReorderDecoder, Ecall)
{
    auto d = decode(0x00000073);
    EXPECT_EQ(d.kind, Kind::Ecall);
    EXPECT_TRUE(d.is_barrier);
}

TEST(ReorderDecoder, WForms)
{
    auto a = decode(0x0052833B);          // addw x6,x5,x5
    EXPECT_EQ(a.kind, Kind::Op);
    EXPECT_TRUE(a.reads_rs2);
    EXPECT_FALSE(a.is_barrier);

    auto b = decode(0x0050031B);          // addiw x6,x0,5
    EXPECT_EQ(b.kind, Kind::OpImm);
    EXPECT_FALSE(b.reads_rs2);
    EXPECT_EQ(b.rs1, 0);
}