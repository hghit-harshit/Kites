/**
 * @file fp_forwarding_test.cpp
 * @brief Back-to-back dependencies between integer and floating-point
 *        instructions on the pipelines: xN and fN must never be confused, FP
 *        instructions that read a GPR must get forwarded values, stores and
 *        FP->integer ops must not look like FP-register writers, and f0 must be
 *        forwarded like any other FP register. Also covers JALR's offset
 *        surviving forwarding.
 *
 * Setup values are separated from the instruction pair under test by nops, so
 * only the pair itself exercises forwarding/hazard handling.
 */

#include <gtest/gtest.h>

#include <cstdint>
#include <cstring>
#include <memory>
#include <sstream>
#include <string>

#include "assembler/assembler.h"
#include "processor/rv5s/rv5s_processor_h_f.h"
#include "processor/rv5s/rv5s_processor_h_nf.h"
#include "processor/rv5s/rv5s_processor_nh_f.h"
#include "processor/rv5s/rv5s_processor_nh_nf.h"
#include "processor/rvss/rvss_processor.h"
#include "utils/utils.h"

namespace
{

const std::string kPad = "addi x0, x0, 0\naddi x0, x0, 0\naddi x0, x0, 0\n";

// Data section base (lui s0, 0x10000) and a few single-precision constants.
const std::string kFloatData = ".data\n"
                               "v: .float 1.5, 2.5, 1.0, 2.0, 5.0\n" // offsets 0,4,8,12,16
                               ".text\n"
                               "lui s0, 0x10000\n" +
                               kPad;

template <typename Vm>
std::unique_ptr<Vm> runSource(const std::string &source)
{
    Kites::setupVmStateDirectory();
    auto vm = std::make_unique<Vm>();
    std::istringstream stream(source);
    vm->LoadProgram(Kites::assemble(stream));
    vm->DebugRun();
    return vm;
}

uint32_t floatBits(float f)
{
    uint32_t bits;
    std::memcpy(&bits, &f, sizeof(bits));
    return bits;
}

uint32_t lowWord(uint64_t v)
{
    return static_cast<uint32_t>(v);
}

// Models that resolve every hazard (single cycle, stall+forward, stall-only).
template <typename Vm>
class FpHazardSafe : public ::testing::Test
{
};
using HazardSafeVms = ::testing::Types<Kites::RVSSProcessor, Kites::RV5StageProcessorHF,
                                       Kites::RV5StageProcessorHNF>;
TYPED_TEST_SUITE(FpHazardSafe, HazardSafeVms);

// Also includes forwarding without hazard detection: fine for ALU->ALU pairs
// (no load-use).
template <typename Vm>
class FpAluForwarding : public ::testing::Test
{
};
using AluForwardingVms = ::testing::Types<Kites::RVSSProcessor, Kites::RV5StageProcessorHF,
                                          Kites::RV5StageProcessorHNF,
                                          Kites::RV5StageProcessorNHF>;
TYPED_TEST_SUITE(FpAluForwarding, AluForwardingVms);

// All five models; programs fully padded.
template <typename Vm>
class AllModels : public ::testing::Test
{
};
using AllVms = ::testing::Types<Kites::RVSSProcessor, Kites::RV5StageProcessorHF,
                                Kites::RV5StageProcessorHNF, Kites::RV5StageProcessorNHF,
                                Kites::RV5StageProcessorNHNF>;
TYPED_TEST_SUITE(AllModels, AllVms);

} // namespace

// flw writes fa0 (f10); the next addi reads a0 (x10). The integer add must see
// the integer register, and the following flw must use the forwarded base.
TYPED_TEST(FpHazardSafe, FloatLoadDoesNotClobberSameNumberedGpr)
{
    auto vm = runSource<TypeParam>(kFloatData + "addi a0, s0, 0\n" + kPad +
                                   "flw fa0, 0(a0)\n"
                                   "addi a0, a0, 4\n"  // x10, not f10
                                   "flw fa1, 0(a0)\n"  // base forwarded from the addi
                                   "fadd.s fa2, fa0, fa1\n"
                                   "fcvt.w.s a1, fa2\n" +
                                   kPad);
    EXPECT_EQ(vm->registers_.ReadGpr(10), 0x10000004ull) << "a0 got FP load data";
    EXPECT_EQ(lowWord(vm->registers_.ReadFpr(11)), floatBits(2.5f)) << "flw used a stale base";
    EXPECT_EQ(vm->registers_.ReadGpr(11), 4u) << "1.5 + 2.5";
}

// An FP result must not be forwarded into an integer instruction that reads the
// same-numbered GPR.
TYPED_TEST(FpAluForwarding, FpResultDoesNotReachSameNumberedGpr)
{
    auto vm = runSource<TypeParam>(kFloatData + "flw fa0, 0(s0)\nflw fa1, 4(s0)\n"
                                                "addi t0, x0, 10\naddi t1, x0, 20\n" +
                                   kPad +
                                   "fadd.s f5, fa0, fa1\n" // f5
                                   "add t2, t0, t1\n" +    // x5 + x6
                                   kPad);
    EXPECT_EQ(vm->registers_.ReadGpr(7), 30u);
    EXPECT_EQ(lowWord(vm->registers_.ReadFpr(5)), floatBits(4.0f));
}

// fmv.w.x / fcvt.s.w read a GPR in EX: it must be forwarded from the producer
// right before it.
TYPED_TEST(FpAluForwarding, GprProducerForwardsIntoFpInstruction)
{
    auto vm = runSource<TypeParam>(kPad +
                                   "lui t0, 0x40400\n"    // 3.0f bit pattern
                                   "fmv.w.x ft0, t0\n" +  // must see the lui result
                                   kPad +
                                   "addi t1, x0, 7\n"
                                   "fcvt.s.w ft1, t1\n" + // must see 7
                                   kPad + "fcvt.w.s a2, ft0\nfcvt.w.s a3, ft1\n" + kPad);
    EXPECT_EQ(vm->registers_.ReadGpr(12), 3u);
    EXPECT_EQ(vm->registers_.ReadGpr(13), 7u);
}

// fsw has rd field = imm[4:0]; it must not be treated as writing that FPR.
TYPED_TEST(FpAluForwarding, FloatStoreIsNotAnFpRegisterWriter)
{
    auto vm = runSource<TypeParam>(kFloatData + "flw ft0, 0(s0)\nflw f8, 8(s0)\nflw f9, 12(s0)\n" +
                                   kPad +
                                   "fsw ft0, 8(s0)\n"       // rd field = 8
                                   "fadd.s ft1, f8, f9\n" + // must read the real f8 (1.0)
                                   kPad + "fcvt.w.s a1, ft1\n" + kPad);
    EXPECT_EQ(vm->registers_.ReadGpr(11), 3u) << "fsw offset was forwarded as f8";
    EXPECT_EQ(vm->memory_controller_.readWord(0x10000008), floatBits(1.5f));
}

// feq writes x10; the next instruction reads f10. The 0/1 result must not be
// forwarded as an FP value.
TYPED_TEST(FpAluForwarding, FpCompareResultIsNotForwardedAsFpValue)
{
    auto vm = runSource<TypeParam>(kFloatData + "flw fa0, 12(s0)\nflw fa1, 16(s0)\nflw ft0, 0(s0)\n" +
                                   kPad +
                                   "feq.s a0, ft0, ft0\n"   // x10 = 1
                                   "fadd.s f3, fa0, fa1\n" + // f10 + f11 = 2 + 5
                                   kPad + "fcvt.w.s a1, f3\n" + kPad);
    EXPECT_EQ(vm->registers_.ReadGpr(10), 1u);
    EXPECT_EQ(vm->registers_.ReadGpr(11), 7u);
}

// f0 is an ordinary FP register and must be forwarded.
TYPED_TEST(FpAluForwarding, FZeroIsForwarded)
{
    auto vm = runSource<TypeParam>(kFloatData + "flw f1, 8(s0)\nflw f2, 12(s0)\n" + kPad +
                                   "fadd.s f0, f1, f2\n"   // 3.0
                                   "fmul.s f3, f0, f0\n" + // 9.0
                                   kPad + "fcvt.w.s a1, f3\n" + kPad);
    EXPECT_EQ(vm->registers_.ReadGpr(11), 9u);
}

// JALR's offset 40 has imm[4:0] = 8 = s0. A write to s0 right before it must
// not be forwarded in place of the offset.
TYPED_TEST(AllModels, JalrOffsetSurvivesForwarding)
{
    auto vm = runSource<TypeParam>("addi a1, x0, 0\n" + kPad + // 0..12
                                   "auipc t0, 0\n" + kPad +    // 16: t0 = 16; pad 20..28
                                   "addi s0, x0, 7\n"          // 32
                                   "jalr ra, 40(t0)\n"         // 36: target 56, ra = 40
                                   "addi a1, a1, 100\n"        // 40 (skipped)
                                   "addi a1, a1, 100\n"        // 44 (skipped)
                                   "addi a1, a1, 100\n"        // 48 (skipped)
                                   "addi a1, a1, 100\n"        // 52 (skipped)
                                   "addi a1, a1, 1\n" + kPad); // 56
    EXPECT_EQ(vm->registers_.ReadGpr(11), 1u);
    EXPECT_EQ(vm->registers_.ReadGpr(1), 40u);
}
