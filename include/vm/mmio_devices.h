/**
 * @file mmio_devices.h
 * @brief 
 * @author Vishank Singh, https://github.com/VishankSingh
 */
#pragma once

#include <cstdint>
#include <unordered_map>
#include <string>

class MMIODevice {
public:
    virtual ~MMIODevice() = default;

    /**
     * @brief Read a value from the MMIO device.
     * @param offset The offset to read from.
     * @return The value read from the device.
     */
    virtual uint32_t read(uint32_t offset) = 0;

    /**
     * @brief Write a value to the MMIO device.
     * @param offset The offset to write to.
     * @param value The value to write.
     */
    virtual void write(uint32_t offset, uint32_t value) = 0;

    /**
     * @brief Check if the MMIO device is ready for access.
     * @return True if the device is ready, false otherwise.
     */
    virtual bool isReady() const = 0;

    /**
     * @brief Get the size of the MMIO device.
     * @return The size of the device in bytes.
     */
    virtual uint32_t size() const = 0;  

    /**
     * @brief Get the base address of the MMIO device.
     * @return The base address of the device.
     */
    virtual uint32_t baseAddress() const = 0;

    /**
     * @brief Get the name of the MMIO device.
     * @return The name of the device.
     */
    virtual const char* name() const = 0;

    
}; // class MMIODevice


// macros for MMIO device management in assembler code



/**
 * @brief A simple MMIO device that does nothing.
 */
class NullMMIODevice : public MMIODevice {
public:
    uint32_t read(uint32_t offset) override {
        return 0;
    }

    void write(uint32_t offset, uint32_t value) override {
    }

    bool isReady() const override {
        return true; 
    }

    uint32_t size() const override {
        return 0;
    }

    uint32_t baseAddress() const override {
        return 0;
    }

    const char* name() const override {
        return "NullMMIODevice"; 
    }
}; // class NullMMIODevice
