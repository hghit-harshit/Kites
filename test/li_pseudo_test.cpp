/**
 * @file li_pseudo_test.cpp
 * @brief The `li` pseudo-instruction: expansion shape (assembler only) and the
 *        value it actually loads on every processor model that handles hazards.
 *
 * Covers the 12-bit, 32-bit (including the INT32_MAX rounding edge where
 * lui must be followed by addiw, not addi) and 64-bit expansion paths.
 */

#include <gtest/gtest.h>

#include <cctype>
#include <cstdint>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "assembler/assembler.h"
#include "processor/rv5s/rv5s_processor_h_f.h"
#include "processor/rv5s/rv5s_processor_h_nf.h"
#include "processor/rv5s/rv5s_processor_nh_f.h"
#include "processor/rvss/rvss_processor.h"
#include "utils/utils.h"

namespace
{

struct LiCase
{
    const char *literal; ///< Literal as written after `li a0,`.
    uint64_t expected;   ///< Register value the expansion must produce.
};

const std::vector<LiCase> kLiCases = {
    {"0", 0x0ull},
    {"1", 0x1ull},
    {"-1", 0xFFFFFFFFFFFFFFFFull},
    {"2047", 0x7FFull},
    {"2048", 0x800ull},
    {"-2048", 0xFFFFFFFFFFFFF800ull},
    {"-2049", 0xFFFFFFFFFFFFF7FFull},
    {"0x7FFFF7FF", 0x7FFFF7FFull},
    {"0x7FFFF800", 0x7FFFF800ull},
    {"0x7FFFFFFF", 0x7FFFFFFFull},
    {"-0x80000000", 0xFFFFFFFF80000000ull},
    {"0x80000000", 0x80000000ull},
    {"0xFFFFFFFF", 0xFFFFFFFFull},
    {"0x100000000", 0x100000000ull},
    {"0x7FFFFFFF000", 0x7FFFFFFF000ull},
    {"0x123456789ABCDEF0", 0x123456789ABCDEF0ull},
    {"-0x12345678", 0xFFFFFFFFEDCBA988ull},
    {"9223372036854775807", 0x7FFFFFFFFFFFFFFFull},
    {"-9223372036854775808", 0x8000000000000000ull},
};

Kites::AssembledProgram assembleSource(const std::string &source)
{
    std::istringstream stream(source);
    return Kites::assemble(stream);
}

std::string liSource(const LiCase &c)
{
    return std::string("li a0, ") + c.literal + "\n";
}

// ---------------------------------------------------------------------------
// Expansion shape
// ---------------------------------------------------------------------------

class LiExpansionShape : public ::testing::TestWithParam<LiCase>
{
};

TEST_P(LiExpansionShape, UsesTheRightInstructions)
{
    const LiCase &c = GetParam();
    auto program = assembleSource(liSource(c));

    std::vector<std::string> opcodes;
    for (const auto &[block, resolved] : program.intermediate_code)
    {
        opcodes.push_back(block.getOpcode());
    }
    ASSERT_FALSE(opcodes.empty());
    EXPECT_LE(opcodes.size(), 8u) << "li should never need more than 8 instructions";
    EXPECT_EQ(program.text_buffer.size(), opcodes.size());

    const auto value = static_cast<int64_t>(c.expected);
    if (value >= -2048 && value <= 2047)
    {
        ASSERT_EQ(opcodes.size(), 1u);
        EXPECT_EQ(opcodes[0], "addi");
    }
    else if (value == static_cast<int64_t>(static_cast<int32_t>(value)))
    {
        EXPECT_EQ(opcodes[0], "lui");
        EXPECT_LE(opcodes.size(), 2u);
    }

    // On RV64 a lui must be followed by addiw: a 64-bit addi leaves the upper
    // word set for values near INT32_MAX.
    for (size_t i = 0; i + 1 < opcodes.size(); ++i)
    {
        if (opcodes[i] == "lui")
        {
            EXPECT_EQ(opcodes[i + 1], "addiw") << "instruction " << i + 1;
        }
    }
}

std::string liCaseName(const ::testing::TestParamInfo<LiCase> &info)
{
    std::string name = info.param.literal;
    for (char &ch : name)
    {
        if (!std::isalnum(static_cast<unsigned char>(ch)))
        {
            ch = (ch == '-') ? 'm' : '_';
        }
    }
    return "v" + std::to_string(info.index) + "_" + name;
}

INSTANTIATE_TEST_SUITE_P(AllValues, LiExpansionShape, ::testing::ValuesIn(kLiCases),
                         liCaseName);

// ---------------------------------------------------------------------------
// Loaded value, per processor model. The no-hazard/no-forwarding pipeline is
// left out: li emits back-to-back dependent instructions, which that model
// cannot run correctly by design.
// ---------------------------------------------------------------------------

template <typename Vm>
class LiLoadedValue : public ::testing::Test
{
};

using HazardSafeVms = ::testing::Types<Kites::RVSSProcessor, Kites::RV5StageProcessorHF,
                                       Kites::RV5StageProcessorHNF, Kites::RV5StageProcessorNHF>;
TYPED_TEST_SUITE(LiLoadedValue, HazardSafeVms);

TYPED_TEST(LiLoadedValue, EveryValueLoadsExactly)
{
    for (const LiCase &c : kLiCases)
    {
        SCOPED_TRACE(liSource(c));
        Kites::setupVmStateDirectory();
        auto vm = std::make_unique<TypeParam>();
        vm->LoadProgram(assembleSource(liSource(c)));
        vm->DebugRun();
        EXPECT_EQ(vm->registers_.ReadGpr(10), c.expected);
    }
}

TYPED_TEST(LiLoadedValue, ConsecutiveLisDoNotInterfere)
{
    Kites::setupVmStateDirectory();
    auto vm = std::make_unique<TypeParam>();
    vm->LoadProgram(assembleSource("li a0, 0x7FFFFFFF\n"
                                   "li a1, 0x123456789ABCDEF0\n"
                                   "li a2, -2049\n"
                                   "li a3, 0x7FFFF800\n"));
    vm->DebugRun();
    EXPECT_EQ(vm->registers_.ReadGpr(10), 0x7FFFFFFFull);
    EXPECT_EQ(vm->registers_.ReadGpr(11), 0x123456789ABCDEF0ull);
    EXPECT_EQ(vm->registers_.ReadGpr(12), 0xFFFFFFFFFFFFF7FFull);
    EXPECT_EQ(vm->registers_.ReadGpr(13), 0x7FFFF800ull);
}

} // namespace
