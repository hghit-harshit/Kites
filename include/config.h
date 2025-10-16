/**
 * @file config.h
 * @brief Contains configuration options for the assembler.
 * @author Vishank Singh, https://github.com/VishankSingh
 */
#ifndef CONFIG_H
#define CONFIG_H

#include "globals.h"
#include <string>
#include <iostream>
#include <stdexcept>
#include <cstdint>
#include <fstream>
#include <sstream>

/**
 * @namespace vm_config
 * @brief Namespace for VM configuration management.
 */
namespace vm_config {
enum class VmTypes {
  SINGLE_STAGE,
  MULTI_STAGE
};

struct VmConfig {
  VmTypes vm_type = VmTypes::SINGLE_STAGE;
  uint64_t run_step_delay = 300;
  uint64_t memory_size = 0xffffffffffffffff; // 64-bit address space
  uint64_t memory_block_size = 1024; // 1 KB blocks
  uint64_t data_section_start = 0x10000000; // Default start address for data section
  uint64_t text_section_start = 0x0; // Default start address for text section
  uint64_t bss_section_start = 0x11000000; // Default start address for BSS section

  void setVmType(const VmTypes &type) {
    vm_type = type;
  }

  VmTypes getVmType() const {
    return vm_type;
  }
  void setRunStepDelay(uint64_t delay) {
    run_step_delay = delay;
    std::cout << "Run step delay set to: " << run_step_delay << " ms" << std::endl;
  }
  uint64_t getRunStepDelay() const {
    return run_step_delay;
  }
  void setMemorySize(uint64_t size) {
    memory_size = size;
  }
  uint64_t getMemorySize() const {
    return memory_size;
  }
  void setMemoryBlockSize(uint64_t size) {
    memory_block_size = size;
  }
  uint64_t getMemoryBlockSize() const {
    return memory_block_size;
  }
  void setDataSectionStart(uint64_t start) {
    data_section_start = start;
  }
  uint64_t getDataSectionStart() const {
    return data_section_start;
  }

  void setTextSectionStart(uint64_t start) {
    text_section_start = start;
  }

  uint64_t getTextSectionStart() const {
    return text_section_start;
  }

  void setBssSectionStart(uint64_t start) {
    bss_section_start = start;
  }

  uint64_t getBssSectionStart() const {
    return bss_section_start;
  }

  void modifyConfig(const std::string &section, const std::string &key, const std::string &value) {
    if (section == "Execution") {
      if (key == "processor_type") {
        if (value == "single_stage") {
          setVmType(VmTypes::SINGLE_STAGE);
        } else if (value == "multi_stage") {
          setVmType(VmTypes::MULTI_STAGE);
        } else {
          throw std::invalid_argument("Unknown VM type: " + value);
        }
      } else if (key == "run_step_delay") {
        setRunStepDelay(std::stoull(value));
      } else {
        throw std::invalid_argument("Unknown key: " + key);
      }
    } else if (section == "Memory") {
      if (key == "memory_size") {
        setMemorySize(std::stoull(value));
      } else if (key == "memory_block_size") {
        setMemoryBlockSize(std::stoull(value));
      } else if (key == "data_section_start") {
        setDataSectionStart(std::stoull(value, nullptr, 16));
      } else if (key == "text_section_start") {
        setTextSectionStart(std::stoull(value, nullptr, 16));
      } else if (key == "bss_section_start") {
        setBssSectionStart(std::stoull(value, nullptr, 16));
      }
      
      
      
      else {
        throw std::invalid_argument("Unknown key: " + key);
      }
    } 
    
    
    
    else {
      throw std::invalid_argument("Unknown section: " + section);
    }
  }


};

extern VmConfig config;


} // namespace vm_config


#endif // CONFIG_H
