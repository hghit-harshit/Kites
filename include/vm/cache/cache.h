/**
 * @file cache.h
 * @brief File containing the cache class for the virtual machine
 * @author Vishank Singh, https://github.com/VishankSingh
 */
#ifndef CACHE_H
#define CACHE_H

#include <cstdint>
#include <vector>

namespace cache {

enum class ReplacementPolicy {
  LRU,    ///< Least Recently Used
  FIFO,   ///< First In First Out
  Random  ///< Random replacement
};

enum class CacheType {
  Instruction, ///< Cache for instructions
  Data         ///< Cache for data
};

enum class CacheLineState {
  Valid,       ///< Cache line is valid
  Invalid,     ///< Cache line is invalid
  Dirty        ///< Cache line has been modified
};

enum class WriteHitPolicy {
  WriteThrough, ///< Write through policy
  WriteBack     ///< Write back policy
};

enum class WriteMissPolicy {
  NoWriteAllocate, ///< Do not allocate on write miss
  WriteAllocate    ///< Allocate on write miss
};

struct CacheConfig {
  unsigned long lines = 0;  ///< Number of lines in the cache
  unsigned long associativity = 0; ///< Associativity of the cache
  unsigned long words_per_line = 0; ///< Number of words per line in the cache
  ReplacementPolicy replacement_policy = ReplacementPolicy::LRU; ///< Replacement policy for the cache
  CacheType cache_type = CacheType::Data; ///< Type of cache (instruction or data)
  WriteHitPolicy write_hit_policy = WriteHitPolicy::WriteBack; ///< Write hit policy
  WriteMissPolicy write_miss_policy = WriteMissPolicy::NoWriteAllocate; ///< Write miss policy
  unsigned long size = 0;   ///< Size of the cache in bytes
};

struct CacheLine {
  CacheLineState state = CacheLineState::Invalid; ///< State of the cache line
  unsigned long tag = 0;    ///< Tag for the cache line
  std::vector<uint8_t> data; ///< Data stored in the cache line
  
};

struct CacheStats {
  unsigned long accesses; ///< Total number of accesses to the cache
  unsigned long hits;     ///< Total number of hits in the cache
  unsigned long misses;   ///< Total number of misses in the cache


};

struct CacheSet {
  unsigned long associativity; ///< Associativity of the cache set
  std::vector<CacheLine> lines; ///< Lines in the cache set

  CacheSet(unsigned long assoc)
    : associativity(assoc), lines(assoc) {}
};

class Cache {
  bool enabled; ///< Flag to indicate if the cache is enabled
  CacheType type; ///< Type of cache (instruction or data)
  CacheConfig config; ///< Configuration of the cache
  CacheStats stats; ///< Statistics for the cache


};



} // namespace cache



#endif // CACHE_H