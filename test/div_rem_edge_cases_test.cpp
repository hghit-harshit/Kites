#include <gtest/gtest.h>

#include <memory>
#include <sstream>
#include <string>

#include "assembler/assembler.h"
#include "processor/rv5s/rv5s_processor_h_f.h"
#include "processor/rvss/rvss_processor.h"
#include "utils/utils.h"

namespace
{

// Runs `source` to completion on VmType and hands back the finished VM so the
// caller can inspect the register file.
template <typename VmType>
std::unique_ptr<VmType> runSource(const std::string &source)
{
    Kites::setupVmStateDirectory();

    auto vm = std::make_unique<VmType>();
    std::istringstream stream(source);
    Kites::AssembledProgram program = Kites::assemble(stream);
    vm->LoadProgram(program);
    vm->DebugRun();
    return vm;
}

// INT64_MIN and INT32_MIN are built by shifting rather than by `li` with a wide
// literal, so these tests exercise div/rem semantics and not the assembler's
// immediate range handling.
constexpr const char *kLoadInt64Min = "li x1, 1\n slli x1, x1, 63\n";
constexpr const char *kLoadInt32Min = "li x1, 1\n slli x1, x1, 31\n";

template <typename VmType>
void expectDivRem(const std::string &source, uint64_t expectedQuotient,
                  uint64_t expectedRemainder)
{
    auto vm = runSource<VmType>(source);
    EXPECT_EQ(vm->registers_.ReadGpr(3), expectedQuotient);
    EXPECT_EQ(vm->registers_.ReadGpr(4), expectedRemainder);
}

// ---------------------------------------------------------------------------
// The shared edge-case matrix, run against every VM under test.
//
// RISC-V defines these results rather than trapping: divide-by-zero yields an
// all-ones quotient and a remainder equal to the dividend, and the signed
// overflow case (INT_MIN / -1) yields the dividend with a zero remainder.
// ---------------------------------------------------------------------------

template <typename VmType>
void runDivRemMatrix()
{
    // 1. DIV / REM, positive dividend, divisor zero.
    expectDivRem<VmType>("li x1, 100\n"
                         "li x2, 0\n"
                         "div x3, x1, x2\n"
                         "rem x4, x1, x2\n",
                         0xFFFFFFFFFFFFFFFFULL, 0x0000000000000064ULL);

    // 2. DIV / REM, negative dividend, divisor zero.
    expectDivRem<VmType>("li x1, -100\n"
                         "li x2, 0\n"
                         "div x3, x1, x2\n"
                         "rem x4, x1, x2\n",
                         0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFF9CULL);

    // 3. DIVU / REMU, divisor zero.
    expectDivRem<VmType>("li x1, 100\n"
                         "li x2, 0\n"
                         "divu x3, x1, x2\n"
                         "remu x4, x1, x2\n",
                         0xFFFFFFFFFFFFFFFFULL, 0x0000000000000064ULL);

    // 4. DIVU / REMU, dividend with every bit set, divisor zero.
    expectDivRem<VmType>("li x1, -1\n"
                         "li x2, 0\n"
                         "divu x3, x1, x2\n"
                         "remu x4, x1, x2\n",
                         0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFFFFULL);

    // 5. Signed overflow: INT64_MIN / -1 returns the dividend, remainder zero.
    expectDivRem<VmType>(std::string(kLoadInt64Min) +
                             "li x2, -1\n"
                             "div x3, x1, x2\n"
                             "rem x4, x1, x2\n",
                         0x8000000000000000ULL, 0x0000000000000000ULL);

    // 6. DIVU / REMU, ordinary operands.
    expectDivRem<VmType>("li x1, 100\n"
                         "li x2, 7\n"
                         "divu x3, x1, x2\n"
                         "remu x4, x1, x2\n",
                         14ULL, 2ULL);

    // 7. DIV / REM, ordinary operands, negative dividend (truncation toward zero).
    expectDivRem<VmType>("li x1, -100\n"
                         "li x2, 7\n"
                         "div x3, x1, x2\n"
                         "rem x4, x1, x2\n",
                         0xFFFFFFFFFFFFFFF2ULL, 0xFFFFFFFFFFFFFFFEULL);

    // 8. DIVW / REMW, divisor zero.
    expectDivRem<VmType>("li x1, 100\n"
                         "li x2, 0\n"
                         "divw x3, x1, x2\n"
                         "remw x4, x1, x2\n",
                         0xFFFFFFFFFFFFFFFFULL, 0x0000000000000064ULL);

    // 9. DIVUW / REMUW, divisor zero.
    expectDivRem<VmType>("li x1, 100\n"
                         "li x2, 0\n"
                         "divuw x3, x1, x2\n"
                         "remuw x4, x1, x2\n",
                         0xFFFFFFFFFFFFFFFFULL, 0x0000000000000064ULL);

    // 10. REMW returns the dividend's low 32 bits, sign-extended.
    expectDivRem<VmType>("li x1, -100\n"
                         "li x2, 0\n"
                         "divw x3, x1, x2\n"
                         "remw x4, x1, x2\n",
                         0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFF9CULL);

    // 11. REMUW also sign-extends its 32-bit result, so a dividend whose low
    //     word has bit 31 set comes back sign-extended, not zero-extended.
    expectDivRem<VmType>("li x1, -100\n"
                         "li x2, 0\n"
                         "divuw x3, x1, x2\n"
                         "remuw x4, x1, x2\n",
                         0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFF9CULL);

    // 12. Signed word overflow: INT32_MIN / -1 returns the 32-bit dividend,
    //     sign-extended, with a zero remainder.
    expectDivRem<VmType>(std::string(kLoadInt32Min) +
                             "li x2, -1\n"
                             "divw x3, x1, x2\n"
                             "remw x4, x1, x2\n",
                         0xFFFFFFFF80000000ULL, 0x0000000000000000ULL);

    // 13. DIVW / REMW use the low 32 bits of each operand, so upper bits in the
    //     dividend must not leak into the result.
    expectDivRem<VmType>("li x1, -100\n"
                         "li x2, 7\n"
                         "divw x3, x1, x2\n"
                         "remw x4, x1, x2\n",
                         0xFFFFFFFFFFFFFFF2ULL, 0xFFFFFFFFFFFFFFFEULL);

    // 14. REMU on a full 64-bit dividend, guarding against the operands being
    //     truncated to 32 bits by a mis-decoded REMU.
    expectDivRem<VmType>(std::string(kLoadInt64Min) +
                             "li x2, 3\n"
                             "divu x3, x1, x2\n"
                             "remu x4, x1, x2\n",
                         0x2AAAAAAAAAAAAAAAULL, 0x0000000000000002ULL);
}

} // namespace

TEST(DivRemEdgeCases, SingleCycle)
{
    runDivRemMatrix<Kites::RVSSProcessor>();
}

TEST(DivRemEdgeCases, FiveStageHazardForwarding)
{
    runDivRemMatrix<Kites::RV5StageProcessorHF>();
}
