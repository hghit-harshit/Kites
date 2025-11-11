#include <gtest/gtest.h>
#include "../include/vm/rv5s_vms/rv5svm_h_f.h"
#include "../include/assembler/assembler.h"
#include "../include/utils.h"

TEST(RV5StageVM_H_F_TEST,hazard_forwarding_test_1)
{
    setupVmStateDirectory();
    RV5StageVM_H_F vm;
    std::cout << "Program Information " << std::endl;
    std::cout << "Program size : " <<vm.program_size_ << std::endl;
    AssembledProgram program = assemble("../examples/pipeline_test_code_1.s");
    vm.LoadProgram(program);
    vm.DebugRun();
    
    GTEST_ASSERT_EQ(vm.registers_.ReadGpr(1), 36);
}