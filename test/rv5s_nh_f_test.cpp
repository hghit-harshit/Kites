#include <gtest/gtest.h>
#include "../include/vm/rv5s_vms/rv5svm_nh_f.h"
#include "../include/assembler/assembler.h"
#include "../include/utils.h"
#include "../include/assembler/assembler.h"

// TEST(RV5S_NH_F_TEST,no_hazard_forwarding_test_1)
// {
//     setupVmStateDirectory();
//     RV5StageVM_NH_F vm;
//     std::cout << "Program Information " << std::endl;
//     std::cout << "Program size : " <<vm.program_size_ << std::endl;
//     AssembledProgram program = assemble("../examples/pipeline_test_code_1.s");
//     vm.LoadProgram(program);
//     vm.DebugRun();
    
//     GTEST_ASSERT_EQ(vm.registers_.ReadGpr(1), 36);
// }