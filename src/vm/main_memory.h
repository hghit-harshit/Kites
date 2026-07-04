/**
 * @file main_memory.h
 * @brief Contains the definition of the MemoryBlock and Memory classes.
 * @author Vishank Singh, https://github.com/VishankSingh
 */

#ifndef MAIN_MEMORY_H
#define MAIN_MEMORY_H

#include "config/config.h"
#include "memory_block.h"
#include "memory_device.h"
#include <cstdint>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace Kites
{
/**
 * @brief Represents a memory management system with dynamic memory block allocation.
 */

class MainMemory : public MemoryDevice
{
private:
    std::unordered_map<uint64_t, MemoryBlock>
        blocks_;              ///< A map storing memory blocks, indexed by block index.
    unsigned int block_size_; ///< The size of each memory block in bytes.
    uint64_t memory_size_ = vm_config::config.getMemorySize(); ///< The total memory size in bytes.

    /**
     * @brief Gets the block index for a given memory address.
     * @param address The memory address.
     * @return The block index corresponding to the address.
     */
    uint64_t getBlockIndex(uint64_t address) const;

    /**
     * @brief Gets the offset within a block for a given memory address.
     * @param address The memory address.
     * @return The offset within the block corresponding to the address.
     */
    uint64_t getBlockOffset(uint64_t address) const;

    /**
     * @brief Checks if a memory block is present at the specified index.
     * @param block_index The index of the block to check.
     * @return True if the block is present, false otherwise.
     */
    bool isBlockPresent(uint64_t block_index) const;

    /**
     * @brief Ensures that a memory block exists at the specified index, if not then adds it.
     * @param block_index The index of the block to check or create.
     */
    void ensureBlockExists(uint64_t block_index);

    /**
     * @brief Generic function to read data of type T from the memory.
     * @tparam T The type of data to read.
     * @param address The memory address to read from.
     * @return The value read from the specified memory address.
     */
    template <typename T> T readGeneric(uint64_t address);

    /**
     * @brief Generic function to write data of type T to the memory.
     * @tparam T The type of data to write.
     * @param address The memory address to write to.
     * @param value The value to write to the specified memory address.
     */
    template <typename T> void writeGeneric(uint64_t address, T value);
    
    /**
     * @brief Function to read cache Line 
     * @param address The memory address to read from.
     * @param lineSize The size of the line to read.
     * @return A span of bytes representing the data read from the specified memory address.
     */
    std::span<const uint8_t> readLine(uint64_t address, size_t lineSize) override;
    /**
     * @brief Writes a line of data to the cache.
     * @param address The memory address to write to.
     * @param data The data to write.
     */
    void writeLine(uint64_t address, std::span<const uint8_t> data) override;

  public:
    /**
     * @brief Constructs a Memory object.
     */
    MainMemory()
    {
        block_size_ = vm_config::config.getMemoryBlockSize();
    }
    /**
     * @brief Destroys the Memory object.
     */
    ~MainMemory() = default;

    void reset()
    {
        blocks_.clear();
    }

    /**
     * @brief Reads a single byte from the given memory address.
     * @param address The memory address to read from.
     * @return The byte value at the given address.
     */
    uint8_t read(uint64_t address);

    /**
     * @brief Writes a single byte to the given memory address.
     * @param address The memory address to write to.
     * @param value The byte value to write.
     */
    void write(uint64_t address, uint8_t value);

    /**
     * @brief Reads a single byte from the given memory address.
     * @param address The memory address to read from.
     * @return The byte value at the given address.
     */
    uint8_t readByte(uint64_t address);

    /**
     * @brief Reads a 16-bit halfword from the given memory address.
     * @param address The memory address to read from.
     * @return The 16-bit value at the given address.
     */
    uint16_t readHalfWord(uint64_t address);

    /**
     * @brief Reads a 32-bit word from the given memory address.
     * @param address The memory address to read from.
     * @return The 32-bit value at the given address.
     */
    uint32_t readWord(uint64_t address);

    /**
     * @brief Reads a 64-bit double word from the given memory address.
     * @param address The memory address to read from.
     * @return The 64-bit value at the given address.
     */
    uint64_t readDoubleWord(uint64_t address);

    float readFloat(uint64_t address);

    double readDouble(uint64_t address);

    /**
     * @brief Writes a single byte to the given memory address.
     * @param address The memory address to write to.
     * @param value The byte value to write.
     */
    void writeByte(uint64_t address, uint8_t value) override;

    /**
     * @brief Writes a 16-bit halfword to the given memory address.
     * @param address The memory address to write to.
     * @param value The 16-bit value to write.
     */
    void writeHalfWord(uint64_t address, uint16_t value) override;

    /**
     * @brief Writes a 32-bit word to the given memory address.
     * @param address The memory address to write to.
     * @param value The 32-bit value to write.
     */
    void writeWord(uint64_t address, uint32_t value) override;

    /**
     * @brief Writes a 64-bit double word to the given memory address.
     * @param address The memory address to write to.
     * @param value The 64-bit value to write.
     */
    void writeDoubleWord(uint64_t address, uint64_t value) override;

    void writeFloat(uint64_t address, float value);

    void writeDouble(uint64_t address, double value);

    void printMemory(uint64_t address, unsigned int rows);

    void dumpMemory(std::vector<std::string> args);

    void getMemoryPoint(std::string address);

    void printMemoryUsage() const;
};
}//namespace Kites
#endif // MAIN_MEMORY_H
