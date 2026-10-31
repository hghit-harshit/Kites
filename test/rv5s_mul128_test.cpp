#include <gtest/gtest.h>

#include <memory>
#include <sstream>
#include <string>

#include "assembler/assembler.h"
#include "processor/rv5s/rv5s_processor_h_f.h"
#include "utils/utils.h"

using namespace Kites;

namespace
{
// Assemble `source` and run it to completion on the hazard-detection + forwarding pipeline.
std::unique_ptr<RV5StageProcessorHF> runProgram(const std::string &source)
{
    setupVmStateDirectory();
    auto vm = std::make_unique<RV5StageProcessorHF>();
    std::istringstream in(source);
    AssembledProgram program = assemble(in);
    vm->LoadProgram(program);
    vm->DebugRun();
    return vm;
}

uint64_t reg(const std::unique_ptr<RV5StageProcessorHF> &vm, unsigned idx)
{
    return vm->registers_.ReadGpr(idx);
}

constexpr uint64_t kAllOnes = 0xFFFFFFFFFFFFFFFFULL;
} // namespace

// The exact program from the bug report.
TEST(RV5StageVM_H_F_MUL128, basic_6_times_7)
{
    auto vm = runProgram(".text\n"
                         "main:\n"
                         "    addi x12, x0, 6\n"
                         "    addi x13, x0, 7\n"
                         "    mul128 x10, x12, x13\n"
                         "    ecall\n");
    EXPECT_EQ(reg(vm, 12), 6u);
    EXPECT_EQ(reg(vm, 13), 7u);
    EXPECT_EQ(reg(vm, 10), 42u);
    EXPECT_EQ(reg(vm, 11), 0u);
}

TEST(RV5StageVM_H_F_MUL128, nonzero_high_half)
{
    // (2^64-1) * 2 = 2^65 - 2  -> low = 0xFF..FE, high = 1
    auto vm = runProgram(".text\n"
                         "    addi x12, x0, -1\n"
                         "    addi x13, x0, 2\n"
                         "    mul128 x10, x12, x13\n"
                         "    nop\nnop\nnop\nnop\n");
    EXPECT_EQ(reg(vm, 10), kAllOnes - 1);
    EXPECT_EQ(reg(vm, 11), 1u);
}

TEST(RV5StageVM_H_F_MUL128, max_times_max)
{
    // (2^64-1)^2 = 2^128 - 2^65 + 1 -> low = 1, high = 0xFF..FE
    auto vm = runProgram(".text\n"
                         "    addi x12, x0, -1\n"
                         "    addi x13, x0, -1\n"
                         "    mul128 x10, x12, x13\n"
                         "    nop\nnop\nnop\nnop\n");
    EXPECT_EQ(reg(vm, 10), 1u);
    EXPECT_EQ(reg(vm, 11), kAllOnes - 1);
}

TEST(RV5StageVM_H_F_MUL128, operands_are_unsigned)
{
    // x12 = 2^63 (via shift), x13 = 2 -> product 2^64: low = 0, high = 1
    auto vm = runProgram(".text\n"
                         "    addi x12, x0, 1\n"
                         "    slli x12, x12, 63\n"
                         "    addi x13, x0, 2\n"
                         "    mul128 x10, x12, x13\n"
                         "    nop\nnop\nnop\nnop\n");
    EXPECT_EQ(reg(vm, 10), 0u);
    EXPECT_EQ(reg(vm, 11), 1u);
}

TEST(RV5StageVM_H_F_MUL128, dependent_reads_of_both_halves)
{
    // Consumers placed 1, 2 and 3 instructions after the MUL128 (forwarding / stall / RF read).
    auto vm = runProgram(".text\n"
                         "    addi x12, x0, -1\n"
                         "    addi x13, x0, 2\n"
                         "    mul128 x10, x12, x13\n"
                         "    add x20, x10, x0\n"   // low, immediately after
                         "    add x21, x11, x0\n"   // high, 1 apart
                         "    add x22, x10, x11\n"  // 2 apart
                         "    add x23, x11, x0\n"   // 3 apart
                         "    nop\nnop\nnop\nnop\n");
    EXPECT_EQ(reg(vm, 20), kAllOnes - 1);
    EXPECT_EQ(reg(vm, 21), 1u);
    EXPECT_EQ(reg(vm, 22), kAllOnes);
    EXPECT_EQ(reg(vm, 23), 1u);
}

TEST(RV5StageVM_H_F_MUL128, dependent_operands_from_addi_back_to_back)
{
    // Sources written by the instructions directly before the MUL128 (no padding).
    auto vm = runProgram(".text\n"
                         "    addi x12, x0, 9\n"
                         "    addi x13, x0, 11\n"
                         "    mul128 x10, x12, x13\n"
                         "    nop\nnop\nnop\nnop\n");
    EXPECT_EQ(reg(vm, 10), 99u);
    EXPECT_EQ(reg(vm, 11), 0u);
}

TEST(RV5StageVM_H_F_MUL128, store_of_high_half)
{
    auto vm = runProgram(".text\n"
                         "    addi x12, x0, -1\n"
                         "    addi x13, x0, 2\n"
                         "    mul128 x10, x12, x13\n"
                         "    sd x11, 0(x0)\n"
                         "    sd x10, 8(x0)\n"
                         "    ld x20, 0(x0)\n"
                         "    ld x21, 8(x0)\n"
                         "    nop\nnop\nnop\nnop\n");
    EXPECT_EQ(reg(vm, 20), 1u);
    EXPECT_EQ(reg(vm, 21), kAllOnes - 1);
}

TEST(RV5StageVM_H_F_MUL128, overlap_rd_equals_rs1)
{
    auto vm = runProgram(".text\n"
                         "    addi x12, x0, 6\n"
                         "    addi x13, x0, 7\n"
                         "    mul128 x12, x12, x13\n"
                         "    nop\nnop\nnop\nnop\n");
    EXPECT_EQ(reg(vm, 12), 42u);
    EXPECT_EQ(reg(vm, 13), 0u); // high half overwrites rs2
}

TEST(RV5StageVM_H_F_MUL128, overlap_rs2_equals_rd_plus_1)
{
    auto vm = runProgram(".text\n"
                         "    addi x12, x0, 6\n"
                         "    addi x13, x0, 7\n"
                         "    mul128 x10, x12, x13\n"
                         "    mul128 x12, x12, x12\n" // rd == rs1 == rs2
                         "    nop\nnop\nnop\nnop\n");
    EXPECT_EQ(reg(vm, 12), 36u);
    EXPECT_EQ(reg(vm, 13), 0u);
}

TEST(RV5StageVM_H_F_MUL128, overlap_sources_are_rd_pair)
{
    // rs1 = rd, rs2 = rd+1 : both sources are read before either half is written.
    auto vm = runProgram(".text\n"
                         "    addi x10, x0, 6\n"
                         "    addi x11, x0, 7\n"
                         "    mul128 x10, x10, x11\n"
                         "    nop\nnop\nnop\nnop\n");
    EXPECT_EQ(reg(vm, 10), 42u);
    EXPECT_EQ(reg(vm, 11), 0u);
}

TEST(RV5StageVM_H_F_MUL128, back_to_back_mul128)
{
    // second MUL128 consumes both halves of the first
    // 1: (2^64-1)*2 -> hi=1, lo=2^64-2 ; 2: lo * hi = (2^64-2)*1
    auto vm = runProgram(".text\n"
                         "    addi x12, x0, -1\n"
                         "    addi x13, x0, 2\n"
                         "    mul128 x10, x12, x13\n"
                         "    mul128 x14, x10, x11\n"
                         "    mul128 x16, x14, x15\n"
                         "    nop\nnop\nnop\nnop\n");
    EXPECT_EQ(reg(vm, 10), kAllOnes - 1);
    EXPECT_EQ(reg(vm, 11), 1u);
    EXPECT_EQ(reg(vm, 14), kAllOnes - 1);
    EXPECT_EQ(reg(vm, 15), 0u);
    EXPECT_EQ(reg(vm, 16), 0u);
    EXPECT_EQ(reg(vm, 17), 0u);
}

TEST(RV5StageVM_H_F_MUL128, back_to_back_independent)
{
    auto vm = runProgram(".text\n"
                         "    addi x12, x0, 3\n"
                         "    addi x13, x0, 4\n"
                         "    mul128 x2, x12, x13\n"
                         "    mul128 x4, x13, x13\n"
                         "    mul128 x30, x12, x12\n"
                         "    nop\nnop\nnop\nnop\n");
    EXPECT_EQ(reg(vm, 2), 12u);
    EXPECT_EQ(reg(vm, 3), 0u);
    EXPECT_EQ(reg(vm, 4), 16u);
    EXPECT_EQ(reg(vm, 5), 0u);
    EXPECT_EQ(reg(vm, 30), 9u);
    EXPECT_EQ(reg(vm, 31), 0u);
}

TEST(RV5StageVM_H_F_MUL128, invalid_destination_pairs_write_nothing)
{
    // x0 (rd < 2), odd rd (x11), and x31 (rd+1 would be out of range): no architectural effect.
    auto vm = runProgram(".text\n"
                         "    addi x12, x0, 6\n"
                         "    addi x13, x0, 7\n"
                         "    addi x31, x0, 5\n"
                         "    addi x11, x0, 77\n"
                         "    mul128 x0, x12, x13\n"
                         "    mul128 x11, x12, x13\n"
                         "    mul128 x31, x12, x13\n"
                         "    nop\nnop\nnop\nnop\n");
    EXPECT_EQ(reg(vm, 0), 0u);
    EXPECT_EQ(reg(vm, 1), 0u);
    EXPECT_EQ(reg(vm, 11), 77u);
    EXPECT_EQ(reg(vm, 12), 6u);
    EXPECT_EQ(reg(vm, 13), 7u);
    EXPECT_EQ(reg(vm, 31), 5u);
}

TEST(RV5StageVM_H_F_MUL128, invalid_destination_does_not_disturb_following_code)
{
    auto vm = runProgram(".text\n"
                         "    addi x12, x0, 6\n"
                         "    addi x13, x0, 7\n"
                         "    mul128 x11, x12, x13\n"
                         "    addi x20, x0, 1\n"
                         "    add x21, x12, x13\n"
                         "    nop\nnop\nnop\nnop\n");
    EXPECT_EQ(reg(vm, 20), 1u);
    EXPECT_EQ(reg(vm, 21), 13u);
    EXPECT_EQ(reg(vm, 11), 0u);
}

// Ordinary instructions must be unaffected by the custom opcode support.
TEST(RV5StageVM_H_F_MUL128, ordinary_instructions_still_work)
{
    auto vm = runProgram(".text\n"
                         "    addi x5, x0, 10\n"
                         "    addi x6, x0, 3\n"
                         "    add x7, x5, x6\n"
                         "    sub x8, x5, x6\n"
                         "    mul x9, x5, x6\n"
                         "    slli x14, x5, 4\n"
                         "    lui x15, 1\n"
                         "    sd x7, 0(x0)\n"
                         "    ld x16, 0(x0)\n"
                         "    addi x17, x16, 1\n"
                         "    nop\nnop\nnop\nnop\n");
    EXPECT_EQ(reg(vm, 5), 10u);
    EXPECT_EQ(reg(vm, 6), 3u);
    EXPECT_EQ(reg(vm, 7), 13u);
    EXPECT_EQ(reg(vm, 8), 7u);
    EXPECT_EQ(reg(vm, 9), 30u);
    EXPECT_EQ(reg(vm, 14), 160u);
    EXPECT_EQ(reg(vm, 15), 4096u);
    EXPECT_EQ(reg(vm, 16), 13u);
    EXPECT_EQ(reg(vm, 17), 14u);
}

TEST(RV5StageVM_H_F_MUL128, invalid_destination_is_not_forwarded)
{
    // Odd rd: the low half must not be forwarded to an immediate reader of x11 either.
    auto vm = runProgram(".text\n"
                         "    addi x12, x0, 6\n"
                         "    addi x13, x0, 7\n"
                         "    addi x11, x0, 77\n"
                         "    mul128 x11, x12, x13\n"
                         "    add x20, x11, x0\n"
                         "    add x21, x11, x0\n"
                         "    nop\nnop\nnop\nnop\n");
    EXPECT_EQ(reg(vm, 20), 77u);
    EXPECT_EQ(reg(vm, 21), 77u);
    EXPECT_EQ(reg(vm, 11), 77u);
}
