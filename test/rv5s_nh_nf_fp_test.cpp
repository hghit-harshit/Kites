#include <gtest/gtest.h>
#include "vm/rvss/rvss_vm.h"

#include "../include/vm/rv5s_vms/rv5svm_nh_nf.h"
#include "../include/assembler/assembler.h"
#include "../include/utils.h"

static std::unique_ptr<RV5StageVM_NH_NF> runNhNfProgram(const std::string& filename)
{
    setupVmStateDirectory();

    auto vm = std::make_unique<RV5StageVM_NH_NF>();
    AssembledProgram program = assemble(filename);
    vm->LoadProgram(program);
    vm->DebugRun();
    return vm;
}

static std::unique_ptr<VmBase> runProgram(const std::string& filename)
{
    setupVmStateDirectory();

    auto vm = std::make_unique<RVSSVM>();
    AssembledProgram program = assemble(filename);
    vm->LoadProgram(program);
    vm->DebugRun();
    return vm;
}

TEST(RV5StageVM_NH_NF_FP_TEST, f_extension_arithmetic_and_memory_roundtrip)
{
    auto vm = runNhNfProgram("../examples/f_extension_nh_nf_test.s");
    //auto vm = runProgram("../examples/f_extension_nh_nf_test.s");
    // fcvt.w.s x10, f3 where f3 = 8.0
    EXPECT_EQ(vm->registers_.ReadGpr(10), 8);

    // feq.s x11, f3, f3
    EXPECT_EQ(vm->registers_.ReadGpr(11), 1);

    // fcvt.w.s x12, f4 after fsw/flw roundtrip
    EXPECT_EQ(vm->registers_.ReadGpr(12), 8);
}

TEST(RV5StageVM_NH_NF_FP_TEST, d_extension_arithmetic_and_memory_roundtrip)
{
    auto vm = runNhNfProgram("../examples/d_extension_nh_nf_test.s");

    // fcvt.w.d x10, f3 where f3 = 13.0
    EXPECT_EQ(vm->registers_.ReadGpr(10), 13);

    // feq.d x11, f3, f3
    EXPECT_EQ(vm->registers_.ReadGpr(11), 1);

    // fcvt.w.d x12, f4 after fsd/fld roundtrip
    EXPECT_EQ(vm->registers_.ReadGpr(12), 13);
}
