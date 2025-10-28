#include <gtest/gtest.h>
#include "../include/vm/rv5s/rv5s_vm.h"
#include "../include/assembler/assembler.h"
#include "../include/utils.h"

TEST(FIVE_STAGE_VM_TEST,PipelineTest)
{
    setupVmStateDirectory();
    RV5StageVM vm;
    AssembledProgram program = assemble("../examples/pipeline_test_code.s");
    vm.LoadProgram(program);
    vm.DebugRun();
    
    
}