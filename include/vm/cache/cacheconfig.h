#include <cstdint>
#pragma once
enum class WritePolicy
{
    WriteThrough,
    WriteBack
};

enum class AllocationPolicy
{
    WriteAllocate,
    NoWriteAllocate
};

enum class ReplacementPolicy
{
    LRU,
    FIFO
};

struct CacheConfig
{
    size_t num_lines;
    size_t block_size;
    size_t num_ways;
    WritePolicy write_policy;
    AllocationPolicy allocation_policy;
    ReplacementPolicy replacement_policy;
};