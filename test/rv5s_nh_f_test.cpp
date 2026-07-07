// #include <gtest/gtest.h>
// #include "../include/processor/rv5s/rv5s_processor_nh_f.h"
// #include "../include/assembler/assembler.h"
// #include "../include/utils.h"
#include "../include/assembler/assembler.h"

// TEST(RV5S_NH_F_TEST,no_hazard_forwarding_test_1)
// {
//     setupVmStateDirectory();
//     RV5StageProcessorNHF vm;
//     std::cout << "Program Information " << std::endl;
//     std::cout << "Program size : " <<vm.program_size_ << std::endl;
//     AssembledProgram program = assemble("../examples/pipeline_test_code_1.s");
//     vm.LoadProgram(program);
//     vm.DebugRun();
    
//     EXPECT_EQ(vm.registers_.ReadGpr(6), 0);
// }

#include <gtest/gtest.h>
#include "../include/processor/rv5s/rv5s_processor_nh_f.h"
#include "../include/assembler/assembler.h"
#include "../include/utils.h"


static std::unique_ptr<RV5StageProcessorNHF> runHazardProgram(const std::string& filename)
{
    setupVmStateDirectory();

    auto vm = std::make_unique<RV5StageProcessorNHF>();
    AssembledProgram program = assemble(filename);
    vm->LoadProgram(program);
    vm->DebugRun();
    return vm;
}

// ----------------------------
// 1. R -> R
// ----------------------------
TEST(RV5StageVM_NH_F_TEST, r_r_forwarding)
{
    auto vm = runHazardProgram("../examples/R_R_hazard.s");
    EXPECT_EQ(vm->registers_.ReadGpr(6), 5);
}

// ----------------------------
// 2. Load -> Use
// ----------------------------
TEST(RV5StageVM_NH_F_TEST, l_u_forwarding)
{
    auto vm = runHazardProgram("../examples/L_U_hazard.s");
    EXPECT_EQ(vm->registers_.ReadGpr(6), 200);
}

// ----------------------------
// 3. R -> Store
// ----------------------------
TEST(RV5StageVM_NH_F_TEST, r_s_forwarding)
{
    auto vm = runHazardProgram("../examples/R_S_hazard.s");
    EXPECT_EQ(vm->registers_.ReadGpr(8), 20);
}

// ----------------------------
// 4. Load -> Store
// ----------------------------
TEST(RV5StageVM_NH_F_TEST, l_s_forwarding)
{
    auto vm = runHazardProgram("../examples/L_S_hazard.s");
    EXPECT_EQ(vm->registers_.ReadGpr(7), 7);
}

// ----------------------------
// 5. R -> Branch
// ----------------------------
TEST(RV5StageVM_NH_F_TEST, r_b_forwarding)
{
    auto vm = runHazardProgram("../examples/R_B_hazard.s");
    EXPECT_EQ(vm->registers_.ReadGpr(10), 0);
}

// ----------------------------
// 6. Load -> Branch
// ----------------------------
TEST(RV5StageVM_NH_F_TEST, l_b_forwarding)
{
    auto vm = runHazardProgram("../examples/L_B_hazard.s");
    EXPECT_EQ(vm->registers_.ReadGpr(7), 0);
}

// ----------------------------
// 7. R -> MemAddr
// ----------------------------
TEST(RV5StageVM_NH_F_TEST, r_m_forwarding)
{
    auto vm = runHazardProgram("../examples/R_M_hazard.s");
    EXPECT_EQ(vm->registers_.ReadGpr(9), 55);
}
