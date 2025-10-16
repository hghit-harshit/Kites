/**
 * @file main_memory.h
 * @brief Contains the definition of the MemoryBlock and Memory classes.
 * @author Vishank Singh, https://github.com/VishankSingh
 */

#ifndef MAIN_MEMORY_H
#define MAIN_MEMORY_H

#include "config.h"

#include <vector>
#include <unordered_map>
#include <cstdint>
#include <string>
#include <stdexcept>

/**
 * @brief Represents a memory block containing 1 KB of memory.
 */
struct MemoryBlock {
  std::vector<uint8_t> data; ///< A vector representing the memory block data.
  unsigned int block_size = vm_config::config.getMemoryBlockSize(); ///< The size of the memory block in bytes.

  /**
   * @brief Constructs a MemoryBlock with a size of 1 KB initialized to 0.
   */
  MemoryBlock() {
    data.resize(block_size, 0);
  }
};

/**
 * @brief Represents a memory management system with dynamic memory block allocation.
 */
class Memory {
 private:
  std::unordered_map<uint64_t, MemoryBlock> blocks_; ///< A map storing memory blocks, indexed by block index.
  unsigned int block_size_; ///< The size of each memory block in bytes.
  uint64_t memory_size_ = vm_config::config.getMemorySize(); ///< The total memory size in bytes.

  /**
   * @brief Gets the block index for a given memory address.
   * @param address The memory address.
   * @return The block index corresponding to the address.
   */
  uint64_t GetBlockIndex(uint64_t address) const;

  /**
   * @brief Gets the offset within a block for a given memory address.
   * @param address The memory address.
   * @return The offset within the block corresponding to the address.
   */
  uint64_t GetBlockOffset(uint64_t address) const;

  /**
   * @brief Checks if a memory block is present at the specified index.
   * @param block_index The index of the block to check.
   * @return True if the block is present, false otherwise.
   */
  bool IsBlockPresent(uint64_t block_index) const;

  /**
   * @brief Ensures that a memory block exists at the specified index, if not then adds it.
   * @param block_index The index of the block to check or create.
   */
  void EnsureBlockExists(uint64_t block_index);

  /**
   * @brief Generic function to read data of type T from the memory.
   * @tparam T The type of data to read.
   * @param address The memory address to read from.
   * @return The value read from the specified memory address.
   */
  template<typename T>
  T ReadGeneric(uint64_t address);

  /**
   * @brief Generic function to write data of type T to the memory.
   * @tparam T The type of data to write.
   * @param address The memory address to write to.
   * @param value The value to write to the specified memory address.
   */
  template<typename T>
  void WriteGeneric(uint64_t address, T value);

 public:
  /**
   * @brief Constructs a Memory object.
   */
  Memory() {
    block_size_ = vm_config::config.getMemoryBlockSize();
  }
  /**
   * @brief Destroys the Memory object.
   */
  ~Memory() = default;

  void Reset() {
    blocks_.clear();
  }

  /**
   * @brief Reads a single byte from the given memory address.
   * @param address The memory address to read from.
   * @return The byte value at the given address.
   */
  uint8_t Read(uint64_t address);

  /**
   * @brief Writes a single byte to the given memory address.
   * @param address The memory address to write to.
   * @param value The byte value to write.
   */
  void Write(uint64_t address, uint8_t value);

  /**
  * @brief Reads a single byte from the given memory address.
  * @param address The memory address to read from.
  * @return The byte value at the given address.
  */
  uint8_t ReadByte(uint64_t address);

  /**
   * @brief Reads a 16-bit halfword from the given memory address.
   * @param address The memory address to read from.
   * @return The 16-bit value at the given address.
   */
  uint16_t ReadHalfWord(uint64_t address);

  /**
   * @brief Reads a 32-bit word from the given memory address.
   * @param address The memory address to read from.
   * @return The 32-bit value at the given address.
   */
  uint32_t ReadWord(uint64_t address);

  /**
   * @brief Reads a 64-bit double word from the given memory address.
   * @param address The memory address to read from.
   * @return The 64-bit value at the given address.
   */
  uint64_t ReadDoubleWord(uint64_t address);

  float ReadFloat(uint64_t address);

  double ReadDouble(uint64_t address);

  /**
   * @brief Writes a single byte to the given memory address.
   * @param address The memory address to write to.
   * @param value The byte value to write.
   */
  void WriteByte(uint64_t address, uint8_t value);

  /**
   * @brief Writes a 16-bit halfword to the given memory address.
   * @param address The memory address to write to.
   * @param value The 16-bit value to write.
   */
  void WriteHalfWord(uint64_t address, uint16_t value);

  /**
   * @brief Writes a 32-bit word to the given memory address.
   * @param address The memory address to write to.
   * @param value The 32-bit value to write.
   */
  void WriteWord(uint64_t address, uint32_t value);

  /**
   * @brief Writes a 64-bit double word to the given memory address.
   * @param address The memory address to write to.
   * @param value The 64-bit value to write.
   */
  void WriteDoubleWord(uint64_t address, uint64_t value);

  void WriteFloat(uint64_t address, float value);

  void WriteDouble(uint64_t address, double value);

  void PrintMemory(uint64_t address, unsigned int rows);

  void DumpMemory(std::vector<std::string> args);

  void GetMemoryPoint(std::string address);

  void printMemoryUsage() const;
};

#endif // MAIN_MEMORY_H
