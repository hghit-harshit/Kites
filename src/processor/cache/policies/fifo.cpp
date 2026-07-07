#include "processor/cache/policies/fifo.h"

namespace Kites
{
std::string_view FIFOReplacementPolicy::name() const
{
    return "FIFO";
}

ReplacementPolicy FIFOReplacementPolicy::type() const
{
    return ReplacementPolicy::FIFO;
}

size_t FIFOReplacementPolicy::chooseVictim(std::span<const CacheLineView> lines,
                                           const CacheRequestView &request,
                                           const CacheContextView &context)
{
    // The FIFO queue is maintained in the cache metadata for each set.
    // The victim is the line at the front of the queue (the oldest line).
    // If there are invalid lines, we can use them first before evicting any valid lines.
    (void)request;
    (void)context;
    // First check for any invalid lines
    for (size_t i = 0; i < lines.size(); ++i)
    {
        if (!lines[i].valid)
        {
            return i; // Use the first invalid line we find
        }
    }

    size_t least_insert_time_index =
        0; // All lines are valid, so we need to evict the oldest one from the FIFO queue
    size_t least_insert_time = UINT64_MAX;
    for (size_t i = 0; i < lines.size(); ++i)
    {
        if (lines[i].insertTime < least_insert_time)
        {
            least_insert_time = lines[i].insertTime;
            least_insert_time_index = i;
        }
    }
    // All lines are valid, so we need to evict the oldest one from the FIFO queue
    return least_insert_time_index;
}

void FIFOReplacementPolicy::onAccess(const CacheLineView &line, const CacheRequestView &request,
                                     const CacheContextView &context)
{
    // No internal state to update on access for FIFO
    (void)line;
    (void)request;
    (void)context;
}

void FIFOReplacementPolicy::onInsert(const CacheLineView &line, const CacheRequestView &request,
                                     const CacheContextView &context)
{
    // No internal state to update on insert for FIFO since we maintain the FIFO queue in the cache
    // metadata
    (void)line;
    (void)request;
    (void)context;
}

void FIFOReplacementPolicy::onEvict(const CacheLineView &line, const CacheRequestView &request,
                                    const CacheContextView &context)
{
    // No internal state to update on evict for FIFO since we maintain the FIFO queue in the cache
    // metadata
    (void)line;
    (void)request;
    (void)context;
}
}// namespace Kites