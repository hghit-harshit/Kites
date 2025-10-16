/**
 * @file vm_asm_mw.h
 * @brief Contains functions to handle assembler input for the VM.
 * @author Vishank Singh, https://github.com/VishankSingh
 */

#ifndef VM_ASM_MW_H
#define VM_ASM_MW_H

#include <map>
#include <unordered_map>
#include <string>
#include <vector>
#include <variant>
#include <cstdint>

#include "assembler/parser.h"

struct AssembledProgram {
  std::map<unsigned int, unsigned int> line_number_instruction_number_mapping;
  std::map<unsigned int, unsigned int> instruction_number_line_number_mapping;
  std::map<unsigned int, unsigned int> instruction_number_disassembly_mapping;

  std::vector<std::pair<ICUnit, bool>> intermediate_code;

  // std::vector<std::pair<std::string, SymbolData>> symbol_table;
  

  std::map<std::string, SymbolData> symbol_table;

  std::string filename;
  std::vector<std::variant<uint8_t, uint16_t, uint32_t, uint64_t, std::string, float, double>> data_buffer;
  std::vector<uint32_t> text_buffer;
};

#endif // VM_ASM_MW_H