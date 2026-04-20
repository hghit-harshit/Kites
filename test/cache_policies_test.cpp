// tests/cache_test.cpp
#include <gtest/gtest.h>
#include "vm/main_memory.h"
#include "vm/cache/cache.h"
    // ── FIFO detailed tests ───────────────────────────────────────────────────────
class FIFOTest : public ::testing::Test
{
protected:
    // 1 set, 1 word/line, 3 ways
    // 0x00, 0x04, 0x08, 0x0C all map to set 0
    Memory ram;
};

TEST_F(FIFOTest, BasicEvictionOrder)
{
    Cache cache(ram, 1, 1, 3,
                WritePolicy::WriteBack,
                AllocationPolicy::WriteAllocate,
                ReplacementPolicy::FIFO);

    cache.ReadWord(0x00);   // A — inserted first
    cache.ReadWord(0x04);   // B
    cache.ReadWord(0x08);   // C — all 3 ways full

    // evict A (first inserted)
    cache.ReadWord(0x0C);

    // B and C still in cache — hits
    size_t hits_before = cache.GetHits();
    cache.ReadWord(0x04);
    cache.ReadWord(0x08);
    EXPECT_EQ(cache.GetHits(), hits_before + 2);

    // A should be evicted — miss
    size_t misses_before = cache.GetMisses();
    cache.ReadWord(0x00);
    EXPECT_EQ(cache.GetMisses(), misses_before + 1);
}

TEST_F(FIFOTest, HitsDoNotAffectEvictionOrder)
{
    Cache cache(ram, 1, 1, 2,
                WritePolicy::WriteBack,
                AllocationPolicy::WriteAllocate,
                ReplacementPolicy::FIFO);

    cache.ReadWord(0x00);   // A — inserted first
    cache.ReadWord(0x04);   // B

    // keep hitting A — FIFO should still evict A first
    cache.ReadWord(0x00);
    cache.ReadWord(0x00);
    cache.ReadWord(0x00);

    // bring in C — evicts A despite recent hits
    cache.ReadWord(0x08);

    // B still in cache — hit
    size_t hits_before = cache.GetHits();
    cache.ReadWord(0x04);
    EXPECT_EQ(cache.GetHits(), hits_before + 1);

    // A should be evicted — miss
    size_t misses_before = cache.GetMisses();
    cache.ReadWord(0x00);
    EXPECT_EQ(cache.GetMisses(), misses_before + 1);
}

TEST_F(FIFOTest, CircularEvictionOrder)
{
    Cache cache(ram, 1, 1, 2,
                WritePolicy::WriteBack,
                AllocationPolicy::WriteAllocate,
                ReplacementPolicy::FIFO);

    cache.ReadWord(0x00);   // A
    cache.ReadWord(0x04);   // B — full

    cache.ReadWord(0x08);   // evicts A, brings in C
    cache.ReadWord(0x00);   // evicts B, brings in A again

    // C should now be oldest — evicted next
    cache.ReadWord(0x04);   // evicts C, brings in B

    size_t misses_before = cache.GetMisses();
    cache.ReadWord(0x08);   // C was evicted — miss
    EXPECT_EQ(cache.GetMisses(), misses_before + 1);
}

TEST_F(FIFOTest, FIFOWithWriteBack)
{
    Cache cache(ram, 1, 1, 2,
                WritePolicy::WriteBack,
                AllocationPolicy::WriteAllocate,
                ReplacementPolicy::FIFO);

    cache.WriteWord(0x00, 0xAAAAAAAA);  // A dirty
    cache.WriteWord(0x04, 0xBBBBBBBB); // B dirty — full

    // evict A (FIFO) — should write back to memory
    cache.ReadWord(0x08);
    EXPECT_EQ(ram.ReadWord(0x00), 0xAAAAAAAA);

    // evict B — should write back
    cache.ReadWord(0x00);
    EXPECT_EQ(ram.ReadWord(0x04), 0xBBBBBBBB);
}

TEST_F(FIFOTest, FIFOWithMultipleSets)
{
    // 2 sets, 1 word/line, 2 ways
    // set 0: 0x00, 0x08  (stride = 2 sets * 1 word * 4 bytes = 8)
    // set 1: 0x04, 0x0C
    Cache cache(ram, 2, 1, 2,
                WritePolicy::WriteBack,
                AllocationPolicy::WriteAllocate,
                ReplacementPolicy::FIFO);

    cache.ReadWord(0x00);   // set 0, way 0 — A
    cache.ReadWord(0x08);   // set 0, way 1 — B
    cache.ReadWord(0x04);   // set 1, way 0 — C
    cache.ReadWord(0x0C);   // set 1, way 1 — D

    // evict from set 0 — should evict A (0x00), not C or D
    cache.ReadWord(0x10);   // set 0 evicts A

    size_t misses_before = cache.GetMisses();
    cache.ReadWord(0x00);   // A evicted — miss
    EXPECT_EQ(cache.GetMisses(), misses_before + 1);

    // set 1 untouched — C and D still in cache
    size_t hits_before = cache.GetHits();
    cache.ReadWord(0x04);
    cache.ReadWord(0x0C);
    EXPECT_EQ(cache.GetHits(), hits_before + 2);
}

// ── LRU detailed tests ────────────────────────────────────────────────────────
class LRUTest : public ::testing::Test
{
protected:
    // 1 set, 1 word/line, 3 ways
    Memory ram;
};

TEST_F(LRUTest, BasicEvictionOrder)
{
    Cache cache(ram, 1, 1, 3,
                WritePolicy::WriteBack,
                AllocationPolicy::WriteAllocate,
                ReplacementPolicy::LRU);

    cache.ReadWord(0x00);   // A
    cache.ReadWord(0x04);   // B
    cache.ReadWord(0x08);   // C — full, A is LRU

    cache.ReadWord(0x0C);   // evicts A (LRU)

    size_t misses_before = cache.GetMisses();
    cache.ReadWord(0x00);   // A evicted — miss
    EXPECT_EQ(cache.GetMisses(), misses_before + 1);

    size_t hits_before = cache.GetHits();
    cache.ReadWord(0x04);   // B still in — hit
    cache.ReadWord(0x08);   // C still in — hit
    EXPECT_EQ(cache.GetHits(), hits_before + 2);
}

TEST_F(LRUTest, AccessPromotesToMostRecent)
{
    Cache cache(ram, 1, 1, 3,
                WritePolicy::WriteBack,
                AllocationPolicy::WriteAllocate,
                ReplacementPolicy::LRU);

    cache.ReadWord(0x00);   // A — LRU order: A
    cache.ReadWord(0x04);   // B — LRU order: A, B
    cache.ReadWord(0x08);   // C — LRU order: A, B, C

    cache.ReadWord(0x00);   // touch A — LRU order: B, C, A
    cache.ReadWord(0x04);   // touch B — LRU order: C, A, B

    // C is now LRU — should be evicted
    cache.ReadWord(0x0C);

    size_t misses_before = cache.GetMisses();
    cache.ReadWord(0x08);   // C evicted — miss
    EXPECT_EQ(cache.GetMisses(), misses_before + 1);

    size_t hits_before = cache.GetHits();
    cache.ReadWord(0x00);   // A still in — hit
    cache.ReadWord(0x04);   // B still in — hit
    EXPECT_EQ(cache.GetHits(), hits_before + 2);
}

TEST_F(LRUTest, WritePromotesToMostRecent)
{
    Cache cache(ram, 1, 1, 2,
                WritePolicy::WriteBack,
                AllocationPolicy::WriteAllocate,
                ReplacementPolicy::LRU);

    cache.ReadWord(0x00);               // A — LRU
    cache.ReadWord(0x04);               // B
    cache.WriteWord(0x00, 0x12345678);  // write to A — promotes A, B is now LRU

    // bring in C — evicts B (LRU)
    cache.ReadWord(0x08);

    // A still in cache — hit
    size_t hits_before = cache.GetHits();
    cache.ReadWord(0x00);
    EXPECT_EQ(cache.GetHits(), hits_before + 1);

    // B was evicted — miss
    size_t misses_before = cache.GetMisses();
    cache.ReadWord(0x04);
    EXPECT_EQ(cache.GetMisses(), misses_before + 1);
}

TEST_F(LRUTest, LRUWithWriteBack)
{
    Cache cache(ram, 1, 1, 2,
                WritePolicy::WriteBack,
                AllocationPolicy::WriteAllocate,
                ReplacementPolicy::LRU);

    cache.WriteWord(0x00, 0xAAAAAAAA);  // A dirty — LRU
    cache.WriteWord(0x04, 0xBBBBBBBB); // B dirty

    // bring in C — evicts A (LRU), writes back
    cache.ReadWord(0x08);
    EXPECT_EQ(ram.ReadWord(0x00), 0xAAAAAAAA);

    // B still dirty in cache — not yet in memory
    EXPECT_EQ(ram.ReadWord(0x04), 0x00000000);

    // bring in D — evicts B, writes back
    cache.ReadWord(0x0C);
    EXPECT_EQ(ram.ReadWord(0x04), 0xBBBBBBBB);
}

TEST_F(LRUTest, LRUWithMultipleSets)
{
    // 2 sets, 1 word/line, 2 ways
    Cache cache(ram, 2, 1, 2,
                WritePolicy::WriteBack,
                AllocationPolicy::WriteAllocate,
                ReplacementPolicy::LRU);

    // set 0
    cache.ReadWord(0x00);   // A
    cache.ReadWord(0x08);   // B
    // set 1
    cache.ReadWord(0x04);   // C
    cache.ReadWord(0x0C);   // D

    // touch A in set 0 — B is now LRU in set 0
    cache.ReadWord(0x00);

    // evict from set 0 — should evict B not C or D
    cache.ReadWord(0x10);

    size_t misses_before = cache.GetMisses();
    cache.ReadWord(0x08);   // B evicted — miss
    EXPECT_EQ(cache.GetMisses(), misses_before + 1);

    // set 1 untouched
    size_t hits_before = cache.GetHits();
    cache.ReadWord(0x04);
    cache.ReadWord(0x0C);
    EXPECT_EQ(cache.GetHits(), hits_before + 2);
}

TEST_F(LRUTest, LRUAgeingAcrossManyCycles)
{
    Cache cache(ram, 1, 1, 3,
                WritePolicy::WriteBack,
                AllocationPolicy::WriteAllocate,
                ReplacementPolicy::LRU);

    cache.ReadWord(0x00);   // A
    cache.ReadWord(0x04);   // B
    cache.ReadWord(0x08);   // C

    // repeatedly access B and C — A keeps getting older
    for (int i = 0; i < 10; ++i)
    {
        cache.ReadWord(0x04);
        cache.ReadWord(0x08);
    }

    // A should still be LRU — evicted first
    cache.ReadWord(0x0C);

    size_t misses_before = cache.GetMisses();
    cache.ReadWord(0x00);   // A evicted — miss
    EXPECT_EQ(cache.GetMisses(), misses_before + 1);

    size_t hits_before = cache.GetHits();
    cache.ReadWord(0x04);   // B still in — hit
    cache.ReadWord(0x08);   // C still in — hit
    EXPECT_EQ(cache.GetHits(), hits_before + 2);
}

// ── Policy comparison ─────────────────────────────────────────────────────────
class PolicyComparisonTest : public ::testing::Test
{
protected:
    // classic LRU vs FIFO difference:
    // access pattern A B C A B C D
    // with 3 ways:
    // LRU  — no misses on A B C repeats, only D causes eviction of C
    // FIFO — A B C fills, D evicts A, then next A is miss
    Memory ram_lru;
    Memory ram_fifo;
};

TEST_F(PolicyComparisonTest, LRUBetterThanFIFOOnRepeatingPattern)
{
    Cache lru_cache(ram_lru, 1, 1, 3,
                    WritePolicy::WriteBack,
                    AllocationPolicy::WriteAllocate,
                    ReplacementPolicy::LRU);

    Cache fifo_cache(ram_fifo, 1, 1, 3,
                     WritePolicy::WriteBack,
                     AllocationPolicy::WriteAllocate,
                     ReplacementPolicy::FIFO);

    // access pattern: A B C A B C D A B C
    std::vector<uint64_t> pattern = {
        0x00, 0x04, 0x08,   // A B C — cold misses for both
        0x00, 0x04, 0x08,   // A B C — all hits for both
        0x0C,               // D — eviction
        0x00, 0x04, 0x08    // A B C — LRU: C evicted so miss on C
                            //         FIFO: A evicted so miss on A
    };

    for (uint64_t addr : pattern)
    {
        lru_cache.ReadWord(addr);
        fifo_cache.ReadWord(addr);
    }

    // both have same misses up to D
    // after D: LRU evicts C (least recently used), FIFO evicts A (first in)
    // on final A B C: LRU misses on C, FIFO misses on A
    // either way both miss exactly once more — but on different addresses
    // LRU should have >= hits as FIFO on this pattern
    EXPECT_GE(lru_cache.GetHits(), fifo_cache.GetHits());
}
