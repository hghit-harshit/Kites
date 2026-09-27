#pragma once

#include <array>
#include <cstdint>
#include <deque>
#include <memory>
#include <string>
#include <vector>

// #include "processor/memory_controller.h"
#include "cacheconfig.h"
#include "policies/cache_replacement_policy.h"
#include "policies/custom_policy.h"
#include "processor/main_memory.h"
#include <QObject>
#include "processor/memory_device.h"
#include "common/undo_buffer.h"

namespace Kites
{

struct CacheLine
{
    uint64_t tag   = 0;

    // to be used by replacement policies
    uint64_t age        = 0;  // incremented on each access; used by policies
    uint64_t insertTime = 0;  // set when line is brought in
    uint64_t lastAccess = 0;  // updated on every access
    uint64_t frequency  = 0;  // access frequency counter
    std::vector<uint8_t> data{};
    bool     valid = false;
    bool     dirty = false;

    explicit CacheLine(size_t lineSizeInBytes) : data(lineSizeInBytes, 0)
    {
    }
};

// Everything one processor step changed in a cache, for undo/redo. A step can change
// several lines, or only counters (a no-write-allocate store miss touches no line).
struct CacheChange
{
    struct LineChange
    {
        size_t setIndex;
        size_t wayIndex;
        CacheLine oldCacheLine;
        CacheLine newCacheLine;
    };
    std::vector<LineChange> lines;

    size_t oldHitCount{0};
    size_t newHitCount{0};
    size_t oldMissCount{0};
    size_t newMissCount{0};
    size_t oldWriteBackCount{0};
    size_t newWriteBackCount{0};
    uint64_t oldTimestamp{0};
    uint64_t newTimestamp{0};
};
//default values for cache configuration
//maybe we will move its location later on
namespace default_cache_config
{
    constexpr size_t lineSizeinBytes              = 16;
    constexpr size_t setCount                     = 1;
    constexpr size_t wayCount                     = 2;
    constexpr WritePolicy writePolicy             = WritePolicy::WriteThrough;
    constexpr AllocationPolicy allocationPolicy   = AllocationPolicy::NoWriteAllocate;
    constexpr ReplacementPolicy replacementPolicy = ReplacementPolicy::LRU;
}

class Cache : public QObject, public MemoryDevice
{
    Q_OBJECT
public:
    // When next level is memory
    Cache(MemoryDevice &memory, size_t setCount = default_cache_config::setCount,
		size_t lineSizeInBytes = default_cache_config::lineSizeinBytes,
		size_t wayCount = default_cache_config::wayCount,
        WritePolicy writePolicy = default_cache_config::writePolicy,
        AllocationPolicy allocationPolicy = default_cache_config::allocationPolicy,
        ReplacementPolicy replacementPolicy = default_cache_config::replacementPolicy);

    Cache (const Cache &) = delete; // explicitly delete copy constructor


    void reconfigure(CacheConfig newConfig);

    // void BringInCache(uint64_t address);

    // public read write interface
    void writeByte(uint64_t address, uint8_t value) override;
    void writeHalfWord(uint64_t address, uint16_t value) override;
    void writeWord(uint64_t address, uint32_t value) override;
    void writeDoubleWord(uint64_t address, uint64_t value) override;

    //void writeFloat(uint64_t address, float value) override;
    //void writeDouble(uint64_t address, double value) override;

    uint8_t readByte(uint64_t address);
    uint16_t readHalfWord(uint64_t address);
    uint32_t readWord(uint64_t address);
    uint64_t readDoubleWord(uint64_t address);

    // The byte a read would return (this level if cached, else the next level), without
    // allocating lines, touching replacement state, counting hits/misses or emitting signals.
    uint8_t peekByte(uint64_t address) override;

    void reset();
    void flush(); // write back all dirty lines to memory and and invalidate all lines in cache

    // Undo/redo history, one entry per processor step. Between beginStep() and commitStep()
    // the first change to each line is recorded; commitStep() always pushes an entry so the
    // history stays in step with the processor's. undo/redoStep restore lines and counters
    // directly, without counting as accesses.
    void beginStep();
    void commitStep();
    void undoStep();
    void redoStep();
    void clearHistory();
    // Changes whenever the geometry is (re)set, which also clears the history.
    [[nodiscard]] uint64_t getGeneration() const;

    // Statistics
    [[nodiscard]]size_t getHitCount()  const;
    [[nodiscard]]size_t getMissCount() const;
    [[nodiscard]]size_t getWriteBackCount() const;
    [[nodiscard]]double getHitRate()   const;
    [[nodiscard]]double getMissRate()  const;
    [[nodiscard]]size_t getSetCount()  const;
    [[nodiscard]]size_t getWayCount()  const;
    [[nodiscard]]size_t getLineSizeInBytes() const;
    [[nodiscard]]size_t getCacheSizeInBytes() const;

    // Address Decomposition Helper Functions
    [[nodiscard]]uint64_t getTag(uint64_t address) const;
    [[nodiscard]]size_t getSetIndex(uint64_t address) const;
    [[nodiscard]]size_t getOffset(uint64_t address) const;

    CacheConfig getConfig() const;
    void updateStats();

    const CacheLine &getCacheLine(size_t setIndex, size_t wayIndex) const;
public slots:
    void loadCustomPolicyScript(const std::string &path);

private:
    std::span<const uint8_t> readLine(uint64_t address, size_t lineSize) override;
    void writeLine(uint64_t address, std::span<const uint8_t> data) override;
    // Cache Operations Helper Functions
    size_t findWay(size_t setIndex, uint64_t tag) const;
    void touchWay(size_t setIndex, size_t wayIndex);
    size_t evictCacheLine(size_t setIndex);
    void writeBack(size_t setIndex, size_t wayIndex);
    void bringIn(uint64_t address, size_t setIndex, size_t wayIndex);
    // Every change to a line goes through this, so a recorded step saves the line first.
    CacheLine &lineForWrite(size_t setIndex, size_t wayIndex);
    void restoreStep(const CacheChange &change, bool toNew);
    // These functions are used to read and write
    uint8_t getByteFromCache(uint64_t address);
    void putByteInCache(uint64_t address, uint8_t value);
    template <typename T> bool isHit(uint64_t address) const;
    template <typename T> void touchLines(uint64_t address);
    template <typename T> void bringInLines(uint64_t address);
    template <typename T> T readGeneric(uint64_t address);
    template <typename T> void writeGeneric(uint64_t address, T value);

    // Data Members
    MemoryDevice& m_nextLevelMemoryRef; //either memory or next level cache

    std::vector<std::vector<CacheLine>> m_sets; // each set contains wayCount cache lines
    uint64_t m_timestampCounter {0};           // cache-wide clock for replacement metadata

    size_t m_wayCount{0};
    size_t m_lineSizeInBytes{0};
    size_t m_setCount{0};
    WritePolicy m_writePolicy;
    AllocationPolicy m_allocationPolicy;
    std::unique_ptr<CacheReplacementPolicy> m_ReplacementPolicy;
    std::string m_customPolicyScriptPath;

    // precomputed bit masks for tag, index and offset
    size_t m_offsetBits{0};
    size_t m_setBits{0};
    uint64_t m_offsetMask{0};
    uint64_t m_setMask{0};

    // Statistics
    size_t m_hitCount  {0};
    size_t m_missCount {0};
    size_t m_writeBackCount {0};
    // common setup function
    void setupCache(size_t cache_size, size_t lineSizeInBytes,size_t wayCount);
    //TODO get this buffer size from config
    UndoBuffer<CacheChange> m_undoBuffer{100};
    CacheChange m_pendingChange;  // the step being recorded
    bool m_recording{false};
    uint64_t m_generation{0};
signals:
    //TODO : Maybe we can combine hit and miss into one signal with a bool parameter
    void cacheMissSignal(uint64_t address);
    void cacheHitSignal(uint64_t address);
    void cacheLineUpdatedSignal(uint64_t address);
    void cacheReconfiguredSignal(CacheConfig newConfig);
    void cacheStatsUpdatedSignal(CacheStats newStats);
    void customPolicyScriptLoadedSignal(bool success, const std::string &errorMessage = "");
};
}//namespace Kites
