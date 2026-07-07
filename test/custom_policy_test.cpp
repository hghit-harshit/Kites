#include <gtest/gtest.h>
#include <filesystem>

#include "../include/assembler/assembler.h"
#include "../include/utils.h"
#include "../include/processor/rvss/rvss_vm.h"
#include "processor/main_memory.h"
#include "processor/cache/cache.h"

class CustomPolicyTest : public ::testing::Test
{
protected:
    static std::filesystem::path ResolveLfuScriptPath()
    {
        const auto repo_root = std::filesystem::path(__FILE__).parent_path().parent_path();
        const auto script_path = repo_root / "examples" / "custom-replacement-scripts" / "lfu.lua";
        EXPECT_TRUE(std::filesystem::exists(script_path))
            << "LFU custom policy script not found at: " << script_path.string();
        return script_path;
    }

    static std::filesystem::path ResolveAssemblyProgramPath()
    {
        const auto repo_root = std::filesystem::path(__FILE__).parent_path().parent_path();
        const auto asm_path = repo_root / "examples" / "custom_policy_lfu_read_1000.s";
        EXPECT_TRUE(std::filesystem::exists(asm_path))
            << "Assembly test program not found at: " << asm_path.string();
        return asm_path;
    }

    Memory ram;
};

TEST_F(CustomPolicyTest, LoadedLFUEvictsLeastFrequentlyUsed)
{
    Cache cache(ram, 1, 1, 2,
                WritePolicy::WriteBack,
                AllocationPolicy::WriteAllocate,
                ReplacementPolicy::LRU);

    cache.LoadCustomPolicyScript(ResolveLfuScriptPath().string());

    // Fill the set with A and B.
    cache.ReadWord(0x00); // A
    cache.ReadWord(0x04); // B

    // Make A more frequent than B.
    cache.ReadWord(0x00);
    cache.ReadWord(0x00);
    cache.ReadWord(0x00);
    cache.ReadWord(0x04);

    // Force one eviction; LFU should evict B (lower frequency), not A.
    cache.ReadWord(0x08); // C

    const size_t hits_before = cache.GetHits();
    cache.ReadWord(0x00); // A should still be cached.
    EXPECT_EQ(cache.GetHits(), hits_before + 1);

    const size_t misses_before = cache.GetMisses();
    cache.ReadWord(0x04); // B should have been evicted.
    EXPECT_EQ(cache.GetMisses(), misses_before + 1);
}

TEST_F(CustomPolicyTest, AssemblyProgramMatchesLFUPatternAt1000)
{
    setupVmStateDirectory();

    auto vm = std::make_unique<RVSSVM>();
    Cache* l1_cache = vm->memory_controller_.GetL1Cache();

    l1_cache->Reconfigure(CacheConfig{
        1,
        1,
        2,
        WritePolicy::WriteThrough,
        AllocationPolicy::NoWriteAllocate,
        ReplacementPolicy::Custom});
    l1_cache->LoadCustomPolicyScript(ResolveLfuScriptPath().string());

    AssembledProgram program = assemble(ResolveAssemblyProgramPath().string());
    vm->LoadProgram(program);

    bool observed_final_state = false;
    for (int i = 0; i < 400; ++i)
    {
        vm->Step();
        if (vm->registers_.ReadGpr(30) == 111u && vm->registers_.ReadGpr(31) == 222u)
        {
            observed_final_state = true;
            break;
        }
    }
    EXPECT_TRUE(observed_final_state) << "Program did not reach expected final register state within step budget";

    // t2..t6 should hold values loaded from A/B/C/A/B respectively.
    EXPECT_EQ(vm->registers_.ReadGpr(7), 111u);   // t2 <- A
    EXPECT_EQ(vm->registers_.ReadGpr(28), 222u);  // t3 <- B
    EXPECT_EQ(vm->registers_.ReadGpr(29), 333u);  // t4 <- C
    EXPECT_EQ(vm->registers_.ReadGpr(30), 111u);  // t5 <- A
    EXPECT_EQ(vm->registers_.ReadGpr(31), 222u);  // t6 <- B

    const uint64_t tag_a = 0x1000 >> 2;
    const uint64_t tag_b = 0x1004 >> 2;

    bool found_a = false;
    bool found_b = false;
    uint64_t frequency_a = 0;
    uint64_t frequency_b = 0;

    for (size_t way = 0; way < l1_cache->GetNumWays(); ++way)
    {
        const auto& line = l1_cache->GetCacheLine(0, way);
        if (!line.valid)
        {
            continue;
        }

        if (line.tag == tag_a)
        {
            found_a = true;
            frequency_a = line.frequency;
        }
        else if (line.tag == tag_b)
        {
            found_b = true;
            frequency_b = line.frequency;
        }
    }

    EXPECT_TRUE(found_a);
    EXPECT_TRUE(found_b);

    // In this sequence with LFU, B is reinserted on the final access (miss).
    // For a word read miss in this cache implementation, the first byte misses and
    // the remaining 3 bytes hit the freshly inserted line, producing frequency = 3.
    EXPECT_GT(frequency_a, 0u);
    EXPECT_EQ(frequency_b, 3u);
}
