#include <gtest/gtest.h>
#include "../include/vm/rv5s_vms/rv5svm_nh_nf.h"
#include "../include/vm/rv5s_vms/rv5svm_nh_f.h"
#include "../include/vm/rv5s_vms/rv5svm_h_f.h"
#include "../include/vm/rv5s_vms/rv5svm_h_nf.h"
#include "../include/assembler/assembler.h"
#include "../include/utils.h"

// TEST(FIVE_STAGE_VM_TEST,no_hazard_no_forwarding_test_1)
// {
//     setupVmStateDirectory();
//     RV5StageVM_NH_NF vm;
//     std::cout << "Program Information " << std::endl;
//     std::cout << "Program size : " <<vm.program_size_ << std::endl;
//     AssembledProgram program = assemble("../examples/pipeline_test_code_1.s");
//     vm.LoadProgram(program);
//     vm.DebugRun();
    
//     GTEST_ASSERT_EQ(vm.registers_.ReadGpr(1), 15);
// }

// TEST(FIVE_STAGE_VM_TEST,no_hazard_no_forwarding_test_2)
// {
//     setupVmStateDirectory();
//     RV5StageVM_NH_NF vm;
//     AssembledProgram program = assemble("../examples/pipeline_test_code_2.s");
//     vm.LoadProgram(program);
//     vm.DebugRun();    
    
//     GTEST_ASSERT_EQ(vm.registers_.ReadGpr(4), 20);
// }

// TEST(FIVE_STAGE_VM_TEST,hazard_no_forwarding_test_1)
// {
//     setupVmStateDirectory();
//     RV5StageVM_H_NF vm;
//     AssembledProgram program = assemble("../examples/pipeline_test_code_1.s");
//     vm.LoadProgram(program);
//     vm.DebugRun();    
    
//     GTEST_ASSERT_EQ(vm.registers_.ReadGpr(1), 36);
// }

// TEST(FIVE_STAGE_VM_TEST,no_hazard_forwarding_test)
// {
//     setupVmStateDirectory();
//     RV5StageVM_NH_F vm;
//     AssembledProgram program = assemble("../examples/pipeline_test_code.s");
//     vm.LoadProgram(program);
//     vm.DebugRun();    
    
// }

// TEST(FIVE_STAGE_VM_TEST,harzard_no_forwarding_test)
// {
//     setupVmStateDirectory();
//     RV5StageVM_H_F vm;
//     AssembledProgram program = assemble("../examples/pipeline_test_code.s");
//     vm.LoadProgram(program);
//     vm.DebugRun();    
    
// }

// TEST(FIVE_STAGE_VM_TEST,harzard_forwarding_test)
// {
//     setupVmStateDirectory();
//     RV5StageVM_H_NF vm;
//     AssembledProgram program = assemble("../examples/pipeline_test_code.s");
//     vm.LoadProgram(program);
//     vm.DebugRun();    
    
// }