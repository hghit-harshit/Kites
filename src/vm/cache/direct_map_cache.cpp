#include "vm/cache/direct_map_cache.h"

void DirectMapCache::Reset()
{
    for (auto& line : cache_lines_)
    {
        line.valid = false;
        line.dirty = false;
        std::fill(std::begin(line.data), std::end(line.data), 0);
    }
    hits_ = 0;
    misses_ = 0;
}

template <typename T>
void DirectMapCache::WriteCacheGeneric(uint64_t address, T value)
{
    if(address >= vm_config::config.getMemorySize() - (sizeof(T) - 1))
    {
        throw std::out_of_range(std::string("Cache write address out of range: ") + std::to_string(address));
    }
    
    CacheLine& line = GetCacheLine(address);
    if(!line.valid || line.tag != GetTag(address))
    {
        if(allocation_policy_ == AllocationPolicy::WriteAllocate)
        {
            BringInCache(address);
        }
        else
        {
            // if it's no write allocate and the line is not valid or tag does not match, we just write directly to memory without bringing it to cache
            uint64_t block_start_address = address - GetOffset(address);
            for(size_t i = 0; i < sizeof(T); ++i)
            {
                memory_.WriteByte(block_start_address + i, static_cast<uint8_t>((value >> (8*i)) & 0xFF));
            }
            return;
        }
    }
    // since this function will only be called by the specific write functions and that to when line exists and is valid, we can skip the check for valid and tag match
    //if(allocation_policy_)
    for (size_t i = 0; i < sizeof(T); ++i) 
    {
        line.data[GetOffset(address) + i] = static_cast<uint8_t>((value >> (8*i)) & 0xFF);
    }
    if(write_policy_ == WritePolicy::WriteThrough)
    {
        //write through to memory
        uint64_t block_start_address = address - GetOffset(address);
        for(size_t i = 0; i < sizeof(T); ++i)
        {
            memory_.WriteByte(block_start_address + i, line.data[GetOffset(address) + i]);
        }
        line.dirty = false; // since it's written through, it's not dirty anymore
    }
    else
    {
        line.dirty = true; // mark the line as dirty since it's a write back cache
    }
}


void DirectMapCache::WriteByte(uint64_t address, uint8_t value)
{
    
    WriteCacheGeneric<uint8_t>(address, value);
}

void DirectMapCache::WriteHalfWord(uint64_t address, uint16_t value)
{
    WriteCacheGeneric<uint16_t>(address, value);
}

void DirectMapCache::WriteWord(uint64_t address, uint32_t value)
{
    WriteCacheGeneric<uint32_t>(address, value);
}

void DirectMapCache::WriteDoubleWord(uint64_t address, uint64_t value)
{
    WriteCacheGeneric<uint64_t>(address, value);
}

uint8_t DirectMapCache::Read(uint64_t address)
{
    if(address >= vm_config::config.getMemorySize())
    {
        throw std::out_of_range(std::string("Cache read address out of range: ") + std::to_string(address));
    }
    CacheLine& line = GetCacheLine(address);
    // since this function will only be called by the generic and that to when line exists and is valid, we can skip the check for valid and tag match
    return line.data[GetOffset(address)];

}


void DirectMapCache::BringInCache(uint64_t address)
{
    if(address >= vm_config::config.getMemorySize())
    {
        throw std::out_of_range(std::string("Cache bring in address out of range: ") + std::to_string(address));
    }
   
    uint64_t block_start_address = address - GetOffset(address);
    CacheLine& line = GetCacheLine(address);
    if(line.valid && line.dirty && line.tag != GetTag(address) && write_policy_ == WritePolicy::WriteBack)
    {
        //write back to memory
        uint64_t block_start_address = (line.tag * LINE_SIZE);
        for(size_t i = 0; i < LINE_SIZE; ++i)
        {
            memory_.WriteByte(block_start_address + i, line.data[i]);
        }
    }
    line.valid = true;
    line.dirty = false;
    line.tag = GetTag(address);
    for(size_t i = 0; i < LINE_SIZE; ++i)
    {
        line.data[i] = memory_.ReadByte(block_start_address + i);
    }
}

template <typename T>
T DirectMapCache::ReadGeneric(uint64_t address)
{
    if(address >= vm_config::config.getMemorySize() - (sizeof(T) - 1))
    {
        throw std::out_of_range(std::string("Cache read address out of range: ") + std::to_string(address));
    }
    CacheLine& line = GetCacheLine(address);
    // since this function will only be called by the specific read functions and that to when line exists and is valid, we can skip the check for valid and tag match
    T value = 0;
    for (size_t i = 0; i < sizeof(T); ++i) {
        value |= static_cast<T>(line.data[GetOffset(address) + i]) << (8*i);
    }
    return value;
}



uint8_t DirectMapCache::ReadByte(uint64_t address)
{
    CacheLine& line = GetCacheLine(address);
    if(line.valid && line.tag == GetTag(address))
    {
        hits_++;
        return Read(address);
    }
    else
    {
        misses_++;
        uint8_t value = memory_.ReadByte(address);
        BringInCache(address);
        return value;
    }
}

uint16_t DirectMapCache::ReadHalfWord(uint64_t address)
{
    // Implementation of half-word read from cache
    CacheLine& line = GetCacheLine(address);
    if(line.valid && line.tag == GetTag(address))
    {
        hits_++;
        return ReadGeneric<uint16_t>(address);
    }
    else
    {
        misses_++;
        uint16_t value = memory_.ReadHalfWord(address);
        BringInCache(address);
        return value;
    }
}

uint32_t DirectMapCache::ReadWord(uint64_t address)
{
    CacheLine& line = GetCacheLine(address);
    if(line.valid && line.tag == GetTag(address))
    {
        hits_++;
        return ReadGeneric<uint32_t>(address);
    }
    else
    {
        misses_++;
        uint32_t value = memory_.ReadWord(address);
        BringInCache(address);
        return value;
    }
}

uint64_t DirectMapCache::ReadDoubleWord(uint64_t address)
{
    CacheLine& line = GetCacheLine(address);
    if(line.valid && line.tag == GetTag(address))
    {
        hits_++;
        return ReadGeneric<uint64_t>(address);
    }
    else
    {
        misses_++;
        uint64_t value = memory_.ReadDoubleWord(address);
        BringInCache(address);
        return value;
    }
}

