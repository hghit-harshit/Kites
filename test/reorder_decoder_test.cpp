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