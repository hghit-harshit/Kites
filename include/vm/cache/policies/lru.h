#pragma once
#include "cache_replacement_policy.h"

class LRUReplacementPolicy : public CacheReplacementPolicy
{
public:
    LRUReplacementPolicy() = default;
    ~LRUReplacementPolicy() = default;

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

};