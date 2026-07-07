// #include <gtest/gtest.h>
// #include "../include/processor/rv5s/rv5s_processor_h_nf.h"
// #include "../include/assembler/assembler.h"
// #include "../include/utils.h"

// TEST(RV5StageVM_H_NF_TEST,hazard_forwarding_test_1)
// {
//     setupVmStateDirectory();
//     RV5StageProcessorHNF vm;
//     std::cout << "Program Information " << std::endl;
//     std::cout << "Program size : " <<vm.program_size_ << std::endl;
//     AssembledProgram program = assemble("../examples/pipeline_test_code_1.s");
//     vm.LoadProgram(program);
//     vm.DebugRun();
    
//     GTEST_ASSERT_EQ(vm.registers_.ReadGpr(6), 15);
// }


#include <gtest/gtest.h>
#include "../include/processor/rv5s/rv5s_processor_h_nf.h"
#include "../include/assembler/assembler.h"
#include "../include/utils.h"

static std::unique_ptr<RV5StageProcessorHNF> runHazardProgram(const std::string& filename)
{
    setupVmStateDirectory();

    auto vm = std::make_unique<RV5StageProcessorHNF>();
    AssembledProgram program = assemble(filename);
    vm->LoadProgram(program);
    vm->DebugRun();
    return vm;
}

TEST(RV5StageVM_H_NF_TEST, test1)
{
    auto vm = runHazardProgram("../examples/pipeline_test_code_1.s");
    //EXPECT_NE(vm, nullptr);
}


// ----------------------------
// 1. R -> R
// ----------------------------
TEST(RV5StageVM_H_NF_TEST, r_r_forwarding)
{
    auto vm = runHazardProgram("../examples/R_R_hazard.s");
    EXPECT_EQ(vm->registers_.ReadGpr(6), 5);
}

// ----------------------------
// 2. Load -> Use
// ----------------------------
TEST(RV5StageVM_H_NF_TEST, l_u_forwarding)
{
    auto vm = runHazardProgram("../examples/L_U_hazard.s");
    EXPECT_EQ(vm->registers_.ReadGpr(6), 120);
}

// ----------------------------
// 3. R -> Store
// ----------------------------
TEST(RV5StageVM_H_NF_TEST, r_s_forwarding)
{
    auto vm = runHazardProgram("../examples/R_S_hazard.s");
    EXPECT_EQ(vm->registers_.ReadGpr(8), 20);
}

// ----------------------------
// 4. Load -> Store
// ----------------------------
TEST(RV5StageVM_H_NF_TEST, l_s_forwarding)
{
    auto vm = runHazardProgram("../examples/L_S_hazard.s");
    EXPECT_EQ(vm->registers_.ReadGpr(7), 7);
}

// ----------------------------
// 5. R -> Branch
// ----------------------------
TEST(RV5StageVM_H_NF_TEST, r_b_forwarding)
{
    auto vm = runHazardProgram("../examples/R_B_hazard.s");
    EXPECT_EQ(vm->registers_.ReadGpr(10), 0);
}

// ----------------------------
// 6. Load -> Branch
// ----------------------------
TEST(RV5StageVM_H_NF_TEST, l_b_forwarding)
{
    auto vm = runHazardProgram("../examples/L_B_hazard.s");
    EXPECT_EQ(vm->registers_.ReadGpr(7), 1);
}

// ----------------------------
// 7. R -> MemAddr
// ----------------------------
TEST(RV5StageVM_H_NF_TEST, r_m_forwarding)
{
    auto vm = runHazardProgram("../examples/R_M_hazard.s");
    EXPECT_EQ(vm->registers_.ReadGpr(9), 55);
}
