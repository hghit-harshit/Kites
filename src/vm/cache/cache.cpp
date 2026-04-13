#include "vm/cache/cache.h"

void Cache::SetupCache(size_t num_sets, size_t block_size, size_t num_ways)
{

    //assert(cache_size % (block_size * num_ways) == 0 && "Cache size must be divisible by block size times number of ways");

    num_sets_ = num_sets;
    block_size_ = block_size;
    num_ways_ = num_ways;

    offset_bits_ = std::countr_zero(block_size*4); // number of bytes in a cache line, assuming block_size is in words (4 bytes)
    set_bits_ = std::countr_zero(num_sets_);
    offset_mask_ = (block_size*4) - 1;
    set_mask_ = num_sets_ - 1;

    sets_.clear();
    sets_metadata_.clear();
    sets_.reserve(num_sets_);
    sets_metadata_.reserve(num_sets_);
    for(size_t i = 0; i < num_sets_; ++i)
    {
        sets_.emplace_back(num_ways_, CacheLine(block_size_));
        sets_metadata_.emplace_back(num_ways_ );
    }

}

Cache::Cache(Memory& memory, size_t  num_sets, 
    size_t block_size, size_t num_ways, WritePolicy write_policy, 
    AllocationPolicy allocation_policy, ReplacementPolicy replacement_policy)
    : memory_(&memory), 
    next_level_cache_(nullptr), 
    write_policy_(write_policy), 
    allocation_policy_(allocation_policy), 
    replacement_policy_(replacement_policy)
{
    SetupCache(num_sets, block_size, num_ways);
}

Cache::Cache(Cache& next_level_cache, size_t num_sets, 
    size_t block_size, size_t num_ways, WritePolicy write_policy, 
    AllocationPolicy allocation_policy, ReplacementPolicy replacement_policy)
    : memory_(nullptr), 
    next_level_cache_(&next_level_cache), 
    write_policy_(write_policy), 
    allocation_policy_(allocation_policy), 
    replacement_policy_(replacement_policy)
{
    SetupCache(num_sets, block_size, num_ways);
}

void Cache::Reconfigure(CacheConfig new_config)
{
    Flush(); // write back all dirty lines to memory and invalidate cache before reconfiguring
    write_policy_ = new_config.write_policy;
    allocation_policy_ = new_config.allocation_policy;
    replacement_policy_ = new_config.replacement_policy;
    SetupCache(new_config.num_lines, new_config.block_size, new_config.num_ways);
    emit CacheReconfiguredSignal(new_config); 
}

const CacheLine& Cache::GetCacheLine(size_t set_index, size_t way_index) const
{
    if(set_index >= num_sets_ || way_index >= num_ways_)
    {
        throw std::out_of_range(std::string("Cache line index out of range: set_index=") + std::to_string(set_index) + ", way_index=" + std::to_string(way_index));
    }
    return sets_[set_index][way_index];
}


//Next Leve Helper Functions
uint8_t Cache::ReadFromNextLevel(uint64_t address)
{
    if(next_level_cache_)
    {
        return next_level_cache_->ReadByte(address);
    }
    else
    {
        return memory_->ReadByte(address);
    }
}

void Cache::WriteByteToNextLevel(uint64_t address, uint8_t value)
{
    if(next_level_cache_)
    {
        next_level_cache_->WriteByte(address, value);
    }
    else
    {
        memory_->WriteByte(address, value);
    }
}

size_t Cache::SizeofNextLevel() const
{
    if(next_level_cache_)
    {
        return next_level_cache_->SizeofNextLevel();
    }
    else
    {
        return vm_config::config.getMemorySize();
    }
}

/**
 * @brief  Returns the way index of the cache line in the specified set that matches the given tag.
 * If no such line exists, returns num_ways_ to indicate a miss.
 */
size_t Cache::FindWay(size_t set_index, uint64_t tag) const
{
    const auto& set = sets_[set_index];
    for(size_t way = 0; way < num_ways_; ++way)
    {
        if(set[way].valid && set[way].tag == tag)
        {
            return way;
        }
    }
    return num_ways_; // not found
}

void Cache::TouchWay(size_t set_index, size_t way_index)
{
    if(replacement_policy_ == ReplacementPolicy::LRU)
    {
        sets_metadata_[set_index].lru_counter[way_index] = ++sets_metadata_[set_index].timestamp_counter;
    }
    
}

/**
 * @brief 
 * Evicts a cache line from the specified set based on the replacement policy and returns the way index of the evicted line.
 * If there is an invalid line, it will be chosen for eviction without considering the replacement policy.
 */
size_t Cache::EvictWay(size_t set_index)
{
    auto& ways     = sets_[set_index];
    auto& metadata = sets_metadata_[set_index];
    
    // always prefer to evict an invalid line if available
    for(size_t way = 0; way < num_ways_; ++way)
    {
        if(!ways[way].valid)
        {
            if(replacement_policy_ == ReplacementPolicy::FIFO)
            {
                metadata.fifo_queue.push_back(way);
            }
            return way;
        }
    }

    size_t victim = 0;
    switch(replacement_policy_)
    {
        case ReplacementPolicy::LRU:
        {
            victim = std::min_element(metadata.lru_counter.begin(), metadata.lru_counter.end()) - metadata.lru_counter.begin();
            break;
        }
        case ReplacementPolicy::FIFO:
        {
            victim = metadata.fifo_queue.front();
            metadata.fifo_queue.pop_front();
            metadata.fifo_queue.push_back(victim);
            break;
        }
    }

    if(ways[victim].dirty && write_policy_ == WritePolicy::WriteBack)
    {
        WriteBack(set_index, victim);
    }

    ways[victim].valid = false; // invalidate the line before bringing in new data
    return victim;
}

void Cache::WriteBack(size_t set_index, size_t way_index)
{
    CacheLine& line = sets_[set_index][way_index];
    if(line.valid && line.dirty)
    {
        uint64_t block_start_address = (line.tag << (set_bits_ + offset_bits_)) | (set_index << offset_bits_);
        for(size_t i = 0; i < block_size_*4; ++i)
        {
            WriteByteToNextLevel(block_start_address + i, line.data[i]);
        }
        line.dirty = false;
    }
}

void Cache::BringIn(uint64_t address, size_t set_index, size_t way_index)
{
    CacheLine& line = sets_[set_index][way_index];
    uint64_t block_start_address = address & ~(offset_mask_); // align address to block boundary

    line.valid = true;
    line.dirty = false;
    line.tag = GetTag(address);
    for(size_t i = 0; i < block_size_; ++i)
    {
        line.data[i] = ReadFromNextLevel(block_start_address + i);
    }

    SetMetaData& metadata = sets_metadata_[set_index];

    switch(replacement_policy_)
    {
        case ReplacementPolicy::LRU:
        {
            metadata.lru_counter[way_index] = ++metadata.timestamp_counter;
            break;
        }
        case ReplacementPolicy::FIFO:
        {
            // If the line was brought in due to an eviction, it would have already been added to the FIFO queue in EvictWay.
            // If it was brought in due to a miss on an invalid line, we need to add it to the FIFO queue now.
            if(std::find(metadata.fifo_queue.begin(), metadata.fifo_queue.end(), way_index) == metadata.fifo_queue.end())
            {
                metadata.fifo_queue.push_back(way_index);
            }
            break;
        }
    }
}

template <typename T>
T Cache::ReadGeneric(uint64_t address)
{
    if(address >= vm_config::config.getMemorySize() - (sizeof(T) - 1))
    {
        throw std::out_of_range(std::string("Cache read address out of range: ") + std::to_string(address));
    }

    size_t set_index = GetSetIndex(address);
    uint64_t tag = GetTag(address);
    size_t offset = GetOffset(address);
    size_t way_index = FindWay(set_index, tag);

    if(way_index < num_ways_)
    {
        // Cache hit
        hits_++;
        TouchWay(set_index, way_index);
    }
    else
    {
        // Cache miss
        misses_++;
        way_index = EvictWay(set_index);
        //WriteBack(set_index, way_index);
        BringIn(address, set_index, way_index);
    }
    const CacheLine& line = GetCacheLine(set_index, way_index);
    // since this function will only be called by the specific read functions and that to when line exists and is valid, 
    //we can skip the check for valid and tag match
    T value = 0;
    for (size_t i = 0; i < sizeof(T); ++i) {
        value |= static_cast<T>(line.data[GetOffset(address) + i]) << (8*i);
    }
    return value;
}

template <typename T>
void Cache::WriteCacheGeneric(uint64_t address, T value)
{
    if(address >= vm_config::config.getMemorySize() - (sizeof(T) - 1))
    {
        throw std::out_of_range(std::string("Cache write address out of range: ") + std::to_string(address));
    }

    size_t set_index = GetSetIndex(address);
    uint64_t tag = GetTag(address);
    size_t offset = GetOffset(address);
    size_t way_index = FindWay(set_index, tag);

    if(way_index == num_ways_)
    {
        ++misses_;
        if(allocation_policy_ == AllocationPolicy::NoWriteAllocate)
        {
            for(size_t i = 0; i < sizeof(T); ++i)
            {
                WriteByteToNextLevel(address + i,static_cast<uint8_t>((value >> (8*i)) & 0xFF));
            }
            return;
        }

        way_index = EvictWay(set_index);
        //WriteBack(set_index, way_index); // remove this when write back is implemented in the eviction function
        BringIn(address, set_index, way_index);
    }
    else
    {
        ++hits_;
        TouchWay(set_index, way_index);
    }

    CacheLine& line = sets_[set_index][way_index];

    for(size_t i = 0; i < sizeof(T); ++i)
    {
        line.data[offset + i] = static_cast<uint8_t>((value >> (8*i)) & 0xFF);
    }

    if(write_policy_ == WritePolicy::WriteThrough)
    {
        for(size_t i = 0; i < sizeof(T); ++i)
        {
            WriteByteToNextLevel(address + i, static_cast<uint8_t>((value >> (8*i)) & 0xFF));
        }
        line.dirty = false;
    }
    else
    {
        line.dirty = true;
    }
}

uint8_t Cache::ReadByte(uint64_t address)
{
    return ReadGeneric<uint8_t>(address);
}

uint16_t Cache::ReadHalfWord(uint64_t address)
{
    return ReadGeneric<uint16_t>(address);
}

uint32_t Cache::ReadWord(uint64_t address)
{
    return ReadGeneric<uint32_t>(address);
}  

uint64_t Cache::ReadDoubleWord(uint64_t address)
{
    return ReadGeneric<uint64_t>(address);
}

void Cache::WriteByte(uint64_t address, uint8_t value)
{
    
    WriteCacheGeneric<uint8_t>(address, value);
    emit CacheLineUpdatedSignal(address);
}

void Cache::WriteHalfWord(uint64_t address, uint16_t value)
{
    WriteCacheGeneric<uint16_t>(address, value);
    emit CacheLineUpdatedSignal(address);
}

void Cache::WriteWord(uint64_t address, uint32_t value)
{
    WriteCacheGeneric<uint32_t>(address, value);
    emit CacheLineUpdatedSignal(address);

}

void Cache::WriteDoubleWord(uint64_t address, uint64_t value)
{
    WriteCacheGeneric<uint64_t>(address, value);
    emit CacheLineUpdatedSignal(address);

}

void Cache::Reset()
{
    for (auto& set : sets_)
    {
        for (auto& line : set)
        {
            line.valid = false;
            line.dirty = false;
            std::fill(std::begin(line.data), std::end(line.data), 0);
        }
    }
    hits_ = 0;
    misses_ = 0;
}

void Cache::Flush()
{
    for(size_t set_index = 0; set_index < num_sets_; ++set_index)
    {
        for(size_t way_index = 0; way_index < num_ways_; ++way_index)
        {
            if(sets_[set_index][way_index].valid && sets_[set_index][way_index].dirty)
            {
                WriteBack(set_index, way_index);
            }
        }
    }
    Reset();
}

// uint8_t Cache::Read(uint64_t address)
// {
//     if(address >= vm_config::config.getMemorySize())
//     {
//         throw std::out_of_range(std::string("Cache read address out of range: ") + std::to_string(address));
//     }
//     CacheLine& line = GetCacheLine(address);
//     // since this function will only be called by the generic and that to 
//when line exists and is valid, we can skip the check for valid and tag match
//     return line.data[GetOffset(address)];

// }


// void Cache::BringInCache(uint64_t address)
// {
//     if(address >= vm_config::config.getMemorySize())
//     {
//         throw std::out_of_range(std::string("Cache bring in address out of range: ") + std::to_string(address));
//     }
   
//     uint64_t block_start_address = address - GetOffset(address);
//     CacheLine& line = GetCacheLine(address);
//     if(line.valid && line.dirty && line.tag != GetTag(address) && write_policy_ == WritePolicy::WriteBack)
//     {
//         //write back to memory
//         uint64_t block_start_address = (line.tag * LINE_SIZE);
//         for(size_t i = 0; i < LINE_SIZE; ++i)
//         {
//             memory_.WriteByte(block_start_address + i, line.data[i]);
//         }
//     }
//     line.valid = true;
//     line.dirty = false;
//     line.tag = GetTag(address);
//     for(size_t i = 0; i < LINE_SIZE; ++i)
//     {
//         line.data[i] = memory_.ReadByte(block_start_address + i);
//     }
// }




// uint8_t Cache::ReadByte(uint64_t address)
// {
//     CacheLine& line = GetCacheLine(address);
//     if(line.valid && line.tag == GetTag(address))
//     {
//         hits_++;
//         return Read(address);
//     }
//     else
//     {
//         misses_++;
//         uint8_t value = memory_.ReadByte(address);
//         BringInCache(address);
//         return value;
//     }
// }

// uint16_t Cache::ReadHalfWord(uint64_t address)
// {
//     // Implementation of half-word read from cache
//     CacheLine& line = GetCacheLine(address);
//     if(line.valid && line.tag == GetTag(address))
//     {
//         hits_++;
//         return ReadGeneric<uint16_t>(address);
//     }
//     else
//     {
//         misses_++;
//         uint16_t value = memory_.ReadHalfWord(address);
//         BringInCache(address);
//         return value;
//     }
// }

// uint32_t Cache::ReadWord(uint64_t address)
// {
//     CacheLine& line = GetCacheLine(address);
//     if(line.valid && line.tag == GetTag(address))
//     {
//         hits_++;
//         return ReadGeneric<uint32_t>(address);
//     }
//     else
//     {
//         misses_++;
//         uint32_t value = memory_.ReadWord(address);
//         BringInCache(address);
//         return value;
//     }
// }

// uint64_t Cache::ReadDoubleWord(uint64_t address)
// {
//     CacheLine& line = GetCacheLine(address);
//     if(line.valid && line.tag == GetTag(address))
//     {
//         hits_++;
//         return ReadGeneric<uint64_t>(address);
//     }
//     else
//     {
//         misses_++;
//         uint64_t value = memory_.ReadDoubleWord(address);
//         BringInCache(address);
//         return value;
//     }
// }


// void Cache::Reset()
// {
//     for (auto& line : cache_lines_)
//     {
//         line.valid = false;
//         line.dirty = false;
//         std::fill(std::begin(line.data), std::end(line.data), 0);
//     }
//     hits_ = 0;
//     misses_ = 0;
// }
