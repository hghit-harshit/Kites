/**
 * @file exit_syscall_test.cpp
 * @brief The exit syscall (a7 = 10) must stop the program cleanly without
 *        terminating the host process (it used to call exit(0), killing the GUI).
 */

#include <gtest/gtest.h>

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

// Instructions after the ecall must never run.
const std::string kExitProgram = "addi a1, x0, 0\n"
                                 "addi a7, x0, 10\n"
                                 "addi a0, x0, 3\n" +
                                 kPad +
                                 "ecall\n"
                                 "addi a1, a1, 1\n"
                                 "addi a1, a1, 1\n" +
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

} // namespace

TEST(ExitSyscall, SingleCycleStopsWithoutKillingProcess)
{
    // Reaching the assertions at all proves the process was not terminated.
    auto vm = runSource<Kites::RVSSProcessor>(kExitProgram);
    EXPECT_EQ(vm->registers_.ReadGpr(11), 0u) << "instructions after ecall ran";
    EXPECT_EQ(vm->registers_.ReadGpr(10), 3u) << "exit code register changed";
    EXPECT_EQ(vm->output_status_, "VM_EXIT");
    EXPECT_TRUE(vm->IsStopRequested());
}

TEST(ExitSyscall, SingleCycleExitAsLastInstructionKeepsExitStatus)
{
    // The end-of-program check must not replace VM_EXIT with VM_PROGRAM_END.
    auto vm = runSource<Kites::RVSSProcessor>("addi a7, x0, 10\naddi a0, x0, 0\n" + kPad +
                                              "ecall\n");
    EXPECT_EQ(vm->output_status_, "VM_EXIT");
}

TEST(ExitSyscall, SingleCycleNormalEndStillReportsProgramEnd)
{
    auto vm = runSource<Kites::RVSSProcessor>("addi a0, x0, 1\n");
    EXPECT_EQ(vm->output_status_, "VM_PROGRAM_END");
}

TEST(ExitSyscall, SingleCycleCanRunAgainAfterExit)
{
    auto vm = runSource<Kites::RVSSProcessor>(kExitProgram);
    // A second program on a fresh VM in the same process must still work.
    auto second = runSource<Kites::RVSSProcessor>("addi a0, x0, 42\n");
    EXPECT_EQ(second->registers_.ReadGpr(10), 42u);
}

// The pipelines do not implement ecall yet (bug P9 in the audit plan). Enable
// these once handle_syscall() is wired into every 5-stage variant.
template <typename Vm>
class ExitSyscallPipeline : public ::testing::Test
{
};

using PipelineVms = ::testing::Types<Kites::RV5StageProcessorHF, Kites::RV5StageProcessorHNF,
                                     Kites::RV5StageProcessorNHF, Kites::RV5StageProcessorNHNF>;
TYPED_TEST_SUITE(ExitSyscallPipeline, PipelineVms);

TYPED_TEST(ExitSyscallPipeline, DISABLED_StopsWithoutRunningLaterInstructions)
{
    auto vm = runSource<TypeParam>(kExitProgram);
    EXPECT_EQ(vm->registers_.ReadGpr(11), 0u) << "instructions after ecall ran";
    EXPECT_EQ(vm->registers_.ReadGpr(10), 3u) << "exit code register changed";
}
