#pragma once

#include "vm/cache/cacheconfig.h"
#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>

/**
 * @brief This scructure provides a read-only view of a cache line for replacement policies.
 *
 */
struct CacheLineView
{
    bool valid = false;
    uint64_t tag = 0;
    uint64_t age = 0;
    uint64_t frequency = 0;
    uint64_t insertTime = 0;
    uint64_t lastAccess = 0;
    bool dirty = false;
};

/**
 * @brief This structure provides a read-only view of a cache request for replacement policies.
 * This represents the current memory access being processed
 *
 */
struct CacheRequestView
{
    uint64_t address = 0;
    size_t setIndex = 0;
    size_t wayIndex = 0;
    size_t offset = 0;
    size_t accessSize = 1;
    bool isWrite = false;
    uint64_t tag = 0;
};

/**
 * @brief This structure provides contextual information about the cache state for replacement
 * policies. Ths is basically a snapshot of the cache configuration.
 *
 */
struct CacheContextView
{
    size_t setCount = 0;
    size_t wayCount = 0;
    size_t blockSize = 0;
    uint64_t tick = 0;
};

/**
 * @brief Base class for cache replacement policies.
 *
 * Policies make victim decisions from read-only cache views and maintain
 * their own internal state.
 */
class CacheReplacementPolicy
{
  public:
    virtual ~CacheReplacementPolicy() = default;

    /**
     * @brief Given the current state of the cache lines in a set, the incoming request, and the
     * overall cache context, choose a victim line index for eviction.
     *
     * @param lines A span of read-only views of the cache lines in the set
     * @param request A read-only view of the incoming cache request
     * @param context A read-only view of the cache context
     * @return size_t The index of the line to evict (0 to wayCount-1)
     */
    virtual size_t chooseVictim(std::span<const CacheLineView> lines,
                                const CacheRequestView &request,
                                const CacheContextView &context) = 0;

    virtual void onAccess(const CacheLineView &line, const CacheRequestView &request,
                          const CacheContextView &context) = 0;

    virtual void onInsert(const CacheLineView &line, const CacheRequestView &request,
                          const CacheContextView &context) = 0;

    virtual void onEvict(const CacheLineView &line, const CacheRequestView &request,
                         const CacheContextView &context) = 0;

    virtual std::string_view name() const = 0;

    virtual ReplacementPolicy type() const = 0;
};