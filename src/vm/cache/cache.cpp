#include "vm/cache/cache.h"
#include "vm/cache/policies/custom_policy.h"
#include "vm/cache/policies/fifo.h"
#include "vm/cache/policies/lru.h"
#include <span>

namespace Kites
{
static std::unique_ptr<CacheReplacementPolicy>
CreatePolicy(ReplacementPolicy policy_type, const std::string &custom_policy_script_path)
{
    switch (policy_type)
    {
    case ReplacementPolicy::LRU:
        return std::make_unique<LRUReplacementPolicy>();
    case ReplacementPolicy::FIFO:
        return std::make_unique<FIFOReplacementPolicy>();
    case ReplacementPolicy::Custom:
        return std::make_unique<CustomReplacementPolicy>(custom_policy_script_path);
    default:
        return std::make_unique<LRUReplacementPolicy>(); // default fallback
    }
}

void Cache::SetupCache(size_t num_sets, size_t block_size, size_t num_ways)
{

    // assert(cache_size % (block_size * num_ways) == 0 && "Cache size must be divisible by block
    // size times number of ways");

    num_sets_ = num_sets;
    block_size_ = block_size;
    num_ways_ = num_ways;

    offset_bits_ = std::countr_zero(
        block_size *
        4); // number of bytes in a cache line, assuming block_size is in words (4 bytes)
    set_bits_ = std::countr_zero(num_sets_);
    offset_mask_ = (block_size * 4) - 1;
    set_mask_ = num_sets_ - 1;

    sets_.clear();
    timestamp_counter_ = 0;
    sets_.reserve(num_sets_);
    for (size_t i = 0; i < num_sets_; ++i)
    {
        sets_.emplace_back(num_ways_, CacheLine(block_size_));
    }
}

Cache::Cache(Memory &memory, size_t num_sets, size_t block_size, size_t num_ways,
             WritePolicy write_policy, AllocationPolicy allocation_policy,
             ReplacementPolicy replacement_policy)
    : QObject(nullptr),memory_(&memory), next_level_cache_(nullptr), write_policy_(write_policy),
      allocation_policy_(allocation_policy),
      m_policy(CreatePolicy(replacement_policy, std::string{}))
{
    SetupCache(num_sets, block_size, num_ways);
}

Cache::Cache(Cache &next_level_cache, size_t num_sets, size_t block_size, size_t num_ways,
             WritePolicy write_policy, AllocationPolicy allocation_policy,
             ReplacementPolicy replacement_policy)
    : QObject(nullptr),memory_(nullptr), next_level_cache_(&next_level_cache), write_policy_(write_policy),
      allocation_policy_(allocation_policy),
      m_policy(CreatePolicy(replacement_policy, std::string{}))
{
    SetupCache(num_sets, block_size, num_ways);
}

void Cache::Reconfigure(CacheConfig new_config)
{
    Flush(); // write back all dirty lines to memory and invalidate cache before reconfiguring
    write_policy_ = new_config.write_policy;
    allocation_policy_ = new_config.allocation_policy;
    m_policy = CreatePolicy(new_config.replacement_policy, custom_policy_script_path_);
    SetupCache(new_config.num_lines, new_config.block_size, new_config.num_ways);
    emit CacheReconfiguredSignal(new_config);
}

const CacheLine &Cache::GetCacheLine(size_t set_index, size_t way_index) const
{
    if (set_index >= num_sets_ || way_index >= num_ways_)
    {
        throw std::out_of_range(std::string("Cache line index out of range: set_index=") +
                                std::to_string(set_index) +
                                ", way_index=" + std::to_string(way_index));
    }
    return sets_[set_index][way_index];
}

// Next Leve Helper Functions
uint8_t Cache::ReadFromNextLevel(uint64_t address)
{
    if (next_level_cache_)
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
    if (next_level_cache_)
    {
        next_level_cache_->WriteByte(address, value);
    }
    else
    {
        memory_->WriteByte(address, value);
    }
}

uint64_t Cache::SizeofNextLevel() const
{
    if (next_level_cache_)
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
    const auto &set = sets_[set_index];
    for (size_t way = 0; way < num_ways_; ++way)
    {
        if (set[way].valid && set[way].tag == tag)
        {
            return way;
        }
    }
    return num_ways_; // not found
}

void Cache::TouchWay(size_t set_index, size_t way_index)
{
    auto &line = sets_[set_index][way_index];

    // Update line age and lastAccess
    line.lastAccess = ++timestamp_counter_;
    line.age = line.lastAccess;
    line.frequency++;

    // Notify policy of access
    CacheLineView view = {line.valid,      line.tag,        line.age,  line.frequency,
                          line.insertTime, line.lastAccess, line.dirty};
    CacheRequestView request = {0, set_index, way_index, 0, 1, false, line.tag};
    CacheContextView context = {num_sets_, num_ways_, block_size_, timestamp_counter_};
    m_policy->onAccess(view, request, context);
}

/**
 * @brief
 * Evicts a cache line from the specified set based on the replacement policy and returns the way
 * index of the evicted line. If there is an invalid line, it will be chosen for eviction without
 * considering the replacement policy.
 */
size_t Cache::EvictWay(size_t set_index)
{
    auto &ways = sets_[set_index];

    // always prefer to evict an invalid line if available
    for (size_t way = 0; way < num_ways_; ++way)
    {
        if (!ways[way].valid)
        {
            return way;
        }
    }

    // Build views for all valid lines in the set
    std::vector<CacheLineView> line_views;
    for (size_t way = 0; way < num_ways_; ++way)
    {
        const auto &line = ways[way];
        line_views.push_back({line.valid, line.tag, line.age, line.frequency, line.insertTime,
                              line.lastAccess, line.dirty});
    }

    CacheRequestView request = {0, set_index, 0, 0, 1, false, 0};
    CacheContextView context = {num_sets_, num_ways_, block_size_, timestamp_counter_};

    // Ask policy to choose victim
    size_t victim = m_policy->chooseVictim(std::span(line_views), request, context);

    // Notify policy of eviction and write back if dirty
    CacheLineView victim_view = line_views[victim];
    m_policy->onEvict(victim_view, request, context);

    if (ways[victim].dirty && write_policy_ == WritePolicy::WriteBack)
    {
        WriteBack(set_index, victim);
    }

    ways[victim].valid = false; // invalidate the line before bringing in new data
    return victim;
}

void Cache::WriteBack(size_t set_index, size_t way_index)
{
    CacheLine &line = sets_[set_index][way_index];
    if (line.valid && line.dirty)
    {
        uint64_t block_start_address =
            (line.tag << (set_bits_ + offset_bits_)) | (set_index << offset_bits_);
        for (size_t i = 0; i < block_size_ * 4; ++i)
        {
            WriteByteToNextLevel(block_start_address + i, line.data[i]);
        }
        line.dirty = false;
    }
}

void Cache::BringIn(uint64_t address, size_t set_index, size_t way_index)
{
    CacheLine &line = sets_[set_index][way_index];
    uint64_t block_start_address = address & ~(offset_mask_); // align address to block boundary

    line.valid = true;
    line.dirty = false;
    line.tag = GetTag(address);
    line.insertTime = ++timestamp_counter_;
    line.lastAccess = line.insertTime;
    line.age = line.insertTime;
    line.frequency = 0;

    for (size_t i = 0; i < block_size_ * 4; ++i)
    {
        line.data[i] = ReadFromNextLevel(block_start_address + i);
    }

    // Notify policy of insertion
    CacheLineView view = {line.valid,      line.tag,        line.age,  line.frequency,
                          line.insertTime, line.lastAccess, line.dirty};
    CacheRequestView request = {address, set_index, way_index, GetOffset(address),
                                1,       false,     line.tag};
    CacheContextView context = {num_sets_, num_ways_, block_size_, timestamp_counter_};
    m_policy->onInsert(view, request, context);
}

bool Cache::ReadByteAccess(uint64_t address, uint8_t &value, bool count_stats)
{
    size_t set_index = GetSetIndex(address);
    uint64_t tag = GetTag(address);
    size_t way_index = FindWay(set_index, tag);

    const bool hit = way_index < num_ways_;
    if (hit)
    {
        if (count_stats)
        {
            ++hits_;
        }
        TouchWay(set_index, way_index);
    }
    else
    {
        if (count_stats)
        {
            ++misses_;
        }
        way_index = EvictWay(set_index);
        BringIn(address, set_index, way_index);
    }

    value = sets_[set_index][way_index].data[GetOffset(address)];
    return hit;
}

bool Cache::WriteByteAccess(uint64_t address, uint8_t value, bool count_stats)
{
    size_t set_index = GetSetIndex(address);
    uint64_t tag = GetTag(address);
    size_t offset = GetOffset(address);
    size_t way_index = FindWay(set_index, tag);

    const bool hit = way_index < num_ways_;
    if (!hit)
    {
        if (count_stats)
        {
            ++misses_;
        }

        if (allocation_policy_ == AllocationPolicy::NoWriteAllocate)
        {
            WriteByteToNextLevel(address, value);
            return false;
        }

        way_index = EvictWay(set_index);
        BringIn(address, set_index, way_index);
    }
    else
    {
        if (count_stats)
        {
            ++hits_;
        }
        TouchWay(set_index, way_index);
    }

    CacheLine &line = sets_[set_index][way_index];
    line.data[offset] = value;

    if (write_policy_ == WritePolicy::WriteThrough)
    {
        WriteByteToNextLevel(address, value);
        line.dirty = false;
    }
    else
    {
        line.dirty = true;
    }

    return hit;
}

template <typename T> T Cache::ReadGeneric(uint64_t address)
{
    if (address >= vm_config::config.getMemorySize() - (sizeof(T) - 1))
    {
        throw std::out_of_range(std::string("Cache read address out of range: ") +
                                std::to_string(address));
    }

    // Multi-byte accesses can cross cache-line boundaries; handle them byte-wise.

    // The issue here is that if we read byte wise the hits cournt will increase for every bit
    // but we want it tom counte when all the byte are in the cache for the
    // word/half-word/double-word access
    if constexpr (sizeof(T) > 1)
    {
        T value = 0;
        bool all_hit = true;
        for (size_t i = 0; i < sizeof(T); ++i)
        {
            uint8_t byte = 0;
            all_hit = ReadByteAccess(address + i, byte, false) && all_hit;
            value |= static_cast<T>(byte) << (8 * i);
        }

        if (all_hit)
        {
            ++hits_;
        }
        else
        {
            ++misses_;
        }

        return value;
    }
    else
    {
        uint8_t byte = 0;
        ReadByteAccess(address, byte, true);
        return static_cast<T>(byte);
    }
}

template <typename T> void Cache::WriteCacheGeneric(uint64_t address, T value)
{
    if (address >= vm_config::config.getMemorySize() - (sizeof(T) - 1))
    {
        throw std::out_of_range(std::string("Cache write address out of range: ") +
                                std::to_string(address));
    }

    // Multi-byte accesses can cross cache-line boundaries; handle them byte-wise.
    if constexpr (sizeof(T) > 1)
    {
        bool all_hit = true;
        for (size_t i = 0; i < sizeof(T); ++i)
        {
            all_hit = WriteByteAccess(address + i, static_cast<uint8_t>((value >> (8 * i)) & 0xFF),
                                      false) &&
                      all_hit;
        }

        if (all_hit)
        {
            ++hits_;
        }
        else
        {
            ++misses_;
        }

        return;
    }
    else
    {
        WriteByteAccess(address, static_cast<uint8_t>(value), true);
    }
}

uint8_t Cache::ReadByte(uint64_t address)
{
    const size_t misses_before = misses_;
    const uint8_t value = ReadGeneric<uint8_t>(address);
    if (misses_ > misses_before)
    {
        emit CacheMissSignal(address);
    }
    else
    {
        emit CacheHitSignal(address);
    }
    emit CacheLineUpdatedSignal(address);
    UpdateStats();
    return value;
}

uint16_t Cache::ReadHalfWord(uint64_t address)
{
    const size_t misses_before = misses_;
    const uint16_t value = ReadGeneric<uint16_t>(address);
    if (misses_ > misses_before)
    {
        emit CacheMissSignal(address);
    }
    else
    {
        emit CacheHitSignal(address);
    }
    emit CacheLineUpdatedSignal(address);
    UpdateStats();
    return value;
}

uint32_t Cache::ReadWord(uint64_t address)
{
    const size_t misses_before = misses_;
    const uint32_t value = ReadGeneric<uint32_t>(address);
    if (misses_ > misses_before)
    {
        emit CacheMissSignal(address);
    }
    else
    {
        emit CacheHitSignal(address);
    }
    emit CacheLineUpdatedSignal(address);
    UpdateStats();
    return value;
}

uint64_t Cache::ReadDoubleWord(uint64_t address)
{
    const size_t misses_before = misses_;
    const uint64_t value = ReadGeneric<uint64_t>(address);
    if (misses_ > misses_before)
    {
        emit CacheMissSignal(address);
    }
    else
    {
        emit CacheHitSignal(address);
    }
    emit CacheLineUpdatedSignal(address);
    UpdateStats();
    return value;
}

void Cache::WriteByte(uint64_t address, uint8_t value)
{
    const size_t misses_before = misses_;
    WriteCacheGeneric<uint8_t>(address, value);
    if (misses_ > misses_before)
    {
        emit CacheMissSignal(address);
    }
    else
    {
        emit CacheHitSignal(address);
    }
    emit CacheLineUpdatedSignal(address);
    UpdateStats();
}

void Cache::WriteHalfWord(uint64_t address, uint16_t value)
{
    const size_t misses_before = misses_;
    WriteCacheGeneric<uint16_t>(address, value);
    if (misses_ > misses_before)
    {
        emit CacheMissSignal(address);
    }
    else
    {
        emit CacheHitSignal(address);
    }
    emit CacheLineUpdatedSignal(address);
    UpdateStats();
}

void Cache::WriteWord(uint64_t address, uint32_t value)
{
    const size_t misses_before = misses_;
    WriteCacheGeneric<uint32_t>(address, value);
    if (misses_ > misses_before)
    {
        emit CacheMissSignal(address);
    }
    else
    {
        emit CacheHitSignal(address);
    }
    emit CacheLineUpdatedSignal(address);
    UpdateStats();
}

void Cache::WriteDoubleWord(uint64_t address, uint64_t value)
{
    const size_t misses_before = misses_;
    WriteCacheGeneric<uint64_t>(address, value);
    if (misses_ > misses_before)
    {
        emit CacheMissSignal(address);
    }
    else
    {
        emit CacheHitSignal(address);
    }
    emit CacheLineUpdatedSignal(address);
    UpdateStats();
}

void Cache::Reset()
{
    for (auto &set : sets_)
    {
        for (auto &line : set)
        {
            line.valid = false;
            line.dirty = false;
            line.age = 0;
            line.insertTime = 0;
            line.lastAccess = 0;
            line.frequency = 0;
            std::fill(std::begin(line.data), std::end(line.data), 0);
        }
    }
    timestamp_counter_ = 0;
    hits_ = 0;
    misses_ = 0;
}

void Cache::Flush()
{
    for (size_t set_index = 0; set_index < num_sets_; ++set_index)
    {
        for (size_t way_index = 0; way_index < num_ways_; ++way_index)
        {
            if (sets_[set_index][way_index].valid && sets_[set_index][way_index].dirty)
            {
                WriteBack(set_index, way_index);
            }
        }
    }
    Reset();
}

void Cache::UpdateStats()
{
    CacheStats stats;
    stats.hits = hits_;
    stats.misses = misses_;
    // For write-backs, we would need to track them in the WriteBack function
    // stats.writeBacks = write_backs_;
    emit CacheStatsUpdatedSignal(stats);
}

void Cache::LoadCustomPolicyScript(const std::string &path)
{
    custom_policy_script_path_ = path;
    ReplacementPolicy old_replacement_policy = m_policy->type();
    std::string old_script_path;
    if (old_replacement_policy == ReplacementPolicy::Custom)
    {
        old_script_path = dynamic_cast<CustomReplacementPolicy *>(m_policy.get())->getScriptPath();
    }
    try
    {
        m_policy = CreatePolicy(ReplacementPolicy::Custom, custom_policy_script_path_);
        emit CustomPolicyScriptLoadedSignal(true, custom_policy_script_path_);
    }
    catch (const std::exception &e)
    {
        emit CustomPolicyScriptLoadedSignal(false, e.what());
        m_policy = CreatePolicy(old_replacement_policy,
                                old_script_path); // revert to old policy on failure
    }
}
}//namespace Kites

