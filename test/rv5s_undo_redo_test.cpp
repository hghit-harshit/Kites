#include <gtest/gtest.h>

#include <array>
#include <memory>
#include <string>
#include <vector>

#include "../include/assembler/assembler.h"
#include "../include/utils.h"
#include "../include/processor/rv5s/rv5s_processor_h_f.h"
#include "../include/processor/rv5s/rv5s_processor_h_nf.h"
#include "../include/processor/rv5s/rv5s_processor_nh_f.h"
#include "../include/processor/rv5s/rv5s_processor_nh_nf.h"

namespace {

struct VmSnapshot {
    uint64_t pc = 0;
    unsigned int cycles = 0;
    unsigned int retired = 0;
    std::array<uint64_t, 32> gprs{};
    std::vector<uint8_t> watched_memory;
};

VmSnapshot captureSnapshot(VmBase& vm, const std::vector<uint64_t>& watched_addresses)
{
    VmSnapshot snapshot;
    snapshot.pc = vm.program_counter_;
    snapshot.cycles = vm.cycle_s_;
    snapshot.retired = vm.instructions_retired_;

    for (size_t i = 0; i < snapshot.gprs.size(); ++i)
    {
        snapshot.gprs[i] = vm.registers_.ReadGpr(static_cast<uint8_t>(i));
    }

    snapshot.watched_memory.reserve(watched_addresses.size());
    for (const uint64_t addr : watched_addresses)
    {
        snapshot.watched_memory.push_back(vm.memory_controller_.ReadByte(addr));
    }

    return snapshot;
}

void expectSnapshotsEqual(const VmSnapshot& actual, const VmSnapshot& expected)
{
    EXPECT_EQ(actual.pc, expected.pc);
    EXPECT_EQ(actual.cycles, expected.cycles);
    EXPECT_EQ(actual.retired, expected.retired);
    EXPECT_EQ(actual.gprs, expected.gprs);
    EXPECT_EQ(actual.watched_memory, expected.watched_memory);
}

template <typename VM>
void runUndoRedoRoundTripTest(const std::string& asm_file,
                              int step_count,
                              const std::vector<uint64_t>& watched_addresses)
{
    setupVmStateDirectory();

    VM vm;
    AssembledProgram program = assemble(asm_file);
    vm.LoadProgram(program);

    for (int i = 0; i < step_count; ++i)
    {
        SCOPED_TRACE(::testing::Message() << "step=" << i << " file=" << asm_file);

        const VmSnapshot before_step = captureSnapshot(vm, watched_addresses);

        vm.Step();

        const VmSnapshot after_step = captureSnapshot(vm, watched_addresses);

        vm.Undo();
        const VmSnapshot after_undo = captureSnapshot(vm, watched_addresses);
        expectSnapshotsEqual(after_undo, before_step);

        vm.Redo();
        const VmSnapshot after_redo = captureSnapshot(vm, watched_addresses);
        expectSnapshotsEqual(after_redo, after_step);
    }
}

std::vector<uint64_t> watchedMemoryRange(uint64_t base, size_t size)
{
    std::vector<uint64_t> addresses;
    addresses.reserve(size);
    for (size_t i = 0; i < size; ++i)
    {
        addresses.push_back(base + i);
    }
    return addresses;
}

} // namespace

TEST(RV5S_UNDO_REDO_NH_NF, basic_round_trip)
{
    runUndoRedoRoundTripTest<RV5StageVM_NH_NF>("../examples/rv5s_undo_redo_basic.s", 12, {});
}

TEST(RV5S_UNDO_REDO_NH_F, basic_round_trip)
{
    runUndoRedoRoundTripTest<RV5StageVM_NH_F>("../examples/rv5s_undo_redo_basic.s", 12, {});
}

TEST(RV5S_UNDO_REDO_H_NF, basic_round_trip)
{
    runUndoRedoRoundTripTest<RV5StageVM_H_NF>("../examples/rv5s_undo_redo_basic.s", 12, {});
}

TEST(RV5S_UNDO_REDO_H_F, basic_round_trip)
{
    runUndoRedoRoundTripTest<RV5StageVM_H_F>("../examples/rv5s_undo_redo_basic.s", 12, {});
}

TEST(RV5S_UNDO_REDO_NH_NF, memory_round_trip)
{
    runUndoRedoRoundTripTest<RV5StageVM_NH_NF>(
        "../examples/rv5s_undo_redo_memory.s", 12, watchedMemoryRange(128, 16));
}

TEST(RV5S_UNDO_REDO_NH_F, memory_round_trip)
{
    runUndoRedoRoundTripTest<RV5StageVM_NH_F>(
        "../examples/rv5s_undo_redo_memory.s", 12, watchedMemoryRange(128, 16));
}

TEST(RV5S_UNDO_REDO_H_NF, memory_round_trip)
{
    runUndoRedoRoundTripTest<RV5StageVM_H_NF>(
        "../examples/rv5s_undo_redo_memory.s", 12, watchedMemoryRange(128, 16));
}

TEST(RV5S_UNDO_REDO_H_F, memory_round_trip)
{
    runUndoRedoRoundTripTest<RV5StageVM_H_F>(
        "../examples/rv5s_undo_redo_memory.s", 12, watchedMemoryRange(128, 16));
}
