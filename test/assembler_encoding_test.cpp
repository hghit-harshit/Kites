/**
 * @file assembler_encoding_test.cpp
 * @brief Machine-code encodings for RV64 W instructions, system instructions
 *        (ecall/ebreak/fence/fence.i), fclass/fmv, and shift-amount ranges.
 *
 * Expected encodings were derived by hand from the RISC-V ISA manual and match
 * what GNU as produces for the same source.
 */

#include <gtest/gtest.h>

#include <cstdint>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "assembler/assembler.h"
#include "common/instructions.h"

namespace
{

Kites::AssembledProgram assembleSource(const std::string &source)
{
    std::istringstream stream(source);
    return Kites::assemble(stream);
}

uint32_t encodeOne(const std::string &line)
{
    auto program = assembleSource(line + "\n");
    EXPECT_EQ(program.text_buffer.size(), 1u) << line;
    return program.text_buffer.empty() ? 0u : program.text_buffer[0];
}

struct EncodingCase
{
    const char *source;
    uint32_t expected;
};

void expectEncodings(const std::vector<EncodingCase> &cases)
{
    for (const auto &c : cases)
    {
        SCOPED_TRACE(c.source);
        EXPECT_EQ(encodeOne(c.source), c.expected);
    }
}

} // namespace

// ---------------------------------------------------------------------------
// RV64 W instructions
// ---------------------------------------------------------------------------

TEST(AssemblerEncoding, WInstructions)
{
    expectEncodings({
        {"addw a0, a1, a2", 0x00C5853Bu},
        {"subw a0, a1, a2", 0x40C5853Bu},
        {"sllw a0, a1, a2", 0x00C5953Bu},
        {"srlw a0, a1, a2", 0x00C5D53Bu},
        {"sraw a0, a1, a2", 0x40C5D53Bu},
        {"addiw a0, a1, -5", 0xFFB5851Bu},
        {"slliw a0, a1, 31", 0x01F5951Bu},
        {"srliw a0, a1, 31", 0x01F5D51Bu},
        {"sraiw a0, a1, 31", 0x41F5D51Bu},
    });
}

TEST(AssemblerEncoding, WInstructionsAcceptRegisterAliases)
{
    // t0 = x5, s1 = x9, a7 = x17
    EXPECT_EQ(encodeOne("addw t0, s1, a7"), 0x011482BBu);
    EXPECT_EQ(encodeOne("addw x5, x9, x17"), 0x011482BBu);
}

TEST(AssemblerEncoding, AddiwImmediateBoundaries)
{
    EXPECT_EQ(encodeOne("addiw a0, a1, 2047"), 0x7FF5851Bu);
    EXPECT_EQ(encodeOne("addiw a0, a1, -2048"), 0x8005851Bu);
    EXPECT_THROW(assembleSource("addiw a0, a1, 2048\n"), std::runtime_error);
    EXPECT_THROW(assembleSource("addiw a0, a1, -2049\n"), std::runtime_error);
}

TEST(AssemblerEncoding, WordShiftAmountIsFiveBits)
{
    EXPECT_EQ(encodeOne("slliw a0, a1, 0"), 0x0005951Bu);
    for (const char *op : {"slliw", "srliw", "sraiw"})
    {
        SCOPED_TRACE(op);
        EXPECT_NO_THROW(assembleSource(std::string(op) + " a0, a1, 31\n"));
        EXPECT_THROW(assembleSource(std::string(op) + " a0, a1, 32\n"), std::runtime_error);
        EXPECT_THROW(assembleSource(std::string(op) + " a0, a1, 63\n"), std::runtime_error);
    }
}

TEST(AssemblerEncoding, DoublewordShiftAmountIsSixBits)
{
    EXPECT_EQ(encodeOne("slli a0, a1, 63"), 0x03F59513u);
    EXPECT_EQ(encodeOne("srai a0, a1, 63"), 0x43F5D513u); // funct6 010000 + shamt[5]
    EXPECT_THROW(assembleSource("slli a0, a1, 64\n"), std::runtime_error);
}

// ---------------------------------------------------------------------------
// System instructions
// ---------------------------------------------------------------------------

TEST(AssemblerEncoding, SystemInstructions)
{
    expectEncodings({
        {"ecall", 0x00000073u},
        {"ebreak", 0x00100073u},
        {"fence", 0x0FF0000Fu},
        {"fence.i", 0x0000100Fu},
    });
}

TEST(AssemblerEncoding, OldFenceSpellingIsRejected)
{
    EXPECT_ANY_THROW(assembleSource("fence_i\n"));
}

TEST(AssemblerEncoding, EcallAndEbreakDisassembleBack)
{
    using Kites::instruction_set::disassemble;
    EXPECT_NE(disassemble(encodeOne("ecall")).find("ecall"), std::string::npos);
    EXPECT_NE(disassemble(encodeOne("ebreak")).find("ebreak"), std::string::npos);
}

TEST(AssemblerEncoding, SystemInstructionsMixWithOthers)
{
    auto program =
        assembleSource("addi a0, x0, 1\nfence\necall\nebreak\nfence.i\naddi a0, a0, 1\n");
    ASSERT_EQ(program.text_buffer.size(), 6u);
    EXPECT_EQ(program.text_buffer[1], 0x0FF0000Fu);
    EXPECT_EQ(program.text_buffer[2], 0x00000073u);
    EXPECT_EQ(program.text_buffer[3], 0x00100073u);
    EXPECT_EQ(program.text_buffer[4], 0x0000100Fu);
}

// ---------------------------------------------------------------------------
// fclass and its neighbours in the same encoding group
// ---------------------------------------------------------------------------

TEST(AssemblerEncoding, Fclass)
{
    expectEncodings({
        {"fclass.s a0, fa0", 0xE0051553u},
        {"fclass.s t1, ft2", 0xE0011353u},
        {"fclass.d a1, fa1", 0xE20595D3u},
        {"fclass.d x31, f31", 0xE20F9FD3u},
    });
}

TEST(AssemblerEncoding, FmvStillEncodes)
{
    expectEncodings({
        {"fmv.x.w a0, fa1", 0xE0058553u},
        {"fmv.w.x fa0, a1", 0xF0058553u},
        {"fmv.x.d a0, fa1", 0xE2058553u},
        {"fmv.d.x fa0, a1", 0xF2058553u},
    });
}
