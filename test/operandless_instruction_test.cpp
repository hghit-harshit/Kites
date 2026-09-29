#include <gtest/gtest.h>

#include <memory>
#include <sstream>
#include <string>

#include "assembler/assembler.h"
#include "processor/rvss/rvss_processor.h"
#include "utils/utils.h"

namespace
{

std::unique_ptr<Kites::RVSSProcessor> runSource(const std::string &source)
{
    Kites::setupVmStateDirectory();

    auto vm = std::make_unique<Kites::RVSSProcessor>();
    std::istringstream stream(source);
    Kites::AssembledProgram program = Kites::assemble(stream);
    vm->LoadProgram(program);
    vm->DebugRun();
    return vm;
}

} // namespace

// The lexer used to treat the first word of the line after an operand-less
// instruction as that instruction's label operand, so this failed to assemble.
TEST(OperandlessInstruction, NopFollowedByInstruction)
{
    auto vm = runSource("nop\n"
                        "addi x5, x0, 7\n"
                        "nop\n"
                        "addi x6, x5, 1\n");
    EXPECT_EQ(vm->registers_.ReadGpr(5), 7u);
    EXPECT_EQ(vm->registers_.ReadGpr(6), 8u);
}

TEST(OperandlessInstruction, RetFollowedByInstruction)
{
    auto vm = runSource("jal x1, func\n"
                        "addi x5, x5, 100\n"
                        "j end\n"
                        "func:\n"
                        "addi x5, x0, 1\n"
                        "ret\n"
                        "addi x5, x0, 999\n" // never reached; must still assemble
                        "end:\n"
                        "nop\n");
    EXPECT_EQ(vm->registers_.ReadGpr(5), 101u);
}

// A mnemonic in operand position on the same line is still a label reference.
TEST(OperandlessInstruction, MnemonicAsLabelOperandStillWorks)
{
    auto vm = runSource("j add\n"
                        "addi x5, x0, 1\n"
                        "add:\n"
                        "addi x6, x0, 2\n");
    EXPECT_EQ(vm->registers_.ReadGpr(5), 0u);
    EXPECT_EQ(vm->registers_.ReadGpr(6), 2u);
}
