#pragma once
#include <cstdint>
#include <span>
namespace Kites
{

/**
 * @brief This class is an abtraction for memory devices 
 * mainly we will use this class to represent memory and cache as memory devices
 * 
 * 
 */
class Cache;
class MainMemory;
class MemoryDevice
{
    friend class Cache;
    friend class MainMemory;
    public: 
    MemoryDevice() = default;
    virtual ~MemoryDevice() = default;
    virtual void writeByte(uint64_t address, uint8_t value) = 0;
    virtual void writeHalfWord(uint64_t address, uint16_t value) = 0;
    virtual void writeWord(uint64_t address, uint32_t value) = 0;
    virtual void writeDoubleWord(uint64_t address, uint64_t value) = 0;
    //virtual void writeFloat(uint64_t address, float value) = 0;
    //virtual void writeDouble(uint64_t address, double value) = 0;
    private: 
    virtual std::span<const uint8_t> readLine(uint64_t address, size_t lineSize) = 0;
    virtual void writeLine(uint64_t address, std::span<const uint8_t> data) = 0;
};
}//namespace Kites