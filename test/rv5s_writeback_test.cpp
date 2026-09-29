#include <gtest/gtest.h>

#include <memory>
#include <sstream>
#include <string>

#include "assembler/assembler.h"
#include "processor/rv5s/rv5s_processor_h_f.h"
#include "processor/rv5s/rv5s_processor_h_nf.h"
#include "processor/rv5s/rv5s_processor_nh_f.h"
#include "processor/rv5s/rv5s_processor_nh_nf.h"
#include "utils/utils.h"

namespace
{

template <typename VmType>
std::unique_ptr<VmType> runSource(const std::string &source)
{
    Kites::setupVmStateDirectory();

    auto vm = std::make_unique<VmType>();
    std::istringstream stream(source);
    Kites::AssembledProgram program = Kites::assemble(stream);
    vm->LoadProgram(program);
    vm->DebugRun();
    return vm;
}

constexpr const char *kPad = "nop\n"
                             "nop\n"
                             "nop\n";

// Every instruction retires, not only those writing a nonzero GPR: stores,
// branches (taken and not), jumps to x0 and nops all count. Flushed wrong-path
// instructions and bubbles do not.
template <typename VmType>
void runRetiredCount()
{
    auto vm = runSource<VmType>(std::string("addi x10, x0, 128\n") + kPad + // 4
                                "sw x0, 0(x10)\n"                           // 5
                                "beq x0, x10, skip\n"                       // 6 not taken
                                "beq x0, x0, skip\n"                        // 7 taken
                                "addi x5, x0, 1\n"                          // flushed
                                "skip:\n"
                                "j end\n"          // 8
                                "addi x6, x0, 1\n" // flushed
                                "end:\n"
                                "nop\n"); // 9
    EXPECT_EQ(vm->instructions_retired_, 9u);
    EXPECT_EQ(vm->registers_.ReadGpr(5), 0u);
    EXPECT_EQ(vm->registers_.ReadGpr(6), 0u);
}

// FP writeback used to sit behind the GPR `rd != 0` check, so writes to f0 were lost.
template <typename VmType>
void runFpWriteToF0()
{
    auto vm = runSource<VmType>(std::string("addi x5, x0, 3\n") + kPad +
                                "fcvt.s.w f0, x5\n" + kPad + "fcvt.w.s x6, f0\n");
    EXPECT_EQ(vm->registers_.ReadFpr(0) & 0xFFFFFFFFu, 0x40400000u); // 3.0f
    EXPECT_EQ(vm->registers_.ReadGpr(6), 3u);
}

template <typename VmType>
void runWritebackSuite()
{
    runRetiredCount<VmType>();
    runFpWriteToF0<VmType>();
}

} // namespace

TEST(RV5SWriteback, HazardForwarding)
{
    runWritebackSuite<Kites::RV5StageProcessorHF>();
}

TEST(RV5SWriteback, HazardNoForwarding)
{
    runWritebackSuite<Kites::RV5StageProcessorHNF>();
}

TEST(RV5SWriteback, NoHazardForwarding)
{
    runWritebackSuite<Kites::RV5StageProcessorNHF>();
}

TEST(RV5SWriteback, NoHazardNoForwarding)
{
    runWritebackSuite<Kites::RV5StageProcessorNHNF>();
}
