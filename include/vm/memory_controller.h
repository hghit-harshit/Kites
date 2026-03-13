/**
 * @file memory_controller.h
 * @brief Contains the declaration of the MemoryController class for managing memory in the VM.
 * @author Vishank Singh, https://github.com/VishankSingh
 */

#ifndef MEMORY_CONTROLLER_H
#define MEMORY_CONTROLLER_H

#include "../config.h"
#include "main_memory.h"
#include "cache/direct_map_cache.h"
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
    Memory memory_; ///< The main memory object.
    DirectMapCache cache_; ///< The cache object for faster memory access.

public:
    MemoryController():cache_(memory_)
    {
    }

    void Reset() {
        memory_.Reset();
        emit memoryResetSignal(); // this will notify views to reset themselves
    }

    void PrintCacheStatus() const {
    }

    void WriteByte(uint64_t address, uint8_t value) {
      cache_.WriteByte(address, value);
      emit memoryUpdated(address);
    }

    void WriteHalfWord(uint64_t address, uint16_t value) {
      cache_.WriteHalfWord(address, value);
      emit memoryUpdated(address);
    }

    void WriteWord(uint64_t address, uint32_t value) {
      cache_.WriteWord(address, value);
      emit memoryUpdated(address);
    }

    void WriteDoubleWord(uint64_t address, uint64_t value) {
      cache_.WriteDoubleWord(address, value);
      emit memoryUpdated(address);
    }

    [[nodiscard]] uint8_t ReadByte(uint64_t address) {
        return cache_.ReadByte(address);
    }

    [[nodiscard]] uint16_t ReadHalfWord(uint64_t address) {
        return cache_.ReadHalfWord(address);
    }

    [[nodiscard]] uint32_t ReadWord(uint64_t address) {
        return cache_.ReadWord(address);
    }

    [[nodiscard]] uint64_t ReadDoubleWord(uint64_t address) {
        return cache_.ReadDoubleWord(address);
    }

    // Functions to read memory directly with cache bypass

    [[nodiscard]] uint8_t ReadByte_d(uint64_t address) {
        return memory_.ReadByte(address);
    }

    [[nodiscard]] uint16_t ReadHalfWord_d(uint64_t address) {
        return memory_.ReadHalfWord(address);
    }

    [[nodiscard]] uint32_t ReadWord_d(uint64_t address) {
        return memory_.ReadWord(address);
    }

    [[nodiscard]] uint64_t ReadDoubleWord_d(uint64_t address) {
        return memory_.ReadDoubleWord(address);
    }

    void PrintMemory(const uint64_t address, unsigned int rows) {
      memory_.PrintMemory(address, rows);
    }

    void DumpMemory(std::vector<std::string> args) {
      memory_.DumpMemory(args);
    }

    void GetMemoryPoint(std::string address) {
      return memory_.GetMemoryPoint(address);
    }

    DirectMapCache* GetCache() { return &cache_; }
    signals:
    void memoryUpdated(uint64_t address);
    void memoryResetSignal();

};

#endif // MEMORY_CONTROLLER_H

