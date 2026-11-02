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
void runAndReport(const std::string &name, const std::string &source)
{
    Kites::setupVmStateDirectory();

    auto vm = std::make_unique<VmType>();
    std::istringstream stream(source);
    Kites::AssembledProgram program = Kites::assemble(stream);
    vm->LoadProgram(program);
    vm->DebugRun();

    std::cout << "[BASELINE] " << name
              << " cycles=" << vm->cycle_s_
              << " retired=" << vm->instructions_retired_ << std::endl;
}

const char *kLU =
    "addi x1, x0, 100\n"
    "addi x2, x0, 20\n"
    "sw   x2, 0(x1)\n"
    "lw   x3, 0(x1)\n"
    "add  x6, x3, x1\n";

const char *kRR =
    "addi x1, x0, 5\n"
    "add  x2, x1, x1\n"
    "sub  x6, x2, x1\n";

} // namespace

TEST(ReorderBaseline, LU_hazard_HF)  { runAndReport<Kites::RV5StageProcessorHF>("L_U h_f", kLU); }
TEST(ReorderBaseline, LU_hazard_HNF) { runAndReport<Kites::RV5StageProcessorHNF>("L_U h_nf", kLU); }
TEST(ReorderBaseline, RR_hazard_HF)  { runAndReport<Kites::RV5StageProcessorHF>("R_R h_f", kRR); }
TEST(ReorderBaseline, RR_hazard_HNF) { runAndReport<Kites::RV5StageProcessorHNF>("R_R h_nf", kRR); }
