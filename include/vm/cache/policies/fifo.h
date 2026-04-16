#pragma once
#include "cache_replacement_policy.h"
#include <queue>

class FIFOReplacementPolicy : public CacheReplacementPolicy
{
public:
    FIFOReplacementPolicy() = default;
    ~FIFOReplacementPolicy() = default;

    size_t chooseVictim(std::span<const CacheLineView> lines,
                        const CacheRequestView& request,
                        const CacheContextView& context) override;

    void onAccess(const CacheLineView& line,
                  const CacheRequestView& request,
                  const CacheContextView& context) override;

    void onInsert(const CacheLineView& line,
                  const CacheRequestView& request,
                  const CacheContextView& context) override;

    void onEvict(const CacheLineView& line,
                 const CacheRequestView& request,
                 const CacheContextView& context) override;

    std::string_view name() const override;

private:
        //std::vector<std::queue<size_t>> fifo_queues; // Queue to maintain the order of lines for FIFO eviction
};