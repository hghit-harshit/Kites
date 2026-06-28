#include "vm/cache/cache.h"
#include "vm/cache/policies/custom_policy.h"
#include "vm/cache/policies/fifo.h"
#include "vm/cache/policies/lru.h"
#include <span>

namespace Kites
{
/**TODO
 * Look if we can make this a factory class for better testability?
 */
namespace
{
std::unique_ptr<CacheReplacementPolicy>
createPolicy(ReplacementPolicy policy_type, const std::string &custom_policy_script_path)
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
}


void Cache::setupCache(size_t setCount, size_t lineSize, size_t wayCount)
{
    // assert(cache_size % (block_size * wayCount) == 0 && "Cache size must be divisible by block
    // size times number of ways");

    m_setCount        = setCount;
    m_lineSizeInBytes = lineSize;
    m_wayCount        = wayCount;
    m_offsetBits      = std::countr_zero( lineSize);
    m_setBits         = std::countr_zero(m_setCount);
    m_offsetMask      = (lineSize) - 1;
    m_setMask         = m_setCount - 1;

    m_sets.clear();
    m_timestampCounter = 0;
    m_sets.reserve(m_setCount);
    for (size_t i = 0; i < m_setCount; ++i)
    {
        m_sets.emplace_back(m_wayCount, CacheLine(m_lineSizeInBytes));
    }
}

Cache::Cache(MemoryDevice &memory, size_t setCount, size_t lineSize, size_t wayCount,
             WritePolicy writePolicy, AllocationPolicy allocationPolicy,
             ReplacementPolicy replacementPolicy)
    : QObject(nullptr), m_nextLevelMemoryRef(memory), m_writePolicy(writePolicy),
      m_allocationPolicy(allocationPolicy),
      m_ReplacementPolicy(createPolicy(replacementPolicy, std::string{}))
{
    setupCache(setCount, lineSize, wayCount);
}

void Cache::reconfigure(CacheConfig newConfig)
{
    flush(); // write back all dirty lines to memory and invalidate cache before reconfiguring
    m_writePolicy       = newConfig.writePolicy;
    m_allocationPolicy  = newConfig.allocationPolicy;
    m_ReplacementPolicy = createPolicy(newConfig.replacementPolicy, m_customPolicyScriptPath);
    setupCache(newConfig.lineCount, newConfig.lineSizeInBytes, newConfig.wayCount);
    emit cacheReconfiguredSignal(newConfig);
}

const CacheLine &Cache::getCacheLine(size_t setIndex,size_t wayIndex) const
{
    if (setIndex >= m_setCount || wayIndex >= m_wayCount)
    {
        throw std::out_of_range(std::string("Cache line index out of range: setIndex=") +
                                std::to_string(setIndex) +
                                ", wayIndex=" + std::to_string(wayIndex));
    }
    return m_sets[setIndex][wayIndex];
}

std::span<const uint8_t> Cache::readLine(uint64_t address, size_t lineSize)
{
    // a higher level cache will call this
    // if the line requested is not present we bring it from lower level memory devices
    assert(lineSize <= m_lineSizeInBytes && "Requested line size exceeds cache line size.");
    size_t setIndex = getSetIndex(address);
    uint64_t tag    = getTag(address);
    size_t wayIndex = findWay(setIndex, tag);

    if (wayIndex >= m_wayCount)
    {
        ++m_missCount;
        wayIndex = evictCacheLine(setIndex);
        bringIn(address, setIndex, wayIndex);
    }
    else 
    {
        ++m_hitCount;
        touchWay(setIndex, wayIndex);
    }
    size_t offset = getOffset(address); // get offset for this level of cache
                                       // as the higher leve will always call this function
                                       //with the address of the start of the line
                                       // and if size of this cache is larger than calling cache
                                       // address will not align
    return std::span<const uint8_t>(m_sets[setIndex][wayIndex].data.data() + offset, lineSize);
}

void Cache::writeLine(uint64_t address, std::span<const uint8_t> data)
{
    size_t setIndex = getSetIndex(address);
    uint64_t tag    = getTag(address);
    size_t wayIndex = findWay(setIndex, tag);

    if (wayIndex >= m_wayCount)
    {
        m_missCount++;
        if(m_allocationPolicy == AllocationPolicy::NoWriteAllocate)
        {
            m_nextLevelMemoryRef.writeLine(address, data);
            return;
        }
        else
        {
            wayIndex = evictCacheLine(setIndex);
            bringIn(address, setIndex, wayIndex);
        }
    }
    else
    {
        ++m_hitCount;
        touchWay(setIndex, wayIndex);
    }
    size_t offset = getOffset(address); 
    auto &line = m_sets[setIndex][wayIndex];
    std::memcpy(line.data.data() + offset, data.data(), data.size());

    if(m_writePolicy == WritePolicy::WriteThrough)
    {
        m_nextLevelMemoryRef.writeLine(address, data);
    }
    else if(m_writePolicy == WritePolicy::WriteBack)
    {
        line.dirty = true;
    }
    
}
/**
 * @brief  Returns the way index of the cache line in the specified set that matches the given tag.
 * If no such line exists, returns m_wayCount to indicate a miss.
 */
size_t Cache::findWay(size_t setIndex, uint64_t tag) const
{
    const auto &set = m_sets[setIndex];
    for (size_t way = 0; way < m_wayCount; ++way)
    {
        if (set[way].valid && set[way].tag == tag)
        {
            return way;
        }
    }
    return m_wayCount; // not found
}

void Cache::touchWay(size_t setIndex, size_t wayIndex)
{
    auto &line = m_sets[setIndex][wayIndex];

      // Update line age and lastAccess
    line.lastAccess = ++m_timestampCounter;
    line.age        = line.lastAccess;
    line.frequency++;

    // Notify policy of access
    CacheLineView view = {line.valid,      line.tag,        line.age,  line.frequency,
                          line.insertTime, line.lastAccess, line.dirty};
    CacheRequestView request = {0, setIndex, wayIndex, 0, 1, false, line.tag};
    CacheContextView context = {m_setCount, m_wayCount, m_lineSizeInBytes, m_timestampCounter};
    m_ReplacementPolicy->onAccess(view, request, context);
}

/**
 * @brief
 * Evicts a cache line from the specified set based on the replacement policy and returns the way
 * index of the evicted line. If there is an invalid line, it will be chosen for eviction without
 * considering the replacement policy.
 */
size_t Cache::evictCacheLine(size_t setIndex)
{
    auto &ways = m_sets[setIndex];

    // always prefer to evict an invalid line if available
    for (size_t way = 0; way < m_wayCount; ++way)
    {
        if (!ways[way].valid)
        {
            return way;
        }
    }

    // Build views for all valid lines in the set
    std::vector<CacheLineView> line_views;
    for (size_t way = 0; way < m_wayCount; ++way)
    {
        const auto &line = ways[way];
        line_views.push_back({line.valid, line.tag, line.age, line.frequency, line.insertTime,
                              line.lastAccess, line.dirty});
    }

    CacheRequestView request = {0, setIndex, 0, 0, 1, false, 0};
    CacheContextView context = {m_setCount, m_wayCount, m_lineSizeInBytes, m_timestampCounter};

    // Ask policy to choose victim
    size_t victim = m_ReplacementPolicy->chooseVictim(std::span(line_views), request, context);

    // Notify policy of eviction and write back if dirty
    CacheLineView victim_view = line_views[victim];
    m_ReplacementPolicy->onEvict(victim_view, request, context);

    if (ways[victim].dirty && m_writePolicy == WritePolicy::WriteBack)
    {
        writeBack(setIndex, victim);
    }

    ways[victim].valid = false; // invalidate the line before bringing in new data
    return victim;
}

void Cache::writeBack(size_t setIndex, size_t wayIndex)
{
    CacheLine &line = m_sets[setIndex][wayIndex];
    if (line.valid && line.dirty)
    {
        uint64_t lineStartAddress = 
                            (line.tag << (m_setBits + m_offsetBits)) | (setIndex << m_offsetBits);

        m_nextLevelMemoryRef.writeLine(lineStartAddress, std::span<const uint8_t>(line.data));
        line.dirty = false; 
    }
}

void Cache::bringIn(uint64_t address, size_t setIndex, size_t wayIndex)
{
    CacheLine &line = m_sets[setIndex][wayIndex];
    uint64_t lineStartAddress = address & ~(m_offsetMask); // align address to block boundary

    line.valid      = true;
    line.dirty      = false;
    line.tag        = getTag(address);
    line.insertTime = ++m_timestampCounter;
    line.lastAccess = line.insertTime;
    line.age        = line.insertTime;
    line.frequency  = 0;

    std::span<const uint8_t> lineFromNextLevel = m_nextLevelMemoryRef.readLine(lineStartAddress, 
                                                                                m_lineSizeInBytes);
    std::memcpy(line.data.data(), lineFromNextLevel.data(), m_lineSizeInBytes);


    // Notify policy of insertion
    CacheLineView view = {line.valid,      line.tag,        line.age,  line.frequency,
                          line.insertTime, line.lastAccess, line.dirty};
    CacheRequestView request = {address, setIndex, wayIndex, getOffset(address),
                                1,       false,     line.tag};
    CacheContextView context = {m_setCount, m_wayCount, m_lineSizeInBytes, m_timestampCounter};
    m_ReplacementPolicy->onInsert(view, request, context);
}

uint8_t Cache::getByteFromCache(uint64_t address)
{
    size_t   setIndex = getSetIndex(address);
    uint64_t tag      = getTag(address);
    size_t   wayIndex = findWay(setIndex, tag);
    assert(wayIndex < m_wayCount && "getByteFromCache called on a cache line that is not present!");
    return m_sets[setIndex][wayIndex].data[getOffset(address)];
}

void Cache::putByteInCache(uint64_t address, uint8_t value)
{
    size_t   setIndex = getSetIndex(address);
    uint64_t tag      = getTag(address);
    size_t   wayIndex = findWay(setIndex, tag);
    CacheLine &line = m_sets[setIndex][wayIndex];
    line.data[getOffset(address)] = value;

    if (m_writePolicy == WritePolicy::WriteThrough)
    {
        // writeByteToNextLevel(address, value);
        line.dirty = false;
    }
    else
    {
        line.dirty = true;
    }
}

template<typename T>
bool Cache::isHit(uint64_t address) const
{
    uint64_t currentAddress = address;
    uint64_t endAddress = address + sizeof(T) - 1;
    // we check if all the line that the data access will touch are in the cache
    // because the data can be split across multiple cache lines
    while(currentAddress <= endAddress)
    {
        size_t   setIndex = getSetIndex(currentAddress);
        uint64_t tag      = getTag(currentAddress);
        size_t   wayIndex = findWay(setIndex, tag);
        if (wayIndex >= m_wayCount)
        {
            return false;
        }
        currentAddress += m_lineSizeInBytes - getOffset(currentAddress); // move to the next cache line
    }
    return true;
}

template<typename T>
void Cache::touchLines(uint64_t address)
{
    uint64_t currentAddress = address;
    uint64_t endAddress = address + sizeof(T) - 1;
    while(currentAddress <= endAddress)
    {
        size_t   setIndex = getSetIndex(currentAddress);
        uint64_t tag      = getTag(currentAddress);
        size_t   wayIndex = findWay(setIndex, tag);
        touchWay(setIndex, wayIndex); // we know its a cache hit because we 
                                      // check it before calling this function
        currentAddress += m_lineSizeInBytes - getOffset(currentAddress);
    }
}

template<typename T>
void Cache::bringInLines(uint64_t address)
{
    uint64_t currentAddress = address;
    uint64_t endAddress = address + sizeof(T) - 1;
    while(currentAddress <= endAddress)
    {
        size_t   setIndex = getSetIndex(currentAddress);
        size_t   wayIndex = evictCacheLine(setIndex);
        bringIn(currentAddress, setIndex, wayIndex);
        currentAddress += m_lineSizeInBytes - getOffset(currentAddress);
    }
}

template <typename T> 
T Cache::readGeneric(uint64_t address)
{
    if (address >= vm_config::config.getMemorySize() - (sizeof(T) - 1))
    {
        throw std::out_of_range(std::string("Cache read address out of range: ") +
                                std::to_string(address));
    }

    // Multi-byte accesses can cross cache-line boundaries; handle them byte-wise.
    // The issue here is that if we read byte wise the hits count will increase for every byte
    // but we want it to count when all the byte are in the cache for the
    // word/half-word/double-word access
    if(isHit<T>(address))
    {
        ++m_hitCount;
        touchLines<T>(address);
    }
    else
    {
        ++m_missCount;
        bringInLines<T>(address);
    }

    T value = 0;
    for(size_t i = 0; i < sizeof(T); ++i)
    {
        uint8_t byte = getByteFromCache(address + i);
        value |= static_cast<T>(byte) << (8 * i);
    }
    return value;
}

template <typename T> void Cache::writeGeneric(uint64_t address, T value)
{
    if (address >= vm_config::config.getMemorySize() - (sizeof(T) - 1))
    {
        throw std::out_of_range(std::string("Cache write address out of range: ") +
                                std::to_string(address));
    }

    if(isHit<T>(address))
    {
        ++m_hitCount;
        touchLines<T>(address);
    }
    else
    {
        ++m_missCount;
        
        if(m_allocationPolicy == AllocationPolicy::NoWriteAllocate)
        {
            if constexpr(std::is_same_v<T, uint8_t>)
            {
                m_nextLevelMemoryRef.writeByte(address, static_cast<uint8_t>(value));
            }
            else if constexpr(std::is_same_v<T, uint16_t>)
            {
                m_nextLevelMemoryRef.writeHalfWord(address, static_cast<uint16_t>(value));
            }
            else if constexpr(std::is_same_v<T, uint32_t>)
            {
                m_nextLevelMemoryRef.writeWord(address, static_cast<uint32_t>(value));
            }
            else if constexpr(std::is_same_v<T, uint64_t>)
            {
                m_nextLevelMemoryRef.writeDoubleWord(address, static_cast<uint64_t>(value));
            }
            return;
        }
        bringInLines<T>(address);
    }

    for(size_t i = 0; i < sizeof(T); ++i)
    {
        uint8_t byte = static_cast<uint8_t>((value >> (8 * i)) & 0xFF);
        putByteInCache(address + i, byte);
    }

}

uint8_t Cache::readByte(uint64_t address)
{
    const size_t misses_before = m_missCount;
    const uint8_t value        = readGeneric<uint8_t>(address);
    if (m_missCount > misses_before)
    {
        emit cacheMissSignal(address);
    }
    else
    {
        emit cacheHitSignal(address);
    }
    emit cacheLineUpdatedSignal(address);
    updateStats();
    return value;
}

uint16_t Cache::readHalfWord(uint64_t address)
{
    const size_t misses_before = m_missCount;
    const uint16_t value       = readGeneric<uint16_t>(address);
    if (m_missCount > misses_before)
    {
        emit cacheMissSignal(address);
    }
    else
    {
        emit cacheHitSignal(address);
    }
    emit cacheLineUpdatedSignal(address);
    updateStats();
    return value;
}

uint32_t Cache::readWord(uint64_t address)
{
    const size_t misses_before = m_missCount;
    const uint32_t value       = readGeneric<uint32_t>(address);
    if (m_missCount > misses_before)
    {
        emit cacheMissSignal(address);
    }
    else
    {
        emit cacheHitSignal(address);
    }
    emit cacheLineUpdatedSignal(address);
    updateStats();
    return value;
}

uint64_t Cache::readDoubleWord(uint64_t address)
{
    const size_t misses_before = m_missCount;
    const uint64_t value       = readGeneric<uint64_t>(address);
    if (m_missCount > misses_before)
    {
        emit cacheMissSignal(address);
    }
    else
    {
        emit cacheHitSignal(address);
    }
    emit cacheLineUpdatedSignal(address);
    updateStats();
    return value;
}

void Cache::writeByte(uint64_t address, uint8_t value)
{
    const size_t misses_before = m_missCount;
    writeGeneric<uint8_t>(address, value);
    if (m_missCount > misses_before)
    {
        emit cacheMissSignal(address);
    }
    else
    {
        emit cacheHitSignal(address);
    }
    emit cacheLineUpdatedSignal(address);
    updateStats();
}

void Cache::writeHalfWord(uint64_t address, uint16_t value)
{
    const size_t misses_before = m_missCount;
    writeGeneric<uint16_t>(address, value);
    if (m_missCount > misses_before)
    {
        emit cacheMissSignal(address);
    }
    else
    {
        emit cacheHitSignal(address);
    }
    emit cacheLineUpdatedSignal(address);
    updateStats();
}

void Cache::writeWord(uint64_t address, uint32_t value)
{
    const size_t misses_before = m_missCount;
    writeGeneric<uint32_t>(address, value);
    if (m_missCount > misses_before)
    {
        emit cacheMissSignal(address);
    }
    else
    {
        emit cacheHitSignal(address);
    }
    emit cacheLineUpdatedSignal(address);
    updateStats();
}

void Cache::writeDoubleWord(uint64_t address, uint64_t value)
{
    const size_t misses_before = m_missCount;
    writeGeneric<uint64_t>(address, value);
    if (m_missCount > misses_before)
    {
        emit cacheMissSignal(address);
    }
    else
    {
        emit cacheHitSignal(address);
    }
    emit cacheLineUpdatedSignal(address);
    updateStats();
}

void Cache::reset()
{
    for (auto &set : m_sets)
    {
        for (auto &line : set)
        {
            line.valid      = false;
            line.dirty      = false;
            line.age        = 0;
            line.insertTime = 0;
            line.lastAccess = 0;
            line.frequency  = 0;
            std::fill(std::begin(line.data), std::end(line.data), 0);
        }
    }
    m_timestampCounter = 0;
    m_hitCount = 0;
    m_missCount = 0;
}

void Cache::flush()
{
    for (size_t setIndex = 0; setIndex < m_setCount; ++setIndex)
    {
        for (size_t wayIndex = 0; wayIndex < m_wayCount; ++wayIndex)
        {
            if (m_sets[setIndex][wayIndex].valid && m_sets[setIndex][wayIndex].dirty)
            {
                writeBack(setIndex, wayIndex);
            }
        }
    }
    reset();
}

void Cache::updateStats()
{
    CacheStats stats;
    stats.hitCount = m_hitCount;
    stats.missCount = m_missCount;
    // For write-backs, we would need to track them in the WriteBack function
    // stats.writeBacks = write_backs_;
    emit cacheStatsUpdatedSignal(stats);
}

void Cache::loadCustomPolicyScript(const std::string &path)
{
    m_customPolicyScriptPath = path;
    ReplacementPolicy old_replacement_policy = m_ReplacementPolicy->type();
    std::string old_script_path;
    if (old_replacement_policy == ReplacementPolicy::Custom)
    {
        old_script_path = dynamic_cast<CustomReplacementPolicy*>(m_ReplacementPolicy.get())->getScriptPath();
    }
    try
    {
        m_ReplacementPolicy = createPolicy(ReplacementPolicy::Custom, m_customPolicyScriptPath);
        emit customPolicyScriptLoadedSignal(true, m_customPolicyScriptPath);
    }
    catch (const std::exception &e)
    {
        emit customPolicyScriptLoadedSignal(false, e.what());
        m_ReplacementPolicy = createPolicy(old_replacement_policy,
                                old_script_path); // revert to old policy on failure
    }
}

// Statistics
size_t Cache::getHitCount() const
{
    return m_hitCount;
}
size_t Cache::getMissCount() const
{
    return m_missCount;
}
double Cache::getHitRate() const
{
    size_t total = m_hitCount + m_missCount;
    return total > 0 ? static_cast<double>(m_hitCount) / total : 0.0;
}
double Cache::getMissRate() const
{
    size_t total = m_hitCount + m_missCount;
    return total > 0 ? static_cast<double>(m_missCount) / total : 0.0;
}
size_t Cache::getSetCount() const
{
    return m_setCount;
}
size_t Cache::getWayCount() const
{
    return m_wayCount;
}
size_t Cache::getLineSizeInBytes() const
{
    return m_lineSizeInBytes;
}
// Address Decomposition Helper Functions
inline uint64_t Cache::getTag(uint64_t address) const
{
    return address >> (m_offsetBits + m_setBits);
}
inline size_t Cache::getSetIndex(uint64_t address) const
{
    return static_cast<size_t>(address >> m_offsetBits) & m_setMask;
}
inline size_t Cache::getOffset(uint64_t address) const
{
    return static_cast<size_t>(address & m_offsetMask);
}
}//namespace Kites

