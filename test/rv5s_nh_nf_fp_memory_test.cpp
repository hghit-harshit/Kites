#include <gtest/gtest.h>

#include "../include/assembler/assembler.h"
#include "../include/utils.h"
#include "../include/processor/rv5s/rv5s_processor_nh_nf.h"

static std::unique_ptr<RV5StageVM_NH_NF> runNhNfProgramMemory(const std::string& filename)
{
    setupVmStateDirectory();

    auto vm = std::make_unique<RV5StageVM_NH_NF>();
    AssembledProgram program = assemble(filename);
    vm->LoadProgram(program);
    vm->DebugRun();
    return vm;
}

TEST(RV5StageVM_NH_NF_FP_MEMORY_TEST, f_extension_memory_multi_slot_and_overwrite)
{
    auto vm = runNhNfProgramMemory("../examples/f_extension_memory_nh_nf_test.s");

    EXPECT_EQ(vm->registers_.ReadGpr(10), 3);
    EXPECT_EQ(vm->registers_.ReadGpr(11), 11);

    EXPECT_EQ(vm->registers_.ReadGpr(12), 1);
    EXPECT_EQ(vm->registers_.ReadGpr(13), 1);

    EXPECT_EQ(vm->registers_.ReadGpr(14), 11);
}

TEST(RV5StageVM_NH_NF_FP_MEMORY_TEST, d_extension_memory_multi_slot_and_overwrite)
{
    auto vm = runNhNfProgramMemory("../examples/d_extension_memory_nh_nf_test.s");

    EXPECT_EQ(vm->registers_.ReadGpr(10), 5);
    EXPECT_EQ(vm->registers_.ReadGpr(11), 17);

    EXPECT_EQ(vm->registers_.ReadGpr(12), 1);
    EXPECT_EQ(vm->registers_.ReadGpr(13), 1);

    EXPECT_EQ(vm->registers_.ReadGpr(14), 17);
}
