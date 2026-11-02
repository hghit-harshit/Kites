#include <gtest/gtest.h>
#include <iostream>
#include <memory>
#include "processor/rv5s/rv5s_processor_base.h"
#include "processor/rv5s/rv5s_processor_h_nf.h"
#include "processor/rv5s/rv5s_processor_h_f.h"
#include "assembler/assembler.h"
#include "utils/utils.h"

using namespace Kites;

TEST(TotalStalls, hf_load_use_example)
{
    setupVmStateDirectory();
    auto vm = std::make_unique<RV5StageProcessorHF>();
    AssembledProgram program = assemble("../examples/L_U_hazard.s");
    vm->LoadProgram(program);
    vm->DebugRun();

    std::cout << "[HF  L_U] cycles=" << vm->cycle_s_
              << " stalls=" << vm->total_stalls_
              << " retired=" << vm->instructions_retired_ << std::endl;

    EXPECT_GT(vm->total_stalls_, 0u);
}

TEST(TotalStalls, hnf_load_use_example)
{
    setupVmStateDirectory();
    auto vm = std::make_unique<RV5StageProcessorHNF>();
    AssembledProgram program = assemble("../examples/L_U_hazard.s");
    vm->LoadProgram(program);
    vm->DebugRun();

    std::cout << "[HNF L_U] cycles=" << vm->cycle_s_
              << " stalls=" << vm->total_stalls_
              << " retired=" << vm->instructions_retired_ << std::endl;

    EXPECT_GT(vm->total_stalls_, 0u);
}