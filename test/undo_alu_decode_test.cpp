/**
 * @file undo_alu_decode_test.cpp
 * @brief After stepping, undoing and resuming, every processor must reach the
 *        same final state as an uninterrupted run.
 *
 * Guards against the EX stage choosing its ALU operation from stale decode
 * state: the pipeline control unit is not part of the undo history, so after
 * an undo it still holds the signals of a younger instruction. The program
 * puts instructions with different ALU-op classes (load/store = add,
 * I-type, R-type) next to each other so a stale choice changes the result.
 */

#include <gtest/gtest.h>

#include <array>
#include <cstdint>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "assembler/assembler.h"
#include "processor/rv5s/rv5s_processor_h_f.h"
#include "processor/rv5s/rv5s_processor_h_nf.h"
#include "processor/rv5s/rv5s_processor_nh_f.h"
#include "processor/rv5s/rv5s_processor_nh_nf.h"
#include "processor/rvss/rvss_processor.h"
#include "utils/utils.h"

namespace
{

const std::string kPad = "addi x0, x0, 0\naddi x0, x0, 0\naddi x0, x0, 0\n";

// Hazard-free (all sources are written well before use), so the result is the
// same on every model, including the one without hazard handling.
const std::string kMixedProgram = ".data\n"
                                  "arr: .word 5, 7, 9, 11\n"
                                  ".text\n"
                                  "lui s0, 0x10000\n" // data section base
                                  "addi s1, x0, 3\n"
                                  "addi s2, x0, 4\n" +
                                  kPad +
                                  "lw t0, 0(s0)\n"
                                  "add t1, s1, s2\n"
                                  "addi t2, x0, 9\n"
                                  "lw t3, 4(s0)\n"
                                  "sub t4, s2, s1\n"
                                  "xori t5, s1, 6\n"
                                  "sw s2, 8(s0)\n"
                                  "slt t6, s1, s2\n"
                                  "andi a0, s2, 6\n"
                                  "lw a1, 12(s0)\n"
                                  "or a2, s1, s2\n"
                                  "sll a3, s2, s1\n"
                                  "srai a4, s2, 1\n" +
                                  kPad;

constexpr uint64_t kDataBase = 0x10000000;

struct FinalState
{
    std::array<uint64_t, 32> gprs{};
    std::vector<uint8_t> data;
};

FinalState capture(Kites::ProcessorBase &vm)
{
    FinalState s;
    for (size_t i = 0; i < s.gprs.size(); ++i)
    {
        s.gprs[i] = vm.registers_.ReadGpr(static_cast<uint8_t>(i));
    }
    for (uint64_t a = kDataBase; a < kDataBase + 16; ++a)
    {
        s.data.push_back(vm.memory_controller_.readByte(a));
    }
    return s;
}

template <typename Vm>
std::unique_ptr<Vm> loadMixedProgram()
{
    Kites::setupVmStateDirectory();
    auto vm = std::make_unique<Vm>();
    std::istringstream stream(kMixedProgram);
    vm->LoadProgram(Kites::assemble(stream));
    return vm;
}

template <typename Vm>
class UndoAluDecode : public ::testing::Test
{
};

using AllVms = ::testing::Types<Kites::RVSSProcessor, Kites::RV5StageProcessorHF,
                                Kites::RV5StageProcessorHNF, Kites::RV5StageProcessorNHF,
                                Kites::RV5StageProcessorNHNF>;
TYPED_TEST_SUITE(UndoAluDecode, AllVms);

TYPED_TEST(UndoAluDecode, StraightRunHasExpectedValues)
{
    auto vm = loadMixedProgram<TypeParam>();
    vm->DebugRun();
    auto &r = vm->registers_;
    EXPECT_EQ(r.ReadGpr(5), 5u);   // t0 = arr[0]
    EXPECT_EQ(r.ReadGpr(6), 7u);   // t1 = 3 + 4
    EXPECT_EQ(r.ReadGpr(28), 7u);  // t3 = arr[1]
    EXPECT_EQ(r.ReadGpr(29), 1u);  // t4 = 4 - 3
    EXPECT_EQ(r.ReadGpr(30), 5u);  // t5 = 3 ^ 6
    EXPECT_EQ(r.ReadGpr(31), 1u);  // t6 = 3 < 4
    EXPECT_EQ(r.ReadGpr(10), 4u);  // a0 = 4 & 6
    EXPECT_EQ(r.ReadGpr(11), 11u); // a1 = arr[3]
    EXPECT_EQ(r.ReadGpr(12), 7u);  // a2 = 3 | 4
    EXPECT_EQ(r.ReadGpr(13), 32u); // a3 = 4 << 3
    EXPECT_EQ(r.ReadGpr(14), 2u);  // a4 = 4 >> 1
    EXPECT_EQ(vm->memory_controller_.readWord(kDataBase + 8), 4u); // sw s2
}

TYPED_TEST(UndoAluDecode, StepUndoResumeMatchesStraightRun)
{
    auto reference = loadMixedProgram<TypeParam>();
    reference->DebugRun();
    const FinalState expected = capture(*reference);

    for (int steps = 1; steps <= 24; ++steps)
    {
        for (int undos = 1; undos <= 3 && undos <= steps; ++undos)
        {
            SCOPED_TRACE(::testing::Message() << "steps=" << steps << " undos=" << undos);
            auto vm = loadMixedProgram<TypeParam>();
            for (int i = 0; i < steps; ++i)
            {
                vm->Step();
            }
            for (int i = 0; i < undos; ++i)
            {
                vm->Undo();
            }
            vm->DebugRun();
            const FinalState actual = capture(*vm);
            EXPECT_EQ(actual.gprs, expected.gprs);
            EXPECT_EQ(actual.data, expected.data);
        }
    }
}

} // namespace
