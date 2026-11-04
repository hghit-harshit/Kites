/**
 * @file rv64i_execution_test.cpp
 * @brief RV64I instruction semantics on every processor model: W-instructions
 *        (32-bit ops with sign extension), 64-bit shifts, AUIPC / la, and JALR.
 *
 * Dependent instructions are separated by three nops so the programs give the
 * same answer on the pipeline without hazard detection or forwarding.
 */

#include <gtest/gtest.h>

#include <cstdint>
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

/// False for the model that cannot run back-to-back dependent instructions
/// (pseudo-instructions such as `la` expand into such pairs).
template <typename Vm>
constexpr bool kHandlesHazards = true;
template <>
constexpr bool kHandlesHazards<Kites::RV5StageProcessorNHNF> = false;

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

template <typename Vm>
class Rv64iExecution : public ::testing::Test
{
};

using AllVms = ::testing::Types<Kites::RVSSProcessor, Kites::RV5StageProcessorHF,
                                Kites::RV5StageProcessorHNF, Kites::RV5StageProcessorNHF,
                                Kites::RV5StageProcessorNHNF>;
TYPED_TEST_SUITE(Rv64iExecution, AllVms);

// ---------------------------------------------------------------------------
// W instructions
// ---------------------------------------------------------------------------

TYPED_TEST(Rv64iExecution, WInstructionsBasic)
{
    auto vm = runSource<TypeParam>("addi a1, x0, 5\n" + kPad +
                                   "addiw a0, a1, 7\n"  // 12
                                   "slliw a2, a1, 30\n" // low word 0x40000000, positive
                                   "sraiw a3, a1, 1\n" + // 2
                                   kPad + "addw a4, a0, a3\n" + kPad);
    EXPECT_EQ(vm->registers_.ReadGpr(10), 12u);
    EXPECT_EQ(vm->registers_.ReadGpr(12), 0x40000000u);
    EXPECT_EQ(vm->registers_.ReadGpr(13), 2u);
    EXPECT_EQ(vm->registers_.ReadGpr(14), 14u);
}

TYPED_TEST(Rv64iExecution, WInstructionsWrapAndSignExtend)
{
    auto vm = runSource<TypeParam>("lui a1, 0x80000\n" + kPad +   // 0xFFFFFFFF80000000
                                   "addi a1, a1, -1\n"            // 0xFFFFFFFF7FFFFFFF
                                   "addi a4, x0, 1\n"
                                   "addi a6, x0, -16\n"
                                   "addi t0, x0, 33\n"
                                   "addi t1, x0, 1\n"
                                   "lui s3, 0x80000\n" + kPad +   // 0xFFFFFFFF80000000
                                   "addiw a2, a1, 1\n"            // wraps to 0x80000000
                                   "addiw a3, a1, 0\n"            // 0x7FFFFFFF
                                   "slli a4, a4, 32\n"            // 0x100000000
                                   "srliw a7, a6, 4\n"            // 0x0FFFFFFF
                                   "sraiw s1, a6, 4\n"            // -1
                                   "sllw t2, t1, t0\n"            // shift uses rs2[4:0] = 1
                                   "srlw s8, a6, t0\n"            // 0xFFFFFFF0 >> 1 = 0x7FFFFFF8
                                   "sraw s9, a6, t0\n"            // -8
                                   "subw s4, x0, s3\n"            // 0 - 0x80000000 wraps
                                   "sext.w s5, a1\n"              // 0x7FFFFFFF
                                   "negw s6, a6\n" + kPad +       // 16
                                   "addi a4, a4, 5\n" + kPad +    // 0x100000005
                                   "addiw a5, a4, 0\n"            // upper word ignored -> 5
                                   "addw s7, a4, a4\n" + kPad);   // 10
    auto &r = vm->registers_;
    EXPECT_EQ(r.ReadGpr(12), 0xFFFFFFFF80000000ull) << "addiw overflow";
    EXPECT_EQ(r.ReadGpr(13), 0x7FFFFFFFull) << "addiw 0 truncates";
    EXPECT_EQ(r.ReadGpr(17), 0x0FFFFFFFull) << "srliw";
    EXPECT_EQ(r.ReadGpr(9), 0xFFFFFFFFFFFFFFFFull) << "sraiw";
    EXPECT_EQ(r.ReadGpr(7), 2u) << "sllw masks the shift amount";
    EXPECT_EQ(r.ReadGpr(24), 0x7FFFFFF8ull) << "srlw";
    EXPECT_EQ(r.ReadGpr(25), 0xFFFFFFFFFFFFFFF8ull) << "sraw";
    EXPECT_EQ(r.ReadGpr(20), 0xFFFFFFFF80000000ull) << "subw";
    EXPECT_EQ(r.ReadGpr(21), 0x7FFFFFFFull) << "sext.w";
    EXPECT_EQ(r.ReadGpr(22), 16u) << "negw";
    EXPECT_EQ(r.ReadGpr(15), 5u) << "addiw ignores the upper word";
    EXPECT_EQ(r.ReadGpr(23), 10u) << "addw ignores the upper word";
}

// ---------------------------------------------------------------------------
// 64-bit shifts (shamt[5] lives in bit 25)
// ---------------------------------------------------------------------------

TYPED_TEST(Rv64iExecution, SixtyFourBitShifts)
{
    auto vm = runSource<TypeParam>("addi a0, x0, -3\n" + kPad +
                                   "slli a0, a0, 48\n" + kPad + // 0xFFFD000000000000
                                   "srai a1, a0, 31\n"
                                   "srai a2, a0, 32\n"
                                   "srai a3, a0, 40\n"
                                   "srai a4, a0, 63\n"
                                   "srli a5, a0, 40\n"
                                   "srai a6, a0, 0\n"
                                   "srli a7, a0, 63\n" +
                                   kPad);
    auto &r = vm->registers_;
    EXPECT_EQ(r.ReadGpr(10), 0xFFFD000000000000ull);
    EXPECT_EQ(r.ReadGpr(11), 0xFFFFFFFFFFFA0000ull) << "srai 31";
    EXPECT_EQ(r.ReadGpr(12), 0xFFFFFFFFFFFD0000ull) << "srai 32";
    EXPECT_EQ(r.ReadGpr(13), 0xFFFFFFFFFFFFFD00ull) << "srai 40";
    EXPECT_EQ(r.ReadGpr(14), 0xFFFFFFFFFFFFFFFFull) << "srai 63";
    EXPECT_EQ(r.ReadGpr(15), 0x0000000000FFFD00ull) << "srli 40 zero-fills";
    EXPECT_EQ(r.ReadGpr(16), 0xFFFD000000000000ull) << "srai 0";
    EXPECT_EQ(r.ReadGpr(17), 1u) << "srli 63";
}

// ---------------------------------------------------------------------------
// AUIPC and la
// ---------------------------------------------------------------------------

TYPED_TEST(Rv64iExecution, AuipcAtSeveralPcs)
{
    auto vm = runSource<TypeParam>("auipc a0, 0\n" + kPad +       // pc 0
                                   "auipc a1, 0xFFFFF\n" + kPad + // pc 16: 16 - 4096
                                   "auipc a2, 0x80000\n"          // pc 32: sign-extended
                                   "auipc a3, 1\n" +              // pc 36
                                   kPad);
    auto &r = vm->registers_;
    EXPECT_EQ(r.ReadGpr(10), 0u);
    EXPECT_EQ(r.ReadGpr(11), static_cast<uint64_t>(16 - 4096));
    EXPECT_EQ(r.ReadGpr(12), 32u + 0xFFFFFFFF80000000ull);
    EXPECT_EQ(r.ReadGpr(13), 36u + 0x1000u);
}

TYPED_TEST(Rv64iExecution, LaLoadsFromSeveralDataLabels)
{
    if constexpr (!kHandlesHazards<TypeParam>)
    {
        GTEST_SKIP() << "la expands to a dependent auipc/addi pair";
    }
    else
    {
        auto vm = runSource<TypeParam>(".data\n"
                                       "w: .word 0x11223344\n"
                                       "gap: .word 0\n"
                                       "d: .dword 0x1122334455667788\n"
                                       ".text\n" +
                                       kPad + "la a0, w\n" + kPad + "lw a1, 0(a0)\n" +
                                       "la a2, d\n" + kPad + "ld a3, 0(a2)\n" +
                                       "la a4, gap\n" + kPad + "sub a5, a2, a0\n" +
                                       "sub a6, a4, a0\n" + kPad);
        auto &r = vm->registers_;
        EXPECT_EQ(r.ReadGpr(11), 0x11223344ull);
        EXPECT_EQ(r.ReadGpr(13), 0x1122334455667788ull);
        EXPECT_EQ(r.ReadGpr(15), 8u) << "d is 8 bytes after w";
        EXPECT_EQ(r.ReadGpr(16), 4u) << "gap is 4 bytes after w";
    }
}

// ---------------------------------------------------------------------------
// JALR
// ---------------------------------------------------------------------------

TYPED_TEST(Rv64iExecution, JalrClearsBitZeroOfTarget)
{
    auto vm = runSource<TypeParam>("addi a1, x0, 0\n" + kPad + // 0..12
                                   "auipc t0, 0\n" + kPad +    // 16: t0 = 16
                                   "jalr x0, 45(t0)\n" +       // 32: 61 -> 60
                                   kPad +                      // 36..44
                                   "addi a1, a1, 100\n"        // 48 (skipped)
                                   "addi a1, a1, 100\n"        // 52 (skipped)
                                   "addi a1, a1, 100\n"        // 56 (skipped)
                                   "addi a1, a1, 1\n" + kPad); // 60
    EXPECT_EQ(vm->registers_.ReadGpr(11), 1u);
    EXPECT_EQ(vm->registers_.ReadGpr(0), 0u) << "jalr x0 must not write x0";
}

TYPED_TEST(Rv64iExecution, JalrNegativeImmediateAndLinkRegister)
{
    auto vm = runSource<TypeParam>("addi a1, x0, 0\n" + kPad +  // 0..12
                                   "auipc t0, 0\n" + kPad +     // 16: t0 = 16
                                   "addi t0, t0, 48\n" + kPad + // 32: t0 = 64
                                   "jalr ra, -4(t0)\n"          // 48: target 60, ra = 52
                                   "addi a1, a1, 100\n"         // 52 (skipped)
                                   "addi a1, a1, 100\n"         // 56 (skipped)
                                   "addi a1, a1, 1\n" + kPad);  // 60
    EXPECT_EQ(vm->registers_.ReadGpr(11), 1u);
    EXPECT_EQ(vm->registers_.ReadGpr(1), 52u) << "ra = pc of jalr + 4";
}

} // namespace
