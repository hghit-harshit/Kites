/**
 * @file main_memory.cpp
 * @brief Contains the implementation of the Memory class.
 * @author Vishank Singh, https://github.com/VishankSingh
 */

#include "vm/main_memory.h"
#include "common/globals.h"

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <vector>

namespace Kites
{
uint8_t MainMemory::read(uint64_t address)
{
    if (address >= memory_size_)
    {
        throw std::out_of_range("Memory address out of range: " + std::to_string(address));
    }
    uint64_t block_index = getBlockIndex(address);
    uint64_t offset = getBlockOffset(address);
    if (!isBlockPresent(block_index))
    {
        return 0;
    }
    return blocks_[block_index].data[offset];
}

void MainMemory::write(uint64_t address, uint8_t value)
{
    if (address >= memory_size_)
    {
        throw std::out_of_range(std::string("Memory address out of range: ") +
                                std::to_string(address));
    }
    uint64_t block_index = getBlockIndex(address);
    uint64_t offset = getBlockOffset(address);
    ensureBlockExists(block_index);
    blocks_[block_index].data[offset] = value;
}

uint64_t MainMemory::getBlockIndex(uint64_t address) const
{
    return address / block_size_;
}

uint64_t MainMemory::getBlockOffset(uint64_t address) const
{
    return address % block_size_;
}

bool MainMemory::isBlockPresent(uint64_t block_index) const
{
    return blocks_.find(block_index) != blocks_.end();
}

void MainMemory::ensureBlockExists(uint64_t block_index)
{
    if (blocks_.find(block_index) == blocks_.end())
    {
        blocks_.emplace(block_index, MemoryBlock());
    }
}

template <typename T> T MainMemory::readGeneric(uint64_t address)
{
    T value = 0;
    for (size_t i = 0; i < sizeof(T); ++i)
    {
        value |= static_cast<T>(read(address + i)) << (8 * i);
    }
    return value;
}

std::span<const uint8_t> MainMemory::readLine(uint64_t address, size_t lineSize)
{
    if (address + lineSize > memory_size_)
    {
        throw std::out_of_range(std::string("Memory address out of range: ") +
                                std::to_string(address));
    }
    uint64_t block_index = getBlockIndex(address);
    uint64_t offset = getBlockOffset(address);
    ensureBlockExists(block_index);
    return std::span<const uint8_t>(blocks_[block_index].data.data() + offset, lineSize);
}

void MainMemory::writeLine(uint64_t address, std::span<const uint8_t> data)
{
    if (address + data.size() > memory_size_)
    {
        throw std::out_of_range(std::string("Memory address out of range: ") +
                                std::to_string(address));
    }
    uint64_t block_index = getBlockIndex(address);
    uint64_t offset = getBlockOffset(address);
    ensureBlockExists(block_index);
    std::copy(data.begin(), data.end(), blocks_[block_index].data.begin() + offset);
}

template <typename T> void MainMemory::writeGeneric(uint64_t address, T value)
{
    for (size_t i = 0; i < sizeof(T); ++i)
    {
        write(address + i, static_cast<uint8_t>(value >> (8 * i)));
    }
}

uint8_t MainMemory::readByte(uint64_t address)
{
    if (address >= memory_size_)
    {
        throw std::out_of_range(std::string("Memory address out of range: ") +
                                std::to_string(address));
    }
    return read(address);
}

uint16_t MainMemory::readHalfWord(uint64_t address)
{
    if (address >= memory_size_ - 1)
    {
        throw std::out_of_range(std::string("Memory address out of range: ") +
                                std::to_string(address));
    }
    return readGeneric<uint16_t>(address);
}

uint32_t MainMemory::readWord(uint64_t address)
{
    if (address >= memory_size_ - 3)
    {
        throw std::out_of_range(std::string("Memory address out of range: ") +
                                std::to_string(address));
    }
    return readGeneric<uint32_t>(address);
}

uint64_t MainMemory::readDoubleWord(uint64_t address)
{
    if (address >= memory_size_ - 7)
    {
        throw std::out_of_range(std::string("Memory address out of range: ") +
                                std::to_string(address));
    }
    return readGeneric<uint64_t>(address);
}

float MainMemory::readFloat(uint64_t address)
{
    if (address >= memory_size_ - (sizeof(float) - 1))
    {
        throw std::out_of_range(std::string("Memory address out of range: ") +
                                std::to_string(address));
        ;
    }
    uint32_t value = 0;
    for (size_t i = 0; i < sizeof(float); ++i)
    {
        value |= static_cast<uint32_t>(read(address + i)) << (8 * i);
    }
    float result;
    std::memcpy(&result, &value, sizeof(float));
    return result;
}

double MainMemory::readDouble(uint64_t address)
{
    if (address >= memory_size_ - (sizeof(double) - 1))
    {
        throw std::out_of_range(std::string("Memory address out of range: ") +
                                std::to_string(address));
    }
    uint64_t value = 0;
    for (size_t i = 0; i < sizeof(double); ++i)
    {
        value |= static_cast<uint64_t>(read(address + i)) << (8 * i);
    }
    double result;
    std::memcpy(&result, &value, sizeof(double));
    return result;
}

void MainMemory::writeByte(uint64_t address, uint8_t value)
{
    if (address >= memory_size_)
    {
        throw std::out_of_range(std::string("Memory address out of range: ") +
                                std::to_string(address));
    }
    write(address, value);
}

void MainMemory::writeHalfWord(uint64_t address, uint16_t value)
{
    if (address >= memory_size_ - 1)
    {
        throw std::out_of_range(std::string("Memory address out of range: ") +
                                std::to_string(address));
    }
    writeGeneric<uint16_t>(address, value);
}

void MainMemory::writeWord(uint64_t address, uint32_t value)
{
    if (address >= memory_size_ - 3)
    {
        throw std::out_of_range(std::string("Memory address out of range: ") +
                                std::to_string(address));
    }
    writeGeneric<uint32_t>(address, value);
}

void MainMemory::writeDoubleWord(uint64_t address, uint64_t value)
{
    if (address >= memory_size_ - 7)
    {
        throw std::out_of_range(std::string("Memory address out of range: ") +
                                std::to_string(address));
    }
    writeGeneric<uint64_t>(address, value);
}

void MainMemory::writeFloat(uint64_t address, float value)
{
    if (address >= memory_size_ - (sizeof(float) - 1))
    {
        throw std::out_of_range(std::string("Memory address out of range: ") +
                                std::to_string(address));
    }
    uint32_t value_bits;
    std::memcpy(&value_bits, &value, sizeof(float));
    for (size_t i = 0; i < sizeof(float); ++i)
    {
        write(address + i, static_cast<uint8_t>((value_bits >> (8 * i)) & 0xFF));
    }
}

void MainMemory::writeDouble(uint64_t address, double value)
{
    if (address >= memory_size_ - (sizeof(double) - 1))
    {
        throw std::out_of_range(std::string("Memory address out of range: ") +
                                std::to_string(address));
    }
    uint64_t value_bits;
    std::memcpy(&value_bits, &value, sizeof(double));
    for (size_t i = 0; i < sizeof(double); ++i)
    {
        write(address + i, static_cast<uint8_t>((value_bits >> (8 * i)) & 0xFF));
    }
}

void MainMemory::printMemory(const uint64_t address, unsigned int rows)
{
    constexpr size_t bytes_per_row = 8; // One row equals 64 bytes
    std::cout << "Memory Dump at Address: 0x" << std::hex << address << std::dec << "\n";
    std::cout << "-----------------------------------------------------------------\n";
    for (uint64_t i = 0; i < rows; ++i)
    {
        uint64_t current_address = address + (i * bytes_per_row);
        if (current_address >= memory_size_)
        {
            break;
        }
        std::cout << "0x" << std::hex << std::setw(16) << std::setfill('0') << current_address
                  << " | ";
        for (size_t j = 0; j < bytes_per_row; ++j)
        {
            if (current_address + j >= memory_size_)
            {
                break;
            }
            std::cout << std::hex << std::setw(2) << std::setfill('0')
                      << static_cast<int>(read(current_address + j)) << " ";
        }
        std::cout << "| 0x" << std::hex << std::setw(16) << std::setfill('0')
                  << static_cast<int64_t>(readDoubleWord(current_address));
        std::cout << std::dec << std::setfill(' ') << "\n";
    }
    std::cout << "-----------------------------------------------------------------\n";
}

void MainMemory::dumpMemory(std::vector<std::string> args)
{
    std::ofstream file(globals::memory_dump_file_path);
    if (!file.is_open())
    {
        throw std::runtime_error("Unable to open memory dump file: " +
                                 globals::memory_dump_file_path.string());
    }
    file << "{\n";

    for (size_t i = 0; i < args.size(); i += 2)
    {
        if (i + 1 >= args.size())
        {
            throw std::invalid_argument("Invalid number of arguments for memory dump.");
        }
        uint64_t address = std::stoull(args[i], nullptr, 16);
        uint64_t rows = std::stoull(args[i + 1]);
        for (uint64_t j = 0; j < rows; ++j)
        {
            uint64_t current_address = address + (j * 8);
            if (current_address >= memory_size_)
            {
                break;
            }
            file << R"(    "0x)" << std::hex << std::setw(16) << std::setfill('0')
                 << current_address << R"(": )";

            file << R"("0x)";
            for (int k = 7; k >= 0; k--)
            {
                file << std::hex << std::setw(2) << std::setfill('0')
                     << static_cast<int>(read(address + j * 8 + k));
            }
            file << R"(")";
            if (j < rows - 1 || i < args.size() - 2)
            {
                file << ",";
            }
            file << std::setfill(' ') << "\n";
        }
        if (i < args.size() - 2)
        {
            file << "\n";
        }
    }

    file << "}\n";
    file.close();

    std::cout << "VM_MEMORY_DUMPED" << std::endl;
}

void MainMemory::getMemoryPoint(std::string addr_str)
{
    uint64_t address = std::stoull(addr_str, nullptr, 16);
    if (address >= memory_size_ - 7)
    {
        throw std::out_of_range("Memory address out of range: " + std::to_string(address));
    }

    uint64_t value = readDoubleWord(address);
    // std::stringstream ss;
    std::cerr << "VM_MEMORY_POINT_START";
    std::cerr << addr_str;
    std::cerr << "[0x";

    for (int i = 7; i >= 0; --i)
    {
        uint8_t byte = (value >> (i * 8)) & 0xFF;
        std::cerr << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(byte)
                  << std::dec << std::setfill(' ');
    }

    std::cerr << "]";
    std::cerr << "VM_MEMORY_POINT_END" << std::endl;
}

void MainMemory::printMemoryUsage() const
{
    std::cout << "Memory Usage Report:\n";
    std::cout << "---------------------\n";
    std::cout << "Block Count: " << blocks_.size() << "\n";
    for (const auto &[block_index, block] : blocks_)
    {
        size_t used_bytes = std::count_if(block.data.begin(), block.data.end(),
                                          [](uint8_t byte) { return byte != 0; });
        if (used_bytes > 0)
        {
            std::cout << "Block " << block_index << ": " << used_bytes << " / " << block_size_
                      << " bytes used\n";
        }
    }
}
}//namespace Kites
