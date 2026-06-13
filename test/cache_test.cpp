// tests/cache_test.cpp
#include <gtest/gtest.h>
#include "vm/main_memory.h"
#include "vm/cache/cache.h"

// ── Fixture ───────────────────────────────────────────────────────────────────
class CacheTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // 4 sets, 4 words/line (16 bytes), 4 ways
        cache = std::make_unique<Cache>(
            ram,
            /*num_sets*/  4,
            /*block_size*/4,   // 4 words = 16 bytes per line
            /*num_ways*/  4,
            WritePolicy::WriteBack,
            AllocationPolicy::WriteAllocate,
            ReplacementPolicy::LRU);
    }

    Memory              ram;
    std::unique_ptr<Cache> cache;
};

// ── Basic read/write ──────────────────────────────────────────────────────────
TEST_F(CacheTest, WriteThenReadByte)
{
    cache->WriteByte(0x00, 0xAB);
    EXPECT_EQ(cache->ReadByte(0x00), 0xAB);
}

TEST_F(CacheTest, WriteThenReadHalfWord)
{
    cache->WriteHalfWord(0x00, 0xABCD);
    EXPECT_EQ(cache->ReadHalfWord(0x00), 0xABCD);
}

TEST_F(CacheTest, WriteThenReadWord)
{
    cache->WriteWord(0x00, 0xDEADBEEF);
    EXPECT_EQ(cache->ReadWord(0x00), 0xDEADBEEF);
}

TEST_F(CacheTest, WriteThenReadDoubleWord)
{
    cache->WriteDoubleWord(0x00, 0xCAFEBABEDEADBEEF);
    EXPECT_EQ(cache->ReadDoubleWord(0x00), 0xCAFEBABEDEADBEEF);
}

TEST_F(CacheTest, MultipleAddresses)
{
    cache->WriteWord(0x00, 0x11111111);
    cache->WriteWord(0x10, 0x22222222);
    cache->WriteWord(0x20, 0x33333333);
    EXPECT_EQ(cache->ReadWord(0x00), 0x11111111);
    EXPECT_EQ(cache->ReadWord(0x10), 0x22222222);
    EXPECT_EQ(cache->ReadWord(0x20), 0x33333333);
}

TEST_F(CacheTest, UnwrittenAddressReadsZero)
{
    EXPECT_EQ(cache->ReadByte(0x00), 0x00);
}

TEST_F(CacheTest, OverwriteSameAddress)
{
    cache->WriteWord(0x00, 0x11111111);
    cache->WriteWord(0x00, 0x22222222);
    EXPECT_EQ(cache->ReadWord(0x00), 0x22222222);
}

// ── Statistics ────────────────────────────────────────────────────────────────
TEST_F(CacheTest, HitCountOnRepeatedRead)
{
    cache->WriteWord(0x00, 0x12345678);  // miss
    cache->ReadWord(0x00);               // miss — cold
    cache->ReadWord(0x00);               // hit
    cache->ReadWord(0x00);               // hit
    EXPECT_EQ(cache->GetHits(),   3);
    EXPECT_EQ(cache->GetMisses(), 1);
}

TEST_F(CacheTest, HitRateCalculation)
{
    cache->WriteWord(0x00, 0xDEADBEEF);  // miss
    cache->ReadWord(0x00);               // miss
    cache->ReadWord(0x00);               // hit
    cache->ReadWord(0x00);               // hit
    EXPECT_DOUBLE_EQ(cache->GetHitRate(), 0.75);
    EXPECT_DOUBLE_EQ(cache->GetMissRate(), 0.25);
}

TEST_F(CacheTest, StatsResetOnReset)
{
    cache->WriteWord(0x00, 0x12345678);
    cache->ReadWord(0x00);
    cache->Reset();
    EXPECT_EQ(cache->GetHits(),   0);
    EXPECT_EQ(cache->GetMisses(), 0);
}

// ── Write policies ────────────────────────────────────────────────────────────
TEST_F(CacheTest, WriteBackDoesNotUpdateMemoryImmediately)
{
    cache->WriteWord(0x00, 0xDEADBEEF);
    EXPECT_EQ(ram.ReadWord(0x00), 0x00000000);
}

TEST_F(CacheTest, WriteBackFlushesToMemory)
{
    cache->WriteWord(0x00, 0xDEADBEEF);
    cache->Flush();
    EXPECT_EQ(ram.ReadWord(0x00), 0xDEADBEEF);
}

TEST_F(CacheTest, WriteThroughImmediatelyUpdatesMemory)
{
    Memory ram2;
    Cache wt_cache(ram2,
                   /*num_sets*/  4,
                   /*block_size*/4,
                   /*num_ways*/  4,
                   WritePolicy::WriteThrough,
                   AllocationPolicy::WriteAllocate,
                   ReplacementPolicy::LRU);
    wt_cache.WriteWord(0x00, 0xCAFEBABE);
    EXPECT_EQ(ram2.ReadWord(0x00), 0xCAFEBABE);
}

TEST_F(CacheTest, WriteThroughLineNotDirty)
{
    Memory ram2;
    Cache wt_cache(ram2, 4, 4, 4,
                   WritePolicy::WriteThrough,
                   AllocationPolicy::WriteAllocate,
                   ReplacementPolicy::LRU);
    wt_cache.WriteWord(0x00, 0xCAFEBABE);
    EXPECT_FALSE(wt_cache.GetCacheLine(0, 0).dirty);
}

// ── Allocation policies ───────────────────────────────────────────────────────
TEST_F(CacheTest, NoWriteAllocateGoesDirectToMemory)
{
    Memory ram2;
    Cache nwa_cache(ram2, 4, 4, 4,
                    WritePolicy::WriteBack,
                    AllocationPolicy::NoWriteAllocate,
                    ReplacementPolicy::LRU);
    nwa_cache.WriteWord(0x00, 0x12345678);
    EXPECT_EQ(ram2.ReadWord(0x00), 0x12345678);
}

TEST_F(CacheTest, NoWriteAllocateDoesNotBringLineIntoCache)
{
    Memory ram2;
    Cache nwa_cache(ram2, 4, 4, 4,
                    WritePolicy::WriteBack,
                    AllocationPolicy::NoWriteAllocate,
                    ReplacementPolicy::LRU);
    nwa_cache.WriteWord(0x00, 0x12345678);
    bool any_valid = false;
    for (size_t s = 0; s < nwa_cache.GetNumSets(); ++s)
        for (size_t w = 0; w < nwa_cache.GetNumWays(); ++w)
            if (nwa_cache.GetCacheLine(s, w).valid) any_valid = true;
    EXPECT_FALSE(any_valid);
}

// ── Eviction ──────────────────────────────────────────────────────────────────
TEST_F(CacheTest, EvictionWritesBackDirtyLine)
{
    // 4 sets, 1 word/line (4 bytes), 1 way — direct mapped
    // 0x00 and 0x10 map to same set (4 sets * 1 word * 4 bytes = 16 byte stride)
    Memory ram2;
    Cache dm_cache(ram2, 4, 1, 1,
                   WritePolicy::WriteBack,
                   AllocationPolicy::WriteAllocate,
                   ReplacementPolicy::LRU);

    dm_cache.WriteWord(0x00, 0xAAAAAAAA);  // dirty in set 0
    dm_cache.WriteWord(0x10, 0xBBBBBBBB); // evicts set 0 → writes back

    EXPECT_EQ(ram2.ReadWord(0x00), 0xAAAAAAAA);
}

TEST_F(CacheTest, CleanLineEvictedWithoutWriteback)
{
    Memory ram2;
    Cache dm_cache(ram2, 4, 1, 1,
                   WritePolicy::WriteBack,
                   AllocationPolicy::WriteAllocate,
                   ReplacementPolicy::LRU);

    dm_cache.ReadWord(0x00);   // bring in clean line
    dm_cache.ReadWord(0x10);   // evict — should NOT writeback
    EXPECT_EQ(ram2.ReadWord(0x00), 0x00000000);
}

// ── Reset / Flush ─────────────────────────────────────────────────────────────
TEST_F(CacheTest, ResetInvalidatesAllLines)
{
    cache->WriteWord(0x00, 0x12345678);
    cache->WriteWord(0x10, 0xDEADBEEF);
    cache->Reset();

    for (size_t s = 0; s < cache->GetNumSets(); ++s)
        for (size_t w = 0; w < cache->GetNumWays(); ++w)
            EXPECT_FALSE(cache->GetCacheLine(s, w).valid);
}

TEST_F(CacheTest, FlushWritesAllDirtyLines)
{
    cache->WriteWord(0x00, 0x11111111);
    cache->WriteWord(0x10, 0x22222222);
    cache->WriteWord(0x20, 0x33333333);
    cache->Flush();
    EXPECT_EQ(ram.ReadWord(0x00), 0x11111111);
    EXPECT_EQ(ram.ReadWord(0x10), 0x22222222);
    EXPECT_EQ(ram.ReadWord(0x20), 0x33333333);
}

TEST_F(CacheTest, FlushThenInvalidates)
{
    cache->WriteWord(0x00, 0x12345678);
    cache->Flush();
    for (size_t s = 0; s < cache->GetNumSets(); ++s)
        for (size_t w = 0; w < cache->GetNumWays(); ++w)
            EXPECT_FALSE(cache->GetCacheLine(s, w).valid);
}

TEST_F(CacheTest, CrossLineWriteDoesNotCorruptUnrelatedMemory)
{
    // 1-word lines force a word store at +2 to span two cache lines.
    Memory ram2;
    Cache dm_cache(ram2, 4, 1, 1,
                   WritePolicy::WriteBack,
                   AllocationPolicy::WriteAllocate,
                   ReplacementPolicy::LRU);

    dm_cache.WriteWord(0x02, 0xA1B2C3D4);
    dm_cache.Flush();

    EXPECT_EQ(ram2.ReadByte(0x02), 0xD4);
    EXPECT_EQ(ram2.ReadByte(0x03), 0xC3);
    EXPECT_EQ(ram2.ReadByte(0x04), 0xB2);
    EXPECT_EQ(ram2.ReadByte(0x05), 0xA1);

    // Nearby bytes outside write span should remain untouched.
    EXPECT_EQ(ram2.ReadByte(0x01), 0x00);
    EXPECT_EQ(ram2.ReadByte(0x06), 0x00);
    EXPECT_EQ(ram2.ReadByte(0x80), 0x00);
}

// ── Replacement policies ──────────────────────────────────────────────────────
class ReplacementPolicyTest : public ::testing::Test
{
protected:
    // 1 set, 1 word/line, 2 ways
    // 0x00 and 0x04 and 0x08 all map to set 0
    Memory ram;
};

TEST_F(ReplacementPolicyTest, LRUEvictsLeastRecentlyUsed)
{
    Cache cache(ram, 1, 1, 2,
                WritePolicy::WriteBack,
                AllocationPolicy::WriteAllocate,
                ReplacementPolicy::LRU);

    cache.ReadWord(0x00);   // way 0 — A
    cache.ReadWord(0x04);   // way 1 — B
    cache.ReadWord(0x00);   // touch A — B is now LRU
    cache.ReadWord(0x08);   // evicts B

    // A still in cache — hit
    size_t hits_before = cache.GetHits();
    cache.ReadWord(0x00);
    EXPECT_EQ(cache.GetHits(), hits_before + 1);

    // B was evicted — miss
    size_t misses_before = cache.GetMisses();
    cache.ReadWord(0x04);
    EXPECT_EQ(cache.GetMisses(), misses_before + 1);
}

#if 0
TEST_F(ReplacementPolicyTest, FIFOEvictsFirstInserted)
{
    Cache cache(ram, 1, 1, 2,
                WritePolicy::WriteBack,
                AllocationPolicy::WriteAllocate,
                ReplacementPolicy::FIFO);

    cache.ReadWord(0x00);   // A — inserted first
    cache.ReadWord(0x04);   // B
    cache.ReadWord(0x00);   // touch A — FIFO ignores hits
    cache.ReadWord(0x08);   // evicts A (first inserted)

    // A evicted — miss
    size_t misses_before = cache.GetMisses();
    cache.ReadWord(0x00);
    EXPECT_EQ(cache.GetMisses(), misses_before + 1);

    // B still in cache — hit
    size_t hits_before = cache.GetHits();
    cache.ReadWord(0x04);
    EXPECT_EQ(cache.GetHits(), hits_before + 1);
}
#endif

// ── L1/L2 hierarchy ───────────────────────────────────────────────────────────
class HierarchyTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        l2 = std::make_unique<Cache>(
            ram,
            /*num_sets*/  16,
            /*block_size*/4,
            /*num_ways*/  4,
            WritePolicy::WriteBack,
            AllocationPolicy::WriteAllocate,
            ReplacementPolicy::LRU);

        l1 = std::make_unique<Cache>(
            *l2,
            /*num_sets*/  4,
            /*block_size*/4,
            /*num_ways*/  2,
            WritePolicy::WriteBack,
            AllocationPolicy::WriteAllocate,
            ReplacementPolicy::LRU);
    }

    Memory              ram;
    std::unique_ptr<Cache> l2;
    std::unique_ptr<Cache> l1;
};

TEST_F(HierarchyTest, WriteGoesToL1)
{
    l1->WriteWord(0x00, 0xDEADBEEF);
    EXPECT_EQ(l1->ReadWord(0x00), 0xDEADBEEF);
}

TEST_F(HierarchyTest, WriteBackNotInMemoryUntilFlush)
{
    l1->WriteWord(0x00, 0xDEADBEEF);
    EXPECT_EQ(ram.ReadWord(0x00), 0x00000000);
}

TEST_F(HierarchyTest, L1FlushPropagatesL2)
{
    l1->WriteWord(0x00, 0xDEADBEEF);
    l1->Flush();
    EXPECT_EQ(l2->ReadWord(0x00), 0xDEADBEEF);
}

TEST_F(HierarchyTest, FullFlushPropagatesMemory)
{
    l1->WriteWord(0x00, 0xDEADBEEF);
    l1->Flush();
    l2->Flush();
    EXPECT_EQ(ram.ReadWord(0x00), 0xDEADBEEF);
}

TEST_F(HierarchyTest, L1MissFetchesFromL2)
{
    // write directly into L2 via its own write
    l2->WriteWord(0x00, 0x12345678);
    l2->Flush();  // push down to ram so L1 fetches cleanly

    uint32_t val = l1->ReadWord(0x00);
    EXPECT_EQ(val, 0x12345678);
    EXPECT_GE(l1->GetMisses(), 1);
}