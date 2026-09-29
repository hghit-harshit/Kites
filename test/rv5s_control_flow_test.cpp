#include <gtest/gtest.h>

#include <memory>
#include <sstream>
#include <string>
#include <type_traits>

#include "assembler/assembler.h"
#include "processor/rv5s/rv5s_processor_h_f.h"
#include "processor/rv5s/rv5s_processor_h_nf.h"
#include "processor/rv5s/rv5s_processor_nh_f.h"
#include "processor/rv5s/rv5s_processor_nh_nf.h"
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

// Dependent instructions are padded with nops so the programs are hazard-free
// and give the same answer on the variants without hazard detection/forwarding.
constexpr const char *kPad = "addi x0, x0, 0\n"
                             "addi x0, x0, 0\n"
                             "addi x0, x0, 0\n";

// Pins the pipeline's redirect timing: a taken branch (resolved in MEM) leaves a
// 3-cycle bubble and a jump (resolved in EX) a 2-cycle bubble. A duplicated target
// fetch keeps the same cycle count only by executing the target twice, so this also
// catches duplicates the register values can hide (e.g. on the no-forwarding VMs).
template <typename VmType>
void expectPipelineCycles(const VmType &vm, unsigned int expected)
{
    if constexpr (!std::is_same_v<VmType, Kites::RVSSProcessor>)
    {
        EXPECT_EQ(vm.cycle_s_, expected);
    }
}

// The pipeline used to fetch a redirect target twice, running it twice. Every
// target below is a non-idempotent increment so a duplicate shows up in the result.
template <typename VmType>
void runBackwardBranchLoop()
{
    auto vm = runSource<VmType>(std::string("li x1, 5\n"
                                            "li x2, 0\n") +
                                kPad +
                                "loop:\n"
                                "addi x2, x2, 1\n"
                                "addi x1, x1, -1\n" +
                                kPad +
                                "bne x1, x0, loop\n");
    EXPECT_EQ(vm->registers_.ReadGpr(1), 0u);
    EXPECT_EQ(vm->registers_.ReadGpr(2), 5u);
    // 35 instructions + 4 drain cycles + 4 taken branches x 3 bubbles.
    expectPipelineCycles(*vm, 51);
}

template <typename VmType>
void runCallAndReturn()
{
    auto vm = runSource<VmType>(std::string("li x5, 0\n") + kPad +
                                "jal x1, func\n"
                                "addi x5, x5, 100\n"
                                "j end\n"
                                "func:\n"
                                "addi x5, x5, 1\n" +
                                kPad +
                                "jalr x0, 0(x1)\n"
                                "end:\n"
                                "addi x0, x0, 0\n");
    EXPECT_EQ(vm->registers_.ReadGpr(5), 101u);
    // 13 instructions + 4 drain cycles + 3 jumps x 2 bubbles.
    expectPipelineCycles(*vm, 23);
}

template <typename VmType>
void runControlFlowSuite()
{
    runBackwardBranchLoop<VmType>();
    runCallAndReturn<VmType>();
}

} // namespace

TEST(RV5SControlFlow, SingleCycle)
{
    runControlFlowSuite<Kites::RVSSProcessor>();
}

TEST(RV5SControlFlow, HazardForwarding)
{
    runControlFlowSuite<Kites::RV5StageProcessorHF>();
}

TEST(RV5SControlFlow, HazardNoForwarding)
{
    runControlFlowSuite<Kites::RV5StageProcessorHNF>();
}

TEST(RV5SControlFlow, NoHazardForwarding)
{
    runControlFlowSuite<Kites::RV5StageProcessorNHF>();
}

TEST(RV5SControlFlow, NoHazardNoForwarding)
{
    runControlFlowSuite<Kites::RV5StageProcessorNHNF>();
}
