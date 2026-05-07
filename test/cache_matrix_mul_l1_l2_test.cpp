#include <gtest/gtest.h>

#include <array>
#include <cstdint>
#include <memory>

#include "vm/cache/cache.h"
#include "../include/assembler/assembler.h"
#include "../include/utils.h"
#include "../include/vm/rv5s_vms/rv5svm_h_f.h"
#include "../include/vm/rv5s_vms/rv5svm_h_nf.h"
#include "../include/vm/rv5s_vms/rv5svm_nh_f.h"
#include "../include/vm/rv5s_vms/rv5svm_nh_nf.h"
#include "../include/vm/rvss/rvss_vm.h"

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

void writeMatrix(MemoryController& memory, uint64_t base, const std::array<uint32_t, kMatrixWords>& values)
{
    for (size_t i = 0; i < values.size(); ++i)
    {
        memory.WriteWord_d(base + static_cast<uint64_t>(i * 4), values[i]);
    }
}

void warmL2(Cache& l2, uint64_t base, size_t words)
{
    for (size_t i = 0; i < words; ++i)
    {
        l2.ReadWord(base + static_cast<uint64_t>(i * 4));
    }
}

template <typename VmType>
void runMatrixMultiplyCacheTest(const char* vm_label)
{
    SCOPED_TRACE(::testing::Message() << "vm=" << vm_label);

    setupVmStateDirectory();

    auto vm = std::make_unique<VmType>();

    Cache* l1_cache = vm->memory_controller_.GetL1Cache();
    Cache* l2_cache = vm->memory_controller_.GetL2Cache();
    Cache* instruction_cache = vm->memory_controller_.GetInstructionCache();

    l2_cache->Reconfigure(CacheConfig{
        64,
        1,
        2,
        WritePolicy::WriteBack,
        AllocationPolicy::WriteAllocate,
        ReplacementPolicy::LRU});

    l1_cache->Reconfigure(CacheConfig{
        2,
        1,
        1,
        WritePolicy::WriteBack,
        AllocationPolicy::WriteAllocate,
        ReplacementPolicy::LRU});

    instruction_cache->Reconfigure(CacheConfig{
        4,
        1,
        1,
        WritePolicy::WriteThrough,
        AllocationPolicy::WriteAllocate,
        ReplacementPolicy::LRU});

    writeMatrix(vm->memory_controller_, kBaseA, kMatrixA);
    writeMatrix(vm->memory_controller_, kBaseB, kMatrixB);

    for (size_t i = 0; i < kMatrixWords; ++i)
    {
        vm->memory_controller_.WriteWord_d(kBaseC + static_cast<uint64_t>(i * 4), 0);
    }

    warmL2(*l2_cache, kBaseA, kMatrixWords);
    warmL2(*l2_cache, kBaseB, kMatrixWords);
    warmL2(*l2_cache, kBaseC, kMatrixWords);

    l1_cache->Reset();

    const size_t l2_hits_probe_before = l2_cache->GetHits();
    const uint32_t probe_value = vm->memory_controller_.ReadWord(kBaseA);
    EXPECT_EQ(probe_value, kMatrixA[0]);
    const size_t l2_hits_probe_after = l2_cache->GetHits();
    EXPECT_GT(l2_hits_probe_after, l2_hits_probe_before);

    l1_cache->Reset();

    const size_t l1_misses_before = l1_cache->GetMisses();
    const size_t l2_hits_before = l2_cache->GetHits();

    AssembledProgram program = assemble("../examples/cache_matrix_mul_3x3.s");
    vm->LoadProgram(program);
    vm->DebugRun();

    const size_t l1_misses_after = l1_cache->GetMisses();
    const size_t l2_hits_after = l2_cache->GetHits();

    EXPECT_GT(l1_misses_after - l1_misses_before, 0u);
    EXPECT_GT(l2_hits_after - l2_hits_before, 0u);

    l1_cache->Flush();
    l2_cache->Flush();

    for (size_t i = 0; i < kMatrixWords; ++i)
    {
        const uint32_t value = vm->memory_controller_.ReadWord_d(kBaseC + static_cast<uint64_t>(i * 4));
        EXPECT_EQ(value, kExpectedC[i]) << "index=" << i;
    }
}

} // namespace

TEST(CacheHierarchyTest, MatrixMultiplyUsesL2OnL1Misses_RVSS)
{
    runMatrixMultiplyCacheTest<RVSSVM>("RVSSVM");
}

TEST(CacheHierarchyTest, MatrixMultiplyUsesL2OnL1Misses_RV5_NH_NF)
{
    runMatrixMultiplyCacheTest<RV5StageVM_NH_NF>("RV5StageVM_NH_NF");
}

TEST(CacheHierarchyTest, MatrixMultiplyUsesL2OnL1Misses_RV5_NH_F)
{
    runMatrixMultiplyCacheTest<RV5StageVM_NH_F>("RV5StageVM_NH_F");
}

TEST(CacheHierarchyTest, MatrixMultiplyUsesL2OnL1Misses_RV5_H_NF)
{
    runMatrixMultiplyCacheTest<RV5StageVM_H_NF>("RV5StageVM_H_NF");
}

TEST(CacheHierarchyTest, MatrixMultiplyUsesL2OnL1Misses_RV5_H_F)
{
    runMatrixMultiplyCacheTest<RV5StageVM_H_F>("RV5StageVM_H_F");
}
