/**
 * @file code_generator.h
 * @brief Contains the definition of the ICUnit struct and functions for generating machine code.
 * @author Vishank Singh, https://github.com/VishankSingh
 */

#ifndef CODE_GENERATOR_H
#define CODE_GENERATOR_H

#include <array>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>
#include <cstring>
#include <iomanip>

/**
 * @brief Represents a unit of intermediate code used for generating machine code.
 * 
 * This struct stores the details of an intermediate code block, including the line number, 
 * opcode, register destinations, immediate values, and labels. It provides setter and getter 
 * methods to manage the data safely.
 */
struct ICUnit {
  unsigned int line_number; ///< Line number in the source code corresponding to this block.
  unsigned int instruction_index; ///< Index of the instruction in the intermediate code.
  std::array<char, 21> opcode;  ///< Opcode string (up to 20 characters, null-terminated).
  std::array<char, 6> rd;      ///< Destination register name (up to 20 characters, null-terminated). Eg: "x1", "f4", etc.
  std::array<char, 6> rs1;     ///< Source register 1 name (up to 20 characters, null-terminated).
  std::array<char, 6> rs2;     ///< Source register 2 name (up to 20 characters, null-terminated).
  std::array<char, 6> rs3;     ///< Immediate value (up to 32 characters, null-terminated).
  uint32_t csr;           ///< Control and Status Register (CSR) value.
  std::array<char, 33> imm;     ///< Immediate value (up to 32 characters, null-terminated).
  std::string label;            ///< Label associated with this code block, if any.
  uint8_t rm;       ///< Rounding mode (up to 4 characters, null-terminated).

  ICUnit() : line_number{}, opcode{}, rd{}, rs1{}, rs2{}, rs3{}, csr{}, imm{}, label{}, rm{} {
    opcode.fill('\0');
    rd.fill('\0');
    rs1.fill('\0');
    rs2.fill('\0');
    rs3.fill('\0');
    imm.fill('\0');
  }

  friend std::ostream &operator<<(std::ostream &os, const ICUnit &unit) {
    auto emit_if_filled = [&os](const std::array<char, 6> &field,
                                bool &first_operand) {
      if (field[0] != '\0') {                  
        os << (first_operand ? " " : ", ")      
          << field.data();
        first_operand = false;
      }
    };

    // 1. line number + opcode
    os 
    // << std::hex 
    //    << unit.instruction_index * 4
    //    << std::setw(0) << std::dec 
    //    << ": " 
       << unit.opcode.data();

    // 2. operands
    bool first = true;
    emit_if_filled(unit.rd,  first);
    emit_if_filled(unit.rs1, first);
    emit_if_filled(unit.rs2, first);
    emit_if_filled(unit.rs3, first);

    // 3. immediates (33‑byte array so treat separately)
    if (unit.imm[0] != '\0') {
      os << (first ? " " : ", ") << unit.imm.data();
      first = false;
    }

    // 4. CSR (print only if non‑zero)
    if (unit.csr != 0) {
      std::ios_base::fmtflags f(os.flags());           // save stream flags
      os << " csr=0x" << std::hex << unit.csr;         // hex looks nicer
      os.flags(f);                                     // restore flags
    }

    // 5. rounding mode (print only if non‑zero)
    if (unit.rm != 0) {
      os << " rm=" << static_cast<int>(unit.rm);
    }

    // 6. label (if any) — put at the end in angle brackets
    if (!unit.label.empty()) {
      os << " <" << unit.label << '>';
    }

    return os;
  }


  void setLineNumber(unsigned int value) {
    line_number = value;
  }

  void setInstructionIndex(unsigned int value) {
    instruction_index = value;
  }

  void setOpcode(const std::string &value) {
    strncpy(opcode.data(), value.c_str(), 20);
    opcode[20] = '\0';
  }

  void setRd(const std::string &value) {
    strncpy(rd.data(), value.c_str(), 5);
    rd[5] = '\0';
  }

  void setRs1(const std::string &value) {
    strncpy(rs1.data(), value.c_str(), 5);
    rs1[5] = '\0';
  }

  void setRs2(const std::string &value) {
    strncpy(rs2.data(), value.c_str(), 5);
    rs2[5] = '\0';
  }

  void setRs3(const std::string &value) {
    strncpy(rs3.data(), value.c_str(), 5);
    rs3[5] = '\0';
  }

  void setCsr(uint32_t value) {
    csr = value;
  }

  void setImm(const std::string &value) {
    strncpy(imm.data(), value.c_str(), 32);
    imm[32] = '\0';
  }

  void setLabel(const std::string &value) {
    label = value;
  }

  void setRm(uint8_t value) {
    rm = value;
  }

  [[nodiscard]] unsigned int getLineNumber() const {
    return line_number;
  }

  [[nodiscard]] unsigned int getInstructionIndex() const {
    return instruction_index;
  }

  [[nodiscard]] std::string getOpcode() const {
    return opcode.data();
  }

  [[nodiscard]] std::string getRd() const {
    return rd.data();
  }

  [[nodiscard]] std::string getRs1() const {
    return rs1.data();
  }

  [[nodiscard]] std::string getRs2() const {
    return rs2.data();
  }

  [[nodiscard]] std::string getRs3() const {
    return rs3.data();
  }

  [[nodiscard]] uint32_t getCsr() const {
    return csr;
  }

  [[nodiscard]] std::string getImm() const {
    return imm.data();
  }

  [[nodiscard]] std::string getLabel() const {
    return label;
  }

  [[nodiscard]] uint8_t getRm() const {
    return rm;
  }
};

// TODO: use uint32_t instead of std::bitset<32>

/**
 * @brief Prints the intermediate code to a vector of strings.
 * 
 * @param IntermediateCode A vector of pairs containing ICUnit and a boolean flag.
 * @return A vector of strings representing the intermediate code.
 */
std::vector<std::string> printIntermediateCode(const std::vector<std::pair<ICUnit, bool>> &IntermediateCode);

/**
 * @brief Generates machine code for an R-type instruction.
 * 
 * @param block The ICUnit representing the instruction.
 * @return The machine code bitset<32>.
 */
uint32_t generateRTypeMachineCode(const ICUnit &block);

/**
 * @brief Generates machine code for an I1-type instruction.
 * 
 * @param block The ICUnit representing the instruction.
 * @return The machine code bitset<32>.
 */
uint32_t generateI1TypeMachineCode(const ICUnit &block);

/**
 * @brief Generates machine code for an I2-type instruction.
 * 
 * @param block The ICUnit representing the instruction.
 * @return The machine code bitset<32>.
 */
uint32_t generateI2TypeMachineCode(const ICUnit &block);

/**
 * @brief Generates machine code for an I3-type instruction.
 * 
 * @param block The ICUnit representing the instruction.
 * @return The machine code bitset<32>.
 */
uint32_t generateI3TypeMachineCode(const ICUnit &block);

/**
 * @brief Generates machine code for an S-type instruction.
 * 
 * @param block The ICUnit representing the instruction.
 * @return The machine code bitset<32>.
 */
uint32_t generateSTypeMachineCode(const ICUnit &block);

/**
 * @brief Generates machine code for a B-type instruction.
 * 
 * @param block The ICUnit representing the instruction.
 * @return The machine code bitset<32>.
 */
uint32_t generateBTypeMachineCode(const ICUnit &block);

/**
 * @brief Generates machine code for a U-type instruction.
 * 
 * @param block The ICUnit representing the instruction.
 * @return The machine code bitset<32>.
 */
uint32_t generateUTypeMachineCode(const ICUnit &block);

/**
 * @brief Generates machine code for a J-type instruction.
 * 
 * @param block The ICUnit representing the instruction.
 * @return The machine code bitset<32>.
 */
uint32_t generateJTypeMachineCode(const ICUnit &block);

uint32_t generateCSRRTypeMachineCode(const ICUnit &block);
uint32_t generateCSRITypeMachineCode(const ICUnit &block);

uint32_t generateFDRTypeMachineCode(const ICUnit &block);
uint32_t generateFDR1TypeMachineCode(const ICUnit &block);
uint32_t generateFDR2TypeMachineCode(const ICUnit &block);
uint32_t generateFDR3TypeMachineCode(const ICUnit &block);
uint32_t generateFDR4TypeMachineCode(const ICUnit &block);
uint32_t generateFDITypeMachineCode(const ICUnit &block);
uint32_t generateFDSTypeMachineCode(const ICUnit &block);

/**
 * @brief Generates machine code from a vector of intermediate code blocks.
 * 
 * @param IntermediateCode A vector of pairs containing ICUnit and a boolean flag.
 * @return A vector of bitset<32> representing the machine code.
 */
std::vector<uint32_t> generateMachineCode(const std::vector<std::pair<ICUnit, bool>> &IntermediateCode);

#endif // CODE_GENERATOR_H
