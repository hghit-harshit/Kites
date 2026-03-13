#pragma once

#include <vector>
#include <array>
#include <cstdint>
//#include "vm/memory_controller.h"
#include "vm/main_memory.h"
#include <QObject>
enum class WritePolicy
{
    WriteThrough,
    WriteBack
};

enum class AllocationPolicy
{
    WriteAllocate,
    NoWriteAllocate
};

class DirectMapCache;


static constexpr size_t LINE_SIZE = 8; // 8 bytes per cache line
static constexpr size_t CACHE_SIZE = 128; // 128 bytes of cache
static constexpr size_t NUM_LINES = CACHE_SIZE / LINE_SIZE; // Number of cache lines

struct CacheLine
{
    bool valid = false;
    bool dirty = false;
    uint64_t tag = 0;
    std::array<uint8_t, LINE_SIZE> data{};
};


class DirectMapCache : public QObject
{
    //Q_OBJECT
public:

    DirectMapCache(Memory& memory, size_t cache_size = CACHE_SIZE, size_t block_size = LINE_SIZE, WritePolicy write_policy = WritePolicy::WriteThrough, AllocationPolicy allocation_policy = AllocationPolicy::WriteAllocate)
        : write_policy_(write_policy), allocation_policy_(allocation_policy), memory_(memory)
    {
        cache_lines_.resize(NUM_LINES);
    }
    
    uint8_t Read(uint64_t address);
    template <typename T>
    T ReadGeneric(uint64_t address);
    template<typename T>
    void WriteCacheGeneric(uint64_t address, T value);
    
    void BringInCache(uint64_t address);
    
    void WriteByte(uint64_t address, uint8_t value);
    void WriteHalfWord(uint64_t address, uint16_t value);
    void WriteWord(uint64_t address, uint32_t value);
    void WriteDoubleWord(uint64_t address, uint64_t value);

    uint8_t ReadByte(uint64_t address);
    uint16_t ReadHalfWord(uint64_t address);
    uint32_t ReadWord(uint64_t address);
    uint64_t ReadDoubleWord(uint64_t address);

    void Reset();

    //Statistics
    size_t GetHits() const   { return hits_; }
    size_t GetMisses() const { return misses_; }

    inline CacheLine GetCacheLineByAddress(uint64_t address) const
    {
        size_t index = GetIndex(address);
        return cache_lines_[index];
    }
    
    inline CacheLine GetCacheLineByIndex(size_t index) const
    {
        if(index >= NUM_LINES)
        {
            throw std::out_of_range(std::string("Cache line index out of range: ") + std::to_string(index));
        }
        return cache_lines_[index];
    }
private:
    

    std::vector<CacheLine> cache_lines_;
    WritePolicy write_policy_;
    AllocationPolicy allocation_policy_;
    //MemoryController& memory_controller_; // Reference to the main memory controller    
    Memory& memory_; // Reference to the main memory

    static inline uint64_t GetTag(uint64_t address)  { return address / LINE_SIZE; }
    static inline size_t GetIndex(uint64_t address)  { return (address / LINE_SIZE) % NUM_LINES; }
    static inline size_t GetOffset(uint64_t address) { return address % LINE_SIZE; }

    CacheLine& GetCacheLine(uint64_t address)
    {
        size_t index = GetIndex(address);
        return cache_lines_[index];
    }

    //Statistics
    size_t hits_ = 0;
    size_t misses_ = 0;
// signals:
//     void CacheLineUpdatedSignal(uint64_t address);
};

