/**
 * @file memory_controller.h
 * @brief Contains the declaration of the MemoryController class for managing memory in the VM.
 * @author Vishank Singh, https://github.com/VishankSingh
 */

#ifndef MEMORY_CONTROLLER_H
#define MEMORY_CONTROLLER_H

#include "../config.h"
#include "cache/cache.h"
#include "main_memory.h"
#include <QObject>
#include <iostream>
#include <string>
#include <vector>

/**
 * @brief The MemoryController class is responsible for managing memory in the VM.
 */
class MemoryController : public QObject
{
    Q_OBJECT
  private:
    Memory memory_;           ///< The main memory object.
    Cache l1_cache_;          ///< The cache object for faster memory access.
    Cache l2_cache_;          ///< The second level cache object for even faster memory access.
    Cache instruction_cache_; ///< The cache object for instructions.

  public:
    MemoryController() : l2_cache_(memory_), l1_cache_(l2_cache_), instruction_cache_(l2_cache_)
    {
    }

    void Reset()
    {
        memory_.Reset();
        l1_cache_.Reset();
        l2_cache_.Reset();
        instruction_cache_.Reset();
        emit memoryResetSignal(); // this will notify views to reset themselves
    }

    void PrintCacheStatus() const
    {
    }

    void WriteByte(uint64_t address, uint8_t value)
    {
        l1_cache_.WriteByte(address, value);
        emit memoryUpdated(address);
    }

    void WriteHalfWord(uint64_t address, uint16_t value)
    {
        l1_cache_.WriteHalfWord(address, value);
        emit memoryUpdated(address);
    }

    void WriteWord(uint64_t address, uint32_t value)
    {
        l1_cache_.WriteWord(address, value);
        emit memoryUpdated(address);
    }

    void WriteDoubleWord(uint64_t address, uint64_t value)
    {
        l1_cache_.WriteDoubleWord(address, value);
        emit memoryUpdated(address);
    }

    [[nodiscard]] uint8_t ReadByte(uint64_t address)
    {
        return l1_cache_.ReadByte(address);
    }

    [[nodiscard]] uint16_t ReadHalfWord(uint64_t address)
    {
        return l1_cache_.ReadHalfWord(address);
    }

    [[nodiscard]] uint32_t ReadWord(uint64_t address)
    {
        return l1_cache_.ReadWord(address);
    }

    [[nodiscard]] uint64_t ReadDoubleWord(uint64_t address)
    {
        return l1_cache_.ReadDoubleWord(address);
    }

    // Functions to read memory directly with cache bypass

    [[nodiscard]] uint8_t ReadByte_d(uint64_t address)
    {
        return memory_.ReadByte(address);
    }

    [[nodiscard]] uint16_t ReadHalfWord_d(uint64_t address)
    {
        return memory_.ReadHalfWord(address);
    }

    [[nodiscard]] uint32_t ReadWord_d(uint64_t address)
    {
        return memory_.ReadWord(address);
    }

    [[nodiscard]] uint64_t ReadDoubleWord_d(uint64_t address)
    {
        return memory_.ReadDoubleWord(address);
    }

    // function to read from instruction cache
    [[nodiscard]] uint32_t ReadInstruction(uint64_t address)
    {
        return instruction_cache_.ReadWord(address);
    }

    // Functions to write directly to memory with cache bypass
    void WriteByte_d(uint64_t address, uint8_t value)
    {
        memory_.WriteByte(address, value);
        emit memoryUpdated(address);
    }
    void WriteHalfWord_d(uint64_t address, uint16_t value)
    {
        memory_.WriteHalfWord(address, value);
        emit memoryUpdated(address);
    }
    void WriteWord_d(uint64_t address, uint32_t value)
    {
        memory_.WriteWord(address, value);
        emit memoryUpdated(address);
    }
    void WriteDoubleWord_d(uint64_t address, uint64_t value)
    {
        memory_.WriteDoubleWord(address, value);
        emit memoryUpdated(address);
    }

    void PrintMemory(const uint64_t address, unsigned int rows)
    {
        memory_.PrintMemory(address, rows);
    }

    void DumpMemory(std::vector<std::string> args)
    {
        memory_.DumpMemory(args);
    }

    void GetMemoryPoint(std::string address)
    {
        return memory_.GetMemoryPoint(address);
    }

    Cache *GetL1Cache()
    {
        return &l1_cache_;
    }
    Cache *GetL2Cache()
    {
        return &l2_cache_;
    }
    Cache *GetInstructionCache()
    {
        return &instruction_cache_;
    }
    // Cache* GetCache() { return &l1_cache_; }
  signals:
    void memoryUpdated(uint64_t address);
    void memoryResetSignal();
};

#endif // MEMORY_CONTROLLER_H
