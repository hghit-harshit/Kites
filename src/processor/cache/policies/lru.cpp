#include "processor/cache/policies/lru.h"

namespace Kites
{
std::string_view LRUReplacementPolicy::name() const
{
    return "LRU";
}
ReplacementPolicy LRUReplacementPolicy::type() const
{
    return ReplacementPolicy::LRU;
}
size_t LRUReplacementPolicy::chooseVictim(std::span<const CacheLineView> lines,
                                          const CacheRequestView &request,
                                          const CacheContextView &context)
{
    // Find the line with the smallest age (least recently used)
    (void)request;
    (void)context;
    size_t victim_index = 0;
    uint64_t min_age = UINT64_MAX;

    for (size_t i = 0; i < lines.size(); ++i)
    {
        if (!lines[i].valid)
        {
            // If we find an invalid line, we can use it immediately
            return i;
        }
        if (lines[i].age < min_age)
        {
            min_age = lines[i].age;
            victim_index = i;
        }
    }
    return victim_index;
}

void LRUReplacementPolicy::onAccess(const CacheLineView &line, const CacheRequestView &request,
                                    const CacheContextView &context)
{
    // No internal state to update on access since we rely on the age field in CacheLineView
    (void)line;
    (void)request;
    (void)context;
}

void LRUReplacementPolicy::onInsert(const CacheLineView &line, const CacheRequestView &request,
                                    const CacheContextView &context)
{
    // No internal state to update on insert since we rely on the age field in CacheLineView
    (void)line;
    (void)request;
    (void)context;
}

void LRUReplacementPolicy::onEvict(const CacheLineView &line, const CacheRequestView &request,
                                   const CacheContextView &context)
{
    // No internal state to update on evict since we rely on the age field in CacheLineView
    (void)line;
    (void)request;
    (void)context;
}
}//namespace Kites