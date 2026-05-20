#pragma once
#include "config.h"
#include <vector>

/**
 * @brief Represents a memory block containing 1 KB of memory.
 */
struct MemoryBlock
{
    std::vector<uint8_t> data; ///< A vector representing the memory block data.
    unsigned int block_size =
        vm_config::config.getMemoryBlockSize(); ///< The size of the memory block in bytes.

    /**
     * @brief Constructs a MemoryBlock with a size of 1 KB initialized to 0.
     */
    MemoryBlock()
    {
        data.resize(block_size, 0);
    }
};