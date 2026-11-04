#include <gtest/gtest.h>

#include <iostream>
#include <memory>
#include <sstream>
#include <string>

#include "assembler/assembler.h"
#include "processor/rv5s/rv5s_processor_h_f.h"
#include "processor/rv5s/rv5s_processor_h_nf.h"
#include "utils/utils.h"

namespace
{

template <typename VmType>
void runQuirk(const std::string &name, const std::string &source)
{
    Kites::setupVmStateDirectory();

    auto vm = std::make_unique<VmType>();
    std::istringstream stream(source);
    Kites::AssembledProgram program = Kites::assemble(stream);
    vm->LoadProgram(program);
    vm->DebugRun();

    std::cout << "[QUIRK] " << name
              << " cycles=" << vm->cycle_s_
              << " stalls=" << vm->total_stalls_
              << " retired=" << vm->instructions_retired_ << std::endl;
}

std::string quirkSource(int imm)
{
    return "addi x10, x0, 64\n"
           "lw   x5, 0(x10)\n"
           "addi x6, x0, " + std::to_string(imm) + "\n";
}

} // namespace

TEST(ReorderQuirk, Imm5_HF)  { runQuirk<Kites::RV5StageProcessorHF>("imm5 h_f", quirkSource(5)); }
TEST(ReorderQuirk, Imm6_HF)  { runQuirk<Kites::RV5StageProcessorHF>("imm6 h_f", quirkSource(6)); }
TEST(ReorderQuirk, Imm5_HNF) { runQuirk<Kites::RV5StageProcessorHNF>("imm5 h_nf", quirkSource(5)); }
TEST(ReorderQuirk, Imm6_HNF) { runQuirk<Kites::RV5StageProcessorHNF>("imm6 h_nf", quirkSource(6)); }
