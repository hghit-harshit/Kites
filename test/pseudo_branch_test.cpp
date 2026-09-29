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

// Loads x1/x2, then runs `branch` against a forward label. x10 ends up 1 if the
// branch was taken and 2 if it fell through.
std::string forwardBranchProgram(int64_t a, int64_t b, const std::string &branch)
{
    return "li x1, " + std::to_string(a) + "\n" +
           "li x2, " + std::to_string(b) + "\n" +
           branch + " taken\n"
           "li x10, 2\n"
           "j end\n"
           "taken:\n"
           "li x10, 1\n"
           "end:\n"
           "addi x0, x0, 0\n";
}

template <typename VmType>
void expectTaken(int64_t a, int64_t b, const std::string &branch, bool taken)
{
    auto vm = runSource<VmType>(forwardBranchProgram(a, b, branch));
    EXPECT_EQ(vm->registers_.ReadGpr(10), taken ? 1u : 2u) << branch << " a=" << a << " b=" << b;
}

// Pseudo branches used to leave their immediate empty, so code generation threw
// std::invalid_argument from std::stoi. Each case checks the branch assembles and
// that its operand order is right in both the taken and fall-through direction.
template <typename VmType>
void runPseudoBranchMatrix()
{
    expectTaken<VmType>(0, 0, "beqz x1,", true);
    expectTaken<VmType>(5, 0, "beqz x1,", false);

    expectTaken<VmType>(5, 0, "bnez x1,", true);
    expectTaken<VmType>(0, 0, "bnez x1,", false);

    expectTaken<VmType>(0, 0, "blez x1,", true);
    expectTaken<VmType>(-3, 0, "blez x1,", true);
    expectTaken<VmType>(3, 0, "blez x1,", false);

    expectTaken<VmType>(0, 0, "bgez x1,", true);
    expectTaken<VmType>(3, 0, "bgez x1,", true);
    expectTaken<VmType>(-3, 0, "bgez x1,", false);

    expectTaken<VmType>(-3, 0, "bltz x1,", true);
    expectTaken<VmType>(0, 0, "bltz x1,", false);

    expectTaken<VmType>(3, 0, "bgtz x1,", true);
    expectTaken<VmType>(0, 0, "bgtz x1,", false);

    expectTaken<VmType>(5, 3, "bgt x1, x2,", true);
    expectTaken<VmType>(3, 3, "bgt x1, x2,", false);
    expectTaken<VmType>(-1, 1, "bgt x1, x2,", false);

    expectTaken<VmType>(3, 3, "ble x1, x2,", true);
    expectTaken<VmType>(-1, 1, "ble x1, x2,", true);
    expectTaken<VmType>(5, 3, "ble x1, x2,", false);

    // -1 is the largest unsigned value, so the unsigned forms flip the signed result.
    expectTaken<VmType>(-1, 1, "bgtu x1, x2,", true);
    expectTaken<VmType>(3, 3, "bgtu x1, x2,", false);

    expectTaken<VmType>(1, -1, "bleu x1, x2,", true);
    expectTaken<VmType>(3, 3, "bleu x1, x2,", true);
    expectTaken<VmType>(-1, 1, "bleu x1, x2,", false);
}

// A pseudo branch to a label defined earlier in the program (negative offset).
template <typename VmType>
void runBackwardPseudoBranch()
{
    auto vm = runSource<VmType>("li x1, 5\n"
                                "li x2, 0\n"
                                "loop:\n"
                                "addi x2, x2, 1\n"
                                "addi x1, x1, -1\n"
                                "bgtz x1, loop\n");
    EXPECT_EQ(vm->registers_.ReadGpr(1), 0u);
    EXPECT_EQ(vm->registers_.ReadGpr(2), 5u);
}

} // namespace

TEST(PseudoBranch, SingleCycle)
{
    runPseudoBranchMatrix<Kites::RVSSProcessor>();
}

TEST(PseudoBranch, FiveStageHazardForwarding)
{
    runPseudoBranchMatrix<Kites::RV5StageProcessorHF>();
}

TEST(PseudoBranch, BackwardLabelSingleCycle)
{
    runBackwardPseudoBranch<Kites::RVSSProcessor>();
}

TEST(PseudoBranch, BackwardLabelFiveStageHazardForwarding)
{
    runBackwardPseudoBranch<Kites::RV5StageProcessorHF>();
}
