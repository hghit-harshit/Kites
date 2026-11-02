#include <gtest/gtest.h>
#include "processor/registers.h"

TEST(RegisterFileVector, RegistersInitializeToZero)
{
    Kites::RegisterFile registers;

    for (size_t reg = 0; reg < Kites::RegisterFile::NUM_VREG; ++reg)
    {
        EXPECT_EQ(registers.ReadVreg(reg),
                  (Kites::RegisterFile::VectorRegister{0, 0, 0, 0}));
    }
}

TEST(RegisterFileVector, WriteAndReadAllLanes)
{
    Kites::RegisterFile registers;
    const Kites::RegisterFile::VectorRegister value{
        0x12345678u, 0xABCDEF01u, 0u, 0xFFFFFFFFu};

    registers.WriteVreg(3, value);

    EXPECT_EQ(registers.ReadVreg(3), value);
}

TEST(RegisterFileVector, RegistersAreIndependent)
{
    Kites::RegisterFile registers;

    registers.WriteVreg(1, {1, 2, 3, 4});
    registers.WriteVreg(2, {5, 6, 7, 8});

    EXPECT_EQ(registers.ReadVreg(1),
              (Kites::RegisterFile::VectorRegister{1, 2, 3, 4}));
    EXPECT_EQ(registers.ReadVreg(2),
              (Kites::RegisterFile::VectorRegister{5, 6, 7, 8}));
}

TEST(RegisterFileVector, ResetClearsVectorRegisters)
{
    Kites::RegisterFile registers;
    registers.WriteVreg(4, {10, 20, 30, 40});

    registers.Reset();

    EXPECT_EQ(registers.ReadVreg(4),
              (Kites::RegisterFile::VectorRegister{0, 0, 0, 0}));
}

TEST(RegisterFileVector, InvalidIndexThrows)
{
    Kites::RegisterFile registers;

    EXPECT_THROW(registers.ReadVreg(Kites::RegisterFile::NUM_VREG),
                 std::out_of_range);
    EXPECT_THROW(registers.WriteVreg(Kites::RegisterFile::NUM_VREG, {1, 2, 3, 4}),
                 std::out_of_range);
}