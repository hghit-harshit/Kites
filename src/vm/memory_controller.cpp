#include "memory_controller.h"

namespace Kites
{
    
MemoryController::MemoryController() : 
l2_cache_(memory_), 
l1_cache_(static_cast<MemoryDevice&>(l2_cache_)), 
instruction_cache_(static_cast<MemoryDevice&>(l2_cache_)) // casting needed here otherwise
{}                                                        // compiler think we are calling 
                                                          // copy constructor


void MemoryController::reset()
{
    memory_.reset();
    l1_cache_.reset();
    l2_cache_.reset();
    instruction_cache_.reset();
    emit memoryResetSignal(); // this will notify views to reset themselves
}

void MemoryController::writeByte(uint64_t address, uint8_t value)
{
    l1_cache_.writeByte(address, value);
    emit memoryUpdated(address);
}

void MemoryController::writeHalfWord(uint64_t address, uint16_t value)
{
    l1_cache_.writeHalfWord(address, value);
    emit memoryUpdated(address);
}

void MemoryController::writeWord(uint64_t address, uint32_t value)
{
    l1_cache_.writeWord(address, value);
    emit memoryUpdated(address);
}

void MemoryController::writeDoubleWord(uint64_t address, uint64_t value)
{
    l1_cache_.writeDoubleWord(address, value);
    emit memoryUpdated(address);
}

uint8_t MemoryController::readByte(uint64_t address)
{
    return l1_cache_.readByte(address);
}

uint16_t MemoryController::readHalfWord(uint64_t address)
{
    return l1_cache_.readHalfWord(address);
}

uint32_t MemoryController::readWord(uint64_t address)
{
    return l1_cache_.readWord(address);
}

uint64_t MemoryController::readDoubleWord(uint64_t address)
{
    return l1_cache_.readDoubleWord(address);
}

// Functions to read memory directly with cache bypass

uint8_t MemoryController::readByte_d(uint64_t address)
{
    return memory_.readByte(address);
}

uint16_t MemoryController::readHalfWord_d(uint64_t address)
{
    return memory_.readHalfWord(address);
}

uint32_t MemoryController::readWord_d(uint64_t address)
{
    return memory_.readWord(address);
}

uint64_t MemoryController::readDoubleWord_d(uint64_t address)
{
    return memory_.readDoubleWord(address);
}

// function to read from instruction cache
uint32_t MemoryController::readInstruction(uint64_t address)
{
    return instruction_cache_.readWord(address);
}

// Functions to write directly to memory with cache bypass
void MemoryController::writeByte_d(uint64_t address, uint8_t value)
{
    memory_.writeByte(address, value);
    emit memoryUpdated(address);
}
void MemoryController::writeHalfWord_d(uint64_t address, uint16_t value)
{
    memory_.writeHalfWord(address, value);
    emit memoryUpdated(address);
}
void MemoryController::writeWord_d(uint64_t address, uint32_t value)
{
    memory_.writeWord(address, value);
    emit memoryUpdated(address);
}
void MemoryController::writeDoubleWord_d(uint64_t address, uint64_t value)
{
    memory_.writeDoubleWord(address, value);
    emit memoryUpdated(address);
}

void MemoryController::printMemory(const uint64_t address, unsigned int rows)
{
    memory_.printMemory(address, rows);
}

void MemoryController::dumpMemory(std::vector<std::string> args)
{
    memory_.dumpMemory(args);
}

void MemoryController::getMemoryPoint(std::string address)
{
    return memory_.getMemoryPoint(address);
}

Cache *MemoryController::getL1Cache()
{
    return &l1_cache_;
}
Cache *MemoryController::getL2Cache()
{
    return &l2_cache_;
}
Cache *MemoryController::getInstructionCache()
{
    return &instruction_cache_;
}
}