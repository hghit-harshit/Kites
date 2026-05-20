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

enum class ReplacementPolicy : size_t
{
    LRU = 0,
    FIFO,
    Custom
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

struct CacheStats
{
    size_t hits = 0;
    size_t misses = 0;
    size_t writeBacks = 0;
};