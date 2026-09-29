#include <gtest/gtest.h>

#include <array>
#include <cstdint>
#include <memory>

#include "processor/cache/cache.h"
#include "assembler/assembler.h"
#include "utils/utils.h"
#include "processor/rv5s/rv5s_processor_h_f.h"
#include "processor/rv5s/rv5s_processor_h_nf.h"
#include "processor/rv5s/rv5s_processor_nh_f.h"
#include "processor/rv5s/rv5s_processor_nh_nf.h"
#include "processor/rvss/rvss_processor.h"

using namespace Kites;

namespace {

constexpr uint64_t kBaseA = 0x2000;
constexpr uint64_t kBaseB = 0x2100;
constexpr uint64_t kBaseC = 0x2200;
constexpr size_t kMatrixSize = 3;
constexpr size_t kMatrixWords = kMatrixSize * kMatrixSize;

const std::array<uint32_t, kMatrixWords> kMatrixA = {
    1, 2, 3,
    4, 5, 6,
    7, 8, 9
};

const std::array<uint32_t, kMatrixWords> kMatrixB = {
    9, 8, 7,
    6, 5, 4,
    3, 2, 1
};

const std::array<uint32_t, kMatrixWords> kExpectedC = {
    30, 24, 18,
    84, 69, 54,
    138, 114, 90
};

void writeMatrix(Kites::MemoryController& memory, uint64_t base, const std::array<uint32_t, kMatrixWords>& values)
{
    for (size_t i = 0; i < values.size(); ++i)
    {
        memory.writeWord_d(base + static_cast<uint64_t>(i * 4), values[i]);
    }
}

void warmL2(Kites::Cache& l2, uint64_t base, size_t words)
{
    for (size_t i = 0; i < words; ++i)
    {
        l2.readWord(base + static_cast<uint64_t>(i * 4));
    }
}

template <typename VmType>
void runMatrixMultiplyCacheTest(const char* vm_label)
{
    SCOPED_TRACE(::testing::Message() << "vm=" << vm_label);

    setupVmStateDirectory();

    auto vm = std::make_unique<VmType>();

    Kites::Cache* l1_cache = vm->memory_controller_.getL1Cache();
    Kites::Cache* l2_cache = vm->memory_controller_.getL2Cache();
    Kites::Cache* instruction_cache = vm->memory_controller_.getInstructionCache();

    // A, B and C are 0x100 apart, which is exactly one way of this L2 (64 sets x 4 bytes),
    // so all three map to the same sets. 4 ways lets the L2 hold all of them at once.
    l2_cache->reconfigure(CacheConfig{
        64,
        4, // bytes per line (1 word)
        4,
        WritePolicy::WriteBack,
        AllocationPolicy::WriteAllocate,
        ReplacementPolicy::LRU});

    l1_cache->reconfigure(CacheConfig{
        2,
        4, // bytes per line (1 word)
        1,
        WritePolicy::WriteBack,
        AllocationPolicy::WriteAllocate,
        ReplacementPolicy::LRU});

    instruction_cache->reconfigure(CacheConfig{
        4,
        4, // bytes per line (1 word)
        1,
        WritePolicy::WriteThrough,
        AllocationPolicy::WriteAllocate,
        ReplacementPolicy::LRU});

    writeMatrix(vm->memory_controller_, kBaseA, kMatrixA);
    writeMatrix(vm->memory_controller_, kBaseB, kMatrixB);

    for (size_t i = 0; i < kMatrixWords; ++i)
    {
        vm->memory_controller_.writeWord_d(kBaseC + static_cast<uint64_t>(i * 4), 0);
    }

    warmL2(*l2_cache, kBaseA, kMatrixWords);
    warmL2(*l2_cache, kBaseB, kMatrixWords);
    warmL2(*l2_cache, kBaseC, kMatrixWords);

    l1_cache->reset();

    const size_t l2_hits_probe_before = l2_cache->getHitCount();
    const uint32_t probe_value = vm->memory_controller_.readWord(kBaseA);
    EXPECT_EQ(probe_value, kMatrixA[0]);
    const size_t l2_hits_probe_after = l2_cache->getHitCount();
    EXPECT_GT(l2_hits_probe_after, l2_hits_probe_before);

    l1_cache->reset();

    const size_t l1_misses_before = l1_cache->getMissCount();
    const size_t l2_hits_before = l2_cache->getHitCount();

    AssembledProgram program = assemble("../examples/cache_matrix_mul_3x3.s");
    vm->LoadProgram(program);
    vm->DebugRun();

    const size_t l1_misses_after = l1_cache->getMissCount();
    const size_t l2_hits_after = l2_cache->getHitCount();

    EXPECT_GT(l1_misses_after - l1_misses_before, 0u);
    EXPECT_GT(l2_hits_after - l2_hits_before, 0u);

    l1_cache->flush();
    l2_cache->flush();

    for (size_t i = 0; i < kMatrixWords; ++i)
    {
        const uint32_t value = vm->memory_controller_.readWord_d(kBaseC + static_cast<uint64_t>(i * 4));
        EXPECT_EQ(value, kExpectedC[i]) << "index=" << i;
    }
}

} // namespace

TEST(CacheHierarchyTest, MatrixMultiplyUsesL2OnL1Misses_RVSS)
{
    runMatrixMultiplyCacheTest<Kites::RVSSProcessor>("RVSSVM");
}

TEST(CacheHierarchyTest, MatrixMultiplyUsesL2OnL1Misses_RV5_NH_NF)
{
    runMatrixMultiplyCacheTest<Kites::RV5StageProcessorNHNF>("RV5StageProcessorNHNF");
}

TEST(CacheHierarchyTest, MatrixMultiplyUsesL2OnL1Misses_RV5_NH_F)
{
    runMatrixMultiplyCacheTest<Kites::RV5StageProcessorNHF>("RV5StageProcessorNHF");
}

TEST(CacheHierarchyTest, MatrixMultiplyUsesL2OnL1Misses_RV5_H_NF)
{
    runMatrixMultiplyCacheTest<Kites::RV5StageProcessorHNF>("RV5StageProcessorHNF");
}

TEST(CacheHierarchyTest, MatrixMultiplyUsesL2OnL1Misses_RV5_H_F)
{
    runMatrixMultiplyCacheTest<Kites::RV5StageProcessorHF>("RV5StageProcessorHF");
}
