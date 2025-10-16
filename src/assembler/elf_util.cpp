
#include "assembler/elf_util.h"

#include "vm_asm_mw.h"

#include <iostream>
#include <fstream>
#include <variant>
#include <cstdint>

void generateElfFile(const AssembledProgram &program, const std::string &output_filename) {
  std::ofstream elfFile(output_filename, std::ios::binary);

  if (!elfFile) {
    std::cerr << "Failed to open ELF file for writing!" << std::endl;
    return;
  }

  // ELF Header
  ElfHeader elfHeader;
  elfHeader.e_shoff = sizeof(ElfHeader);  // Section headers start after ELF header
  elfHeader.e_shnum = 4;  // Now we have 4 sections: NULL, .text, .data, .shstrtab
  elfHeader.e_shstrndx = 3;  // Index of .shstrtab

  // Section Header String Table (stores section names)
  std::string shstrtab = "\0.text\0.data\0.shstrtab\0";
  uint32_t shstrtab_offset = sizeof(ElfHeader) + 3*sizeof(ElfSectionHeader)
      + program.text_buffer.size()*sizeof(uint32_t)
      + program.data_buffer.size();
  uint32_t text_offset = sizeof(ElfHeader) + 3*sizeof(ElfSectionHeader);
  uint32_t data_offset = text_offset + program.text_buffer.size()*sizeof(uint32_t);

  // Section Headers
  ElfSectionHeader nullSection = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0}; // Empty first entry
  ElfSectionHeader textSection = {1, 1, 6, 0x1000, text_offset,
                                  static_cast<uint32_t>(program.text_buffer.size()*sizeof(uint32_t)), 0, 0, 4, 0};
  ElfSectionHeader dataSection = {7, 1, 3, 0x2000, data_offset,
                                  static_cast<uint32_t>(program.data_buffer.size()), 0, 0, 4, 0};
  ElfSectionHeader shstrtabSection = {13, 3, 0, 0, shstrtab_offset,
                                      static_cast<uint32_t>(shstrtab.size()), 0, 0, 1, 0};

  // Write ELF Header
  elfFile.write(reinterpret_cast<const char *>(&elfHeader), sizeof(elfHeader));

  // Write Section Headers
  elfFile.write(reinterpret_cast<const char *>(&nullSection), sizeof(nullSection));
  elfFile.write(reinterpret_cast<const char *>(&textSection), sizeof(textSection));
  elfFile.write(reinterpret_cast<const char *>(&dataSection), sizeof(dataSection));
  elfFile.write(reinterpret_cast<const char *>(&shstrtabSection), sizeof(shstrtabSection));

  // Write `.text` section (machine code)
  for (const auto &instruction : program.text_buffer) {
    elfFile.write(reinterpret_cast<const char *>(&instruction), sizeof(instruction));
  }

  // Write `.data` section (raw binary data)
  for (const auto &data : program.data_buffer) {
    if (std::holds_alternative<uint8_t>(data)) {
      uint8_t value = std::get<uint8_t>(data);
      elfFile.write(reinterpret_cast<const char *>(&value), sizeof(value));
    } else if (std::holds_alternative<uint16_t>(data)) {
      uint16_t value = std::get<uint16_t>(data);
      elfFile.write(reinterpret_cast<const char *>(&value), sizeof(value));
    } else if (std::holds_alternative<uint32_t>(data)) {
      uint32_t value = std::get<uint32_t>(data);
      elfFile.write(reinterpret_cast<const char *>(&value), sizeof(value));
    } else if (std::holds_alternative<uint64_t>(data)) {
      uint64_t value = std::get<uint64_t>(data);
      elfFile.write(reinterpret_cast<const char *>(&value), sizeof(value));
    }
  }

  // Write `.shstrtab` section
  elfFile.write(shstrtab.c_str(), shstrtab.size());

  std::cout << "ELF file generated: " << output_filename << std::endl;
}
