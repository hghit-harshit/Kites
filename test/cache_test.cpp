// tests/cache_test.cpp
#include <gtest/gtest.h>
#include "processor/main_memory.h"
#include "processor/cache/cache.h"

using namespace Kites;

// ── Fixture ───────────────────────────────────────────────────────────────────
class CacheTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // 4 sets, 4 words/line (16 bytes), 4 ways
        cache = std::make_unique<Kites::Cache>(
            ram,
            /*num_sets*/  4,
            /*block_size*/16,  // 4 words = 16 bytes per line
            /*num_ways*/  4,
            Kites::WritePolicy::WriteBack,
            Kites::AllocationPolicy::WriteAllocate,
            Kites::ReplacementPolicy::LRU);
    }

    Kites::MainMemory              ram;
    std::unique_ptr<Kites::Cache> cache;
};

// ── Basic read/write ──────────────────────────────────────────────────────────
TEST_F(CacheTest, WriteThenReadByte)
{
    cache->writeByte(0x00, 0xAB);
    EXPECT_EQ(cache->readByte(0x00), 0xAB);
}

TEST_F(CacheTest, WriteThenReadHalfWord)
{
    cache->writeHalfWord(0x00, 0xABCD);
    EXPECT_EQ(cache->readHalfWord(0x00), 0xABCD);
}

TEST_F(CacheTest, WriteThenReadWord)
{
    cache->writeWord(0x00, 0xDEADBEEF);
    EXPECT_EQ(cache->readWord(0x00), 0xDEADBEEF);
}

TEST_F(CacheTest, WriteThenReadDoubleWord)
{
    cache->writeDoubleWord(0x00, 0xCAFEBABEDEADBEEF);
    EXPECT_EQ(cache->readDoubleWord(0x00), 0xCAFEBABEDEADBEEF);
}

TEST_F(CacheTest, MultipleAddresses)
{
    cache->writeWord(0x00, 0x11111111);
    cache->writeWord(0x10, 0x22222222);
    cache->writeWord(0x20, 0x33333333);
    EXPECT_EQ(cache->readWord(0x00), 0x11111111);
    EXPECT_EQ(cache->readWord(0x10), 0x22222222);
    EXPECT_EQ(cache->readWord(0x20), 0x33333333);
}

TEST_F(CacheTest, UnwrittenAddressReadsZero)
{
    EXPECT_EQ(cache->readByte(0x00), 0x00);
}

TEST_F(CacheTest, OverwriteSameAddress)
{
    cache->writeWord(0x00, 0x11111111);
    cache->writeWord(0x00, 0x22222222);
    EXPECT_EQ(cache->readWord(0x00), 0x22222222);
}

// ── Statistics ────────────────────────────────────────────────────────────────
TEST_F(CacheTest, HitCountOnRepeatedRead)
{
    cache->writeWord(0x00, 0x12345678);  // miss
    cache->readWord(0x00);               // miss — cold
    cache->readWord(0x00);               // hit
    cache->readWord(0x00);               // hit
    EXPECT_EQ(cache->getHitCount(),   3);
    EXPECT_EQ(cache->getMissCount(), 1);
}

TEST_F(CacheTest, HitRateCalculation)
{
    cache->writeWord(0x00, 0xDEADBEEF);  // miss
    cache->readWord(0x00);               // miss
    cache->readWord(0x00);               // hit
    cache->readWord(0x00);               // hit
    EXPECT_DOUBLE_EQ(cache->getHitRate(), 0.75);
    EXPECT_DOUBLE_EQ(cache->getMissRate(), 0.25);
}

TEST_F(CacheTest, StatsResetOnReset)
{
    cache->writeWord(0x00, 0x12345678);
    cache->readWord(0x00);
    cache->reset();
    EXPECT_EQ(cache->getHitCount(),   0);
    EXPECT_EQ(cache->getMissCount(), 0);
}

// ── Write policies ────────────────────────────────────────────────────────────
TEST_F(CacheTest, WriteBackDoesNotUpdateMemoryImmediately)
{
    cache->writeWord(0x00, 0xDEADBEEF);
    EXPECT_EQ(ram.readWord(0x00), 0x00000000);
}

TEST_F(CacheTest, WriteBackFlushesToMemory)
{
    cache->writeWord(0x00, 0xDEADBEEF);
    cache->flush();
    EXPECT_EQ(ram.readWord(0x00), 0xDEADBEEF);
}

TEST_F(CacheTest, WriteThroughImmediatelyUpdatesMemory)
{
    Kites::MainMemory ram2;
    Kites::Cache wt_cache(ram2,
                   /*num_sets*/  4,
                   /*block_size*/4,
                   /*num_ways*/  4,
                   Kites::WritePolicy::WriteThrough,
                   Kites::AllocationPolicy::WriteAllocate,
                   Kites::ReplacementPolicy::LRU);
    wt_cache.writeWord(0x00, 0xCAFEBABE);
    EXPECT_EQ(ram2.readWord(0x00), 0xCAFEBABE);
}

TEST_F(CacheTest, WriteThroughLineNotDirty)
{
    Kites::MainMemory ram2;
    Kites::Cache wt_cache(ram2, 4, 4, 4,
                   Kites::WritePolicy::WriteThrough,
                   Kites::AllocationPolicy::WriteAllocate,
                   Kites::ReplacementPolicy::LRU);
    wt_cache.writeWord(0x00, 0xCAFEBABE);
    EXPECT_FALSE(wt_cache.getCacheLine(0, 0).dirty);
}

// ── Allocation policies ───────────────────────────────────────────────────────
TEST_F(CacheTest, NoWriteAllocateGoesDirectToMemory)
{
    Kites::MainMemory ram2;
    Kites::Cache nwa_cache(ram2, 4, 4, 4,
                    Kites::WritePolicy::WriteBack,
                    Kites::AllocationPolicy::NoWriteAllocate,
                    Kites::ReplacementPolicy::LRU);
    nwa_cache.writeWord(0x00, 0x12345678);
    EXPECT_EQ(ram2.readWord(0x00), 0x12345678);
}

TEST_F(CacheTest, NoWriteAllocateDoesNotBringLineIntoCache)
{
    Kites::MainMemory ram2;
    Kites::Cache nwa_cache(ram2, 4, 4, 4,
                    Kites::WritePolicy::WriteBack,
                    Kites::AllocationPolicy::NoWriteAllocate,
                    Kites::ReplacementPolicy::LRU);
    nwa_cache.writeWord(0x00, 0x12345678);
    bool any_valid = false;
    for (size_t s = 0; s < nwa_cache.getSetCount(); ++s)
        for (size_t w = 0; w < nwa_cache.getWayCount(); ++w)
            if (nwa_cache.getCacheLine(s, w).valid) any_valid = true;
    EXPECT_FALSE(any_valid);
}

// ── Eviction ──────────────────────────────────────────────────────────────────
TEST_F(CacheTest, EvictionWritesBackDirtyLine)
{
    // 4 sets, 1 word/line (4 bytes), 1 way — direct mapped
    // 0x00 and 0x10 map to same set (4 sets * 1 word * 4 bytes = 16 byte stride)
    Kites::MainMemory ram2;
    Kites::Cache dm_cache(ram2, 4, 4, 1,
                   Kites::WritePolicy::WriteBack,
                   Kites::AllocationPolicy::WriteAllocate,
                   Kites::ReplacementPolicy::LRU);

    dm_cache.writeWord(0x00, 0xAAAAAAAA);  // dirty in set 0
    dm_cache.writeWord(0x10, 0xBBBBBBBB); // evicts set 0 → writes back

    EXPECT_EQ(ram2.readWord(0x00), 0xAAAAAAAA);
}

TEST_F(CacheTest, CleanLineEvictedWithoutWriteback)
{
    Kites::MainMemory ram2;
    Kites::Cache dm_cache(ram2, 4, 4, 1,
                   Kites::WritePolicy::WriteBack,
                   Kites::AllocationPolicy::WriteAllocate,
                   Kites::ReplacementPolicy::LRU);

    dm_cache.readWord(0x00);   // bring in clean line
    dm_cache.readWord(0x10);   // evict — should NOT writeback
    EXPECT_EQ(ram2.readWord(0x00), 0x00000000);
}

// ── Reset / Flush ─────────────────────────────────────────────────────────────
TEST_F(CacheTest, ResetInvalidatesAllLines)
{
    cache->writeWord(0x00, 0x12345678);
    cache->writeWord(0x10, 0xDEADBEEF);
    cache->reset();

    for (size_t s = 0; s < cache->getSetCount(); ++s)
        for (size_t w = 0; w < cache->getWayCount(); ++w)
            EXPECT_FALSE(cache->getCacheLine(s, w).valid);
}

TEST_F(CacheTest, FlushWritesAllDirtyLines)
{
    cache->writeWord(0x00, 0x11111111);
    cache->writeWord(0x10, 0x22222222);
    cache->writeWord(0x20, 0x33333333);
    cache->flush();
    EXPECT_EQ(ram.readWord(0x00), 0x11111111);
    EXPECT_EQ(ram.readWord(0x10), 0x22222222);
    EXPECT_EQ(ram.readWord(0x20), 0x33333333);
}

TEST_F(CacheTest, FlushThenInvalidates)
{
    cache->writeWord(0x00, 0x12345678);
    cache->flush();
    for (size_t s = 0; s < cache->getSetCount(); ++s)
        for (size_t w = 0; w < cache->getWayCount(); ++w)
            EXPECT_FALSE(cache->getCacheLine(s, w).valid);
}

TEST_F(CacheTest, CrossLineWriteDoesNotCorruptUnrelatedMemory)
{
    // 1-word lines force a word store at +2 to span two cache lines.
    Kites::MainMemory ram2;
    Kites::Cache dm_cache(ram2, 4, 4, 1,
                   Kites::WritePolicy::WriteBack,
                   Kites::AllocationPolicy::WriteAllocate,
                   Kites::ReplacementPolicy::LRU);

    dm_cache.writeWord(0x02, 0xA1B2C3D4);
    dm_cache.flush ();

    EXPECT_EQ(ram2.readByte(0x02), 0xD4);
    EXPECT_EQ(ram2.readByte(0x03), 0xC3);
    EXPECT_EQ(ram2.readByte(0x04), 0xB2);
    EXPECT_EQ(ram2.readByte(0x05), 0xA1);

    // Nearby bytes outside write span should remain untouched.
    EXPECT_EQ(ram2.readByte(0x01), 0x00);
    EXPECT_EQ(ram2.readByte(0x06), 0x00);
    EXPECT_EQ(ram2.readByte(0x80), 0x00);
}

// ── peekByte ──────────────────────────────────────────────────────────────────
TEST_F(CacheTest, PeekByteSeesDirtyDataWithoutSideEffects)
{
    cache->writeWord(0x00, 0x12345678); // dirty in the cache, memory still 0
    ram.writeByte(0x1000, 0x5A);        // only in memory

    const size_t hits_before   = cache->getHitCount();
    const size_t misses_before = cache->getMissCount();
    const Kites::CacheLine line_before = cache->getCacheLine(0, 0);

    EXPECT_EQ(cache->peekByte(0x00), 0x78); // from the dirty line, not memory
    EXPECT_EQ(cache->peekByte(0x03), 0x12);
    EXPECT_EQ(ram.readWord(0x00), 0u);
    EXPECT_EQ(cache->peekByte(0x1000), 0x5A); // falls through to memory

    EXPECT_EQ(cache->getHitCount(), hits_before);
    EXPECT_EQ(cache->getMissCount(), misses_before);
    const Kites::CacheLine &line_after = cache->getCacheLine(0, 0);
    EXPECT_EQ(line_after.frequency, line_before.frequency);
    EXPECT_EQ(line_after.lastAccess, line_before.lastAccess);
    EXPECT_EQ(line_after.age, line_before.age);

    // Peeking 0x1000 must not have brought its line in: a real read still misses.
    cache->readByte(0x1000);
    EXPECT_EQ(cache->getMissCount(), misses_before + 1);
}

// ── Accesses spanning two lines ──────────────────────────────────────────────

// Bug: a miss re-fetches every line the access touches, even ones already cached. The
// cached line is loaded again from memory into a second way, where it shadows the dirty
// copy (findWay returns the lowest way), so reads see the old value.
TEST_F(CacheTest, LineCrossingReadDoesNotDuplicateDirtyLine)
{
    // 1 set, 1 word/line, 2 ways
    Kites::MainMemory ram2;
    Kites::Cache cache2(ram2, 1, 4, 2,
                        Kites::WritePolicy::WriteBack,
                        Kites::AllocationPolicy::WriteAllocate,
                        Kites::ReplacementPolicy::LRU);

    cache2.readWord(0x200);      // X -> way 0
    cache2.writeWord(0x100, 5);  // A -> way 1, dirty (memory still 0)

    // Touches A (cached) and 0x104 (not cached), so it counts as a miss.
    EXPECT_EQ(cache2.readDoubleWord(0x100) & 0xFFFFFFFF, 5u);
    EXPECT_EQ(cache2.readWord(0x100), 5u);
}

// Bug: bringing in the second line of an access can evict the first line of the same
// access, when both map to one set with too few ways. getByteFromCache then asserts
// (Debug; this aborts the test process) or reads out of bounds (Release).
TEST_F(CacheTest, LineCrossingReadDoesNotEvictItsOwnLine)
{
    // 1 set, 1 word/line, 1 way
    Kites::MainMemory ram2;
    Kites::Cache cache2(ram2, 1, 4, 1,
                        Kites::WritePolicy::WriteBack,
                        Kites::AllocationPolicy::WriteAllocate,
                        Kites::ReplacementPolicy::LRU);

    ram2.writeDoubleWord(0x00, 0x0807060504030201);
    EXPECT_EQ(cache2.readWord(0x02), 0x06050403u);
}

// Bug: with no-write-allocate, a store that only partly hits is treated as a miss and
// written to the next level only, so the cached line keeps the old bytes.
TEST_F(CacheTest, NoWriteAllocatePartialHitUpdatesCachedLine)
{
    // 1 set, 1 word/line, 2 ways (the default write/allocate policies)
    Kites::MainMemory ram2;
    Kites::Cache cache2(ram2, 1, 4, 2,
                        Kites::WritePolicy::WriteThrough,
                        Kites::AllocationPolicy::NoWriteAllocate,
                        Kites::ReplacementPolicy::LRU);

    cache2.readWord(0x100);  // caches line 0x100
    cache2.writeDoubleWord(0x100, 0x1111111122222222);

    EXPECT_EQ(ram2.readWord(0x100), 0x22222222u);   // memory is right
    EXPECT_EQ(cache2.readWord(0x100), 0x22222222u); // cached line is stale
}

// ── Replacement policies ──────────────────────────────────────────────────────
class ReplacementPolicyTest : public ::testing::Test
{
protected:
    // 1 set, 1 word/line, 2 ways
    // 0x00 and 0x04 and 0x08 all map to set 0
    Kites::MainMemory ram;
};

TEST_F(ReplacementPolicyTest, LRUEvictsLeastRecentlyUsed)
{
    Kites::Cache cache(ram, 1, 4, 2,
                Kites::WritePolicy::WriteBack,
                Kites::AllocationPolicy::WriteAllocate,
                Kites::ReplacementPolicy::LRU);

    cache.readWord(0x00);   // way 0 — A
    cache.readWord(0x04);   // way 1 — B
    cache.readWord(0x00);   // touch A — B is now LRU
    cache.readWord(0x08);   // evicts B

    // A still in cache — hit
    size_t hits_before = cache.getHitCount();
    cache.readWord(0x00);
    EXPECT_EQ(cache.getHitCount(), hits_before + 1);

    // B was evicted — miss
    size_t misses_before = cache.getMissCount();
    cache.readWord(0x04);
    EXPECT_EQ(cache.getMissCount(), misses_before + 1);
}

#if 0
TEST_F(ReplacementPolicyTest, FIFOEvictsFirstInserted)
{
    Kites::Cache cache(ram, 1, 4, 2,
                Kites::WritePolicy::WriteBack,
                Kites::AllocationPolicy::WriteAllocate,
                Kites::ReplacementPolicy::FIFO);

    cache.readWord(0x00);   // A — inserted first
    cache.readWord(0x04);   // B
    cache.readWord(0x00);   // touch A — FIFO ignores hits
    cache.readWord(0x08);   // evicts A (first inserted)

    // A evicted — miss
    size_t misses_before = cache.getMissCount();
    cache.readWord(0x00);
    EXPECT_EQ(cache.getMissCount(), misses_before + 1);

    // B still in cache — hit
    size_t hits_before = cache.getHitCount();
    cache.readWord(0x04);
    EXPECT_EQ(cache.getHitCount(), hits_before + 1);
}
#endif

// ── L1/L2 hierarchy ───────────────────────────────────────────────────────────
class HierarchyTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        l2 = std::make_unique<Kites::Cache>(
            ram,
            /*num_sets*/  16,
            /*block_size*/4,
            /*num_ways*/  4,
            Kites::WritePolicy::WriteBack,
            Kites::AllocationPolicy::WriteAllocate,
            Kites::ReplacementPolicy::LRU);

        l1 = std::make_unique<Kites::Cache>(
            *l2,
            /*num_sets*/  4,
            /*block_size*/4,
            /*num_ways*/  2,
            Kites::WritePolicy::WriteBack,
            Kites::AllocationPolicy::WriteAllocate,
            Kites::ReplacementPolicy::LRU);
    }

    Kites::MainMemory             ram;
    std::unique_ptr<Kites::Cache> l2;
    std::unique_ptr<Kites::Cache> l1;
};

TEST_F(HierarchyTest, WriteGoesToL1)
{
    l1->writeWord(0x00, 0xDEADBEEF);
    EXPECT_EQ(l1->readWord(0x00), 0xDEADBEEF);
}

TEST_F(HierarchyTest, WriteBackNotInMemoryUntilFlush)
{
    l1->writeWord(0x00, 0xDEADBEEF);
    EXPECT_EQ(ram.readWord(0x00), 0x00000000);
}

TEST_F(HierarchyTest, L1FlushPropagatesL2)
{
    l1->writeWord(0x00, 0xDEADBEEF);
    l1->flush();
    EXPECT_EQ(l2->readWord(0x00), 0xDEADBEEF);
}

TEST_F(HierarchyTest, FullFlushPropagatesMemory)
{
    l1->writeWord(0x00, 0xDEADBEEF);
    l1->flush();
    l2->flush();
    EXPECT_EQ(ram.readWord(0x00), 0xDEADBEEF);
}

TEST_F(HierarchyTest, L1MissFetchesFromL2)
{
    // write directly into L2 via its own write
    l2->writeWord(0x00, 0x12345678);
    l2->flush();  // push down to ram so L1 fetches cleanly

    uint32_t val = l1->readWord(0x00);
    EXPECT_EQ(val, 0x12345678);
    EXPECT_GE(l1->getMissCount(), 1);
}
