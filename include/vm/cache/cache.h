#pragma once

#include <vector>
#include <array>
#include <cstdint>
#include <deque>
//#include "vm/memory_controller.h"
#include "vm/main_memory.h"
#include <QObject>
#include "cacheconfig.h"

class DirectMapCache;


static constexpr size_t LINE_SIZE = 8; // 8 bytes per cache line
static constexpr size_t CACHE_SIZE = 128; // 128 bytes of cache
static constexpr size_t NUM_LINES = CACHE_SIZE / LINE_SIZE; // Number of cache lines

struct CacheLine
{
    bool valid   = false;
    bool dirty   = false;
    uint64_t tag = 0;
    std::vector<uint8_t> data{};

    explicit CacheLine(size_t block_size) : data(block_size, 0) {}
};

/**
 * @brief This class stores metadata for each cache set,
 * including LRU counters and FIFO queues for replacement policies.
 * 
 */
struct SetMetaData
{
    std::vector<uint64_t> lru_counter;
    std::deque<size_t>    fifo_queue;
    uint64_t              timestamp_counter = 0; // for LRU tracking

    explicit SetMetaData(size_t num_ways) : lru_counter(num_ways, 0) {}
};

class Cache : public QObject
{
    Q_OBJECT
public:

    //When next level is memory
    Cache(Memory& memory, 
        size_t cache_size                    = CACHE_SIZE, 
        size_t block_size                    = LINE_SIZE, 
        size_t num_ways                      = 1,
        WritePolicy write_policy             = WritePolicy::WriteThrough, 
        AllocationPolicy allocation_policy   = AllocationPolicy::WriteAllocate,
        ReplacementPolicy replacement_policy = ReplacementPolicy::LRU);
        
    //When next level is another cache
    Cache(Cache& next_level_cache, 
        size_t cache_size                    = CACHE_SIZE, 
        size_t block_size                    = LINE_SIZE, 
        size_t num_ways                      = 1,
        WritePolicy write_policy             = WritePolicy::WriteThrough, 
        AllocationPolicy allocation_policy   = AllocationPolicy::WriteAllocate,
        ReplacementPolicy replacement_policy = ReplacementPolicy::LRU);

    void Reconfigure(CacheConfig new_config);
    
    //void BringInCache(uint64_t address);
    
    //public read write interface
    void WriteByte(uint64_t address, uint8_t value);
    void WriteHalfWord(uint64_t address, uint16_t value);
    void WriteWord(uint64_t address, uint32_t value);
    void WriteDoubleWord(uint64_t address, uint64_t value);

    uint8_t ReadByte(uint64_t address);
    uint16_t ReadHalfWord(uint64_t address);
    uint32_t ReadWord(uint64_t address);
    uint64_t ReadDoubleWord(uint64_t address);


    void Reset();
    void Flush(); // write back all dirty lines to memory and and invalidate all lines in cache

    //Statistics
    size_t GetHits() const   { return hits_; }
    size_t GetMisses() const { return misses_; }

    size_t GetNumSets() const { return num_sets_; }
    size_t GetNumWays() const { return num_ways_; }
    size_t GetBlockSize() const { return block_size_; }

    const CacheLine& GetCacheLine(size_t set_index, size_t way_index) const;
    

    // inline CacheLine GetCacheLineByAddress(uint64_t address) const
    // {
    //     size_t index = GetIndex(address);
    //     return cache_lines_[index];
    // }
    
    // inline CacheLine GetCacheLineByIndex(size_t index) const
    // {
    //     if(index >= NUM_LINES)
    //     {
    //         throw std::out_of_range(std::string("Cache line index out of range: ") + std::to_string(index));
    //     }
    //     return cache_lines_[index];
    // }
private:
    // Next Level Helper Functions
    uint8_t  ReadFromNextLevel(uint64_t address);
    void     WriteByteToNextLevel(uint64_t address, uint8_t value);
    uint64_t SizeofNextLevel() const;


    //Address Decomposition Helper Functions
    inline uint64_t GetTag(uint64_t address)      const { return address >> (offset_bits_ + set_bits_); }
    inline size_t   GetSetIndex(uint64_t address) const { return static_cast<size_t>(address >> offset_bits_) & set_mask_; }
    inline size_t   GetOffset(uint64_t address)   const { return static_cast<size_t>(address & offset_mask_); }


    // Cache Operations Helper Functions
    size_t FindWay  (size_t set_index, uint64_t tag) const;
    void   TouchWay (size_t set_index, size_t way_index);
    size_t EvictWay (size_t set_index);
    void   WriteBack(size_t set_index, size_t way_index);
    void   BringIn  (uint64_t address, size_t set_index, size_t way_index);
    

    uint8_t Read(uint64_t address);
    template <typename T> T ReadGeneric(uint64_t address);
    template<typename T> void WriteCacheGeneric(uint64_t address, T value);

    //Data Members
    Memory* memory_; // Reference to the main memory
    Cache*  next_level_cache_; // Pointer to the next level cache (if any)
    
    std::vector<std::vector<CacheLine>> sets_; // each set contains num_ways cache lines
    std::vector<SetMetaData> sets_metadata_;   // metadata for each set
    
    size_t             num_ways_;
    size_t             block_size_;
    size_t             num_sets_;
    WritePolicy        write_policy_;
    AllocationPolicy   allocation_policy_;
    ReplacementPolicy  replacement_policy_;

    //precomputed bit masks for tag, index and offset
    size_t   offset_bits_ ;
    size_t   set_bits_ ;
    uint64_t offset_mask_ ;
    uint64_t set_mask_ ;

    //Statistics
    size_t hits_ = 0;
    size_t misses_ = 0;

    void SetupCache(size_t cache_size, size_t block_size, size_t num_ways); //common setup function for both constructors
    signals:
        void CacheLineUpdatedSignal(uint64_t address);
        void CacheReconfiguredSignal();
};