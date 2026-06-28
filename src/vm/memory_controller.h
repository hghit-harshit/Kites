/**
 * @file memory_controller.h
 * @brief Contains the declaration of the MemoryController class for managing memory in the VM.
 * @author Vishank Singh, https://github.com/VishankSingh
 */

#ifndef MEMORY_CONTROLLER_H
#define MEMORY_CONTROLLER_H

#include "config/config.h"
#include "cache/cache.h"
#include "main_memory.h"
#include <QObject>
#include <iostream>
#include <string>
#include <vector>

namespace Kites
{
/**
 * @brief The MemoryController class is responsible for managing memory in the VM.
 */
class MemoryController : public QObject
{
    Q_OBJECT
  private:
    MainMemory memory_;       ///< The main memory object.
    Cache l2_cache_;          ///< The second level cache object for even faster memory access.
    Cache l1_cache_;          ///< The cache object for faster memory access. 
    Cache instruction_cache_; ///< The cache object for instructions.

  public:
    MemoryController();

    void reset();

    void writeByte(uint64_t address, uint8_t value);
    void writeHalfWord(uint64_t address, uint16_t value);

    void writeWord(uint64_t address, uint32_t value);

    void writeDoubleWord(uint64_t address, uint64_t value);

    [[nodiscard]] uint8_t readByte(uint64_t address);
    [[nodiscard]] uint16_t readHalfWord(uint64_t address);
    [[nodiscard]] uint32_t readWord(uint64_t address);
    [[nodiscard]] uint64_t readDoubleWord(uint64_t address);
    // Functions to read memory directly with cache bypass
    [[nodiscard]] uint8_t readByte_d(uint64_t address);
    [[nodiscard]] uint16_t readHalfWord_d(uint64_t address);
    [[nodiscard]] uint32_t readWord_d(uint64_t address);
    [[nodiscard]] uint64_t readDoubleWord_d(uint64_t address);
    // function to read from instruction cache
    [[nodiscard]] uint32_t readInstruction(uint64_t address);
    // Functions to write directly to memory with cache bypass
    void writeByte_d(uint64_t address, uint8_t value);
    void writeHalfWord_d(uint64_t address, uint16_t value);
    void writeWord_d(uint64_t address, uint32_t value);
    void writeDoubleWord_d(uint64_t address, uint64_t value);

    void printMemory(const uint64_t address, unsigned int rows);
   
    void dumpMemory(std::vector<std::string> args);
    void getMemoryPoint(std::string address);

    Cache *getL1Cache();
    Cache *getL2Cache();
    Cache *getInstructionCache();
 
    // Cache* GetCache() { return &l1_cache_; }
  signals:
    void memoryUpdated(uint64_t address);
    void memoryResetSignal();
};
}// namespace Kites
#endif // MEMORY_CONTROLLER_H
