#include <gtest/gtest.h>
#include "../include/vm/rv5s_vms/rv5svm_nh_nf.h"
#include "../include/vm/rv5s_vms/rv5svm_nh_f.h"
#include "../include/vm/rv5s_vms/rv5svm_h_f.h"
#include "../include/vm/rv5s_vms/rv5svm_h_nf.h"
#include "../include/assembler/assembler.h"
#include "../include/utils.h"

TEST(FIVE_STAGE_VM_TEST,no_hazard_no_forwarding_test)
{
    setupVmStateDirectory();
    RV5StageVM_NH_NF vm;
    AssembledProgram program = assemble("../examples/pipeline_test_code.s");
    vm.LoadProgram(program);
    vm.DebugRun();    
    
}
TEST(FIVE_STAGE_VM_TEST,no_hazard_forwarding_test)
{
    setupVmStateDirectory();
    RV5StageVM_NH_F vm;
    AssembledProgram program = assemble("../examples/pipeline_test_code.s");
    vm.LoadProgram(program);
    vm.DebugRun();    
    
}

TEST(FIVE_STAGE_VM_TEST,harzard_no_forwarding_test)
{
    setupVmStateDirectory();
    RV5StageVM_H_F vm;
    AssembledProgram program = assemble("../examples/pipeline_test_code.s");
    vm.LoadProgram(program);
    vm.DebugRun();    
    
}

TEST(FIVE_STAGE_VM_TEST,harzard_forwarding_test)
{
    setupVmStateDirectory();
    RV5StageVM_H_NF vm;
    AssembledProgram program = assemble("../examples/pipeline_test_code.s");
    vm.LoadProgram(program);
    vm.DebugRun();    
    
}