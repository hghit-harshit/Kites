/**
 * File Name: i_m_formats.cpp
 * Author: Vishank Singh
 * Github: https://github.com/VishankSingh
 */

#include "assembler/parser.h"
#include "common/instructions.h"
#include "vm/registers.h"
#include "utils.h"

#include <string>

bool Parser::parse_O() {
  if (peekToken(1).type==TokenType::EOF_ || peekToken(1).line_number!=currentToken().line_number
      ) {
    ICUnit block;
    block.setOpcode(currentToken().value);
    block.setLineNumber(currentToken().line_number);
    block.setInstructionIndex(instruction_index_);
    skipCurrentLine();
    intermediate_code_.emplace_back(block, true);
    instruction_number_line_number_mapping_[instruction_index_] = block.getLineNumber();
    instruction_index_++;
    return true;
  }
  return false;
}

bool Parser::parse_O_GPR_C_GPR_C_GPR() {
  if (peekToken(1).line_number==currentToken().line_number
      && peekToken(1).type==TokenType::GP_REGISTER
      && peekToken(2).line_number==currentToken().line_number
      && peekToken(2).type==TokenType::COMMA
      && peekToken(3).line_number==currentToken().line_number
      && peekToken(3).type==TokenType::GP_REGISTER
      && peekToken(4).line_number==currentToken().line_number
      && peekToken(4).type==TokenType::COMMA
      && peekToken(5).line_number==currentToken().line_number
      && peekToken(5).type==TokenType::GP_REGISTER
      && (peekToken(6).type==TokenType::EOF_ || peekToken(6).line_number!=currentToken().line_number)
      ) {
    ICUnit block;
    block.setOpcode(currentToken().value);
    block.setLineNumber(currentToken().line_number);
    block.setInstructionIndex(instruction_index_);

    std::string reg;
    reg = reg_alias_to_name.at(peekToken(1).value);
    block.setRd(reg);
    reg = reg_alias_to_name.at(peekToken(3).value);
    block.setRs1(reg);
    reg = reg_alias_to_name.at(peekToken(5).value);
    block.setRs2(reg);

    skipCurrentLine();
    intermediate_code_.emplace_back(block, true);
    instruction_number_line_number_mapping_[instruction_index_] = block.getLineNumber();
    instruction_index_++;
    return true;
  }
  return false;
}

bool Parser::parse_O_GPR_C_GPR_C_I() {
  if (peekToken(1).line_number==currentToken().line_number
      && peekToken(1).type==TokenType::GP_REGISTER
      && peekToken(2).line_number==currentToken().line_number
      && peekToken(2).type==TokenType::COMMA
      && peekToken(3).line_number==currentToken().line_number
      && peekToken(3).type==TokenType::GP_REGISTER
      && peekToken(4).line_number==currentToken().line_number
      && peekToken(4).type==TokenType::COMMA
      && peekToken(5).line_number==currentToken().line_number
      && peekToken(5).type==TokenType::NUM
      && (peekToken(6).type==TokenType::EOF_ || peekToken(6).line_number!=currentToken().line_number)
      ) {
    ICUnit block;
    block.setOpcode(currentToken().value);
    block.setLineNumber(currentToken().line_number);
    block.setInstructionIndex(instruction_index_);
    std::string reg;

    if (instruction_set::isValidITypeInstruction(block.getOpcode())) {
      reg = reg_alias_to_name.at(peekToken(1).value);
      block.setRd(reg);
      reg = reg_alias_to_name.at(peekToken(3).value);
      block.setRs1(reg);
      int64_t imm = std::stoll(peekToken(5).value);

      if (instruction_set::isValidI2TypeInstruction(block.getOpcode())) {
        if (0 <= imm && imm <= 31) {
          block.setImm(std::to_string(imm));
        } else {
          errors_.count++;
          recordError(ParseError(peekToken(5).line_number, "Immediate value out of range"));
          errors_.all_errors.emplace_back(
            errors::ImmediateOutOfRangeError(
              "Immediate value out of range",
              "Expected: 0 <= imm <= 31",
              filename_,
              peekToken(5).line_number,
              peekToken(5).column_number,
              GetLineFromFile(filename_, peekToken(5).line_number)
            )
          );
          skipCurrentLine();
          return true;
        }
      } else {
        if (-2048 <= imm && imm <= 2047) {
          block.setImm(std::to_string(imm));
        } else {
          errors_.count++;
          recordError(ParseError(peekToken(5).line_number, "Immediate value out of range"));
          errors_.all_errors.emplace_back(errors::ImmediateOutOfRangeError("Immediate value out of range",
                                                                           "Expected: -2048 <= imm <= 2047",
                                                                           filename_,
                                                                           peekToken(5).line_number,
                                                                           peekToken(5).column_number,
                                                                           GetLineFromFile(filename_,
                                                                                           peekToken(5).line_number)));
          skipCurrentLine();
          return true;
        }
      }

    } else if (instruction_set::isValidBTypeInstruction(block.getOpcode())) {
      reg = reg_alias_to_name.at(peekToken(1).value);
      block.setRs1(reg);
      reg = reg_alias_to_name.at(peekToken(3).value);
      block.setRs2(reg);
      int64_t imm = std::stoll(peekToken(5).value);
      if (-4096 <= imm && imm <= 4095) {
        if (imm%4==0) {
          block.setImm(std::to_string(imm));
        } else {
          errors_.count++;
          recordError(ParseError(peekToken(5).line_number, "Misaligned immediate value"));
          errors_.all_errors.emplace_back(errors::MisalignedImmediateError("Misaligned immediate value",
                                                                           "Expected: imm % 4 == 0",
                                                                           filename_,
                                                                           peekToken(5).line_number,
                                                                           peekToken(5).column_number,
                                                                           GetLineFromFile(filename_,
                                                                                           peekToken(5).line_number)));
          skipCurrentLine();
          return true;
        }
      } else {
        errors_.count++;
        recordError(ParseError(peekToken(5).line_number, "Immediate value out of range"));
        errors_.all_errors.emplace_back(errors::ImmediateOutOfRangeError("Immediate value out of range",
                                                                         "Expected: -4096 <= imm <= 4095",
                                                                         filename_,
                                                                         peekToken(5).line_number,
                                                                         peekToken(5).column_number,
                                                                         GetLineFromFile(filename_,
                                                                                         peekToken(5).line_number)));
        skipCurrentLine();
        return true;
      }
    }
    skipCurrentLine();
    intermediate_code_.emplace_back(block, true);
    instruction_number_line_number_mapping_[instruction_index_] = block.getLineNumber();
    instruction_index_++;
    return true;
  }
  return false;
}

bool Parser::parse_O_GPR_C_I() {
  if (peekToken(1).line_number==currentToken().line_number
      && peekToken(1).type==TokenType::GP_REGISTER
      && peekToken(2).line_number==currentToken().line_number
      && peekToken(2).type==TokenType::COMMA
      && peekToken(3).line_number==currentToken().line_number
      && peekToken(3).type==TokenType::NUM
      && (peekToken(4).type==TokenType::EOF_ || peekToken(4).line_number!=currentToken().line_number)
      ) {
    ICUnit block;
    block.setOpcode(currentToken().value);
    block.setLineNumber(currentToken().line_number);
    block.setInstructionIndex(instruction_index_);
    std::string reg;

    if (instruction_set::isValidUTypeInstruction(block.getOpcode())) {
      reg = reg_alias_to_name.at(peekToken(1).value);
      block.setRd(reg);
      int64_t imm = std::stoll(peekToken(3).value);
      if (0 <= imm && imm <= 1048575) {
        block.setImm(std::to_string(imm));
      } else {
        errors_.count++;
        recordError(ParseError(peekToken(3).line_number, "Immediate value out of range"));
        errors_.all_errors.emplace_back(errors::ImmediateOutOfRangeError("Immediate value out of range",
                                                                         "Expected: 0 <= imm <= 1048575",
                                                                         filename_,
                                                                         peekToken(3).line_number,
                                                                         peekToken(3).column_number,
                                                                         GetLineFromFile(filename_,
                                                                                         peekToken(3).line_number)));
        skipCurrentLine();
        return true;
      }
    } else if (instruction_set::isValidJTypeInstruction(block.getOpcode())) {
      reg = reg_alias_to_name.at(peekToken(1).value);
      block.setRd(reg);
      int64_t imm = std::stoll(peekToken(3).value);
      if (-1048576 <= imm && imm <= 1048575) {
        if (imm%2==0) {
          block.setImm(std::to_string(imm));
        } else {
          errors_.count++;
          recordError(ParseError(peekToken(3).line_number, "Misaligned immediate value"));
          errors_.all_errors.emplace_back(errors::MisalignedImmediateError("Misaligned immediate value",
                                                                           "Expected: imm % 4 == 0",
                                                                           filename_,
                                                                           peekToken(3).line_number,
                                                                           peekToken(3).column_number,
                                                                           GetLineFromFile(filename_,
                                                                                           peekToken(3).line_number)));
          skipCurrentLine();
          return true;
        }
      } else {
        errors_.count++;
        recordError(ParseError(peekToken(3).line_number, "Immediate value out of range"));
        errors_.all_errors.emplace_back(errors::ImmediateOutOfRangeError("Immediate value out of range",
                                                                         "Expected: -1048576 <= imm <= 1048575",
                                                                         filename_,
                                                                         peekToken(3).line_number,
                                                                         peekToken(3).column_number,
                                                                         GetLineFromFile(filename_,
                                                                                         peekToken(3).line_number)));
        skipCurrentLine();
        return true;
      }
    }

    skipCurrentLine();
    intermediate_code_.emplace_back(block, true);
    instruction_number_line_number_mapping_[instruction_index_] = block.getLineNumber();
    instruction_index_++;
    return true;
  }
  return false;
}

bool Parser::parse_O_GPR_C_GPR_C_IL() {
  if (peekToken(1).line_number==currentToken().line_number
      && peekToken(1).type==TokenType::GP_REGISTER
      && peekToken(2).line_number==currentToken().line_number
      && peekToken(2).type==TokenType::COMMA
      && peekToken(3).line_number==currentToken().line_number
      && peekToken(3).type==TokenType::GP_REGISTER
      && peekToken(4).line_number==currentToken().line_number
      && peekToken(4).type==TokenType::COMMA
      && peekToken(5).line_number==currentToken().line_number
      && peekToken(5).type==TokenType::LABEL_REF
      && (peekToken(6).type==TokenType::EOF_ || peekToken(6).line_number!=currentToken().line_number)
      ) {
    ICUnit block;
    block.setOpcode(currentToken().value);
    block.setLineNumber(currentToken().line_number);
    block.setInstructionIndex(instruction_index_);
    std::string reg;

    if (instruction_set::isValidBTypeInstruction(block.getOpcode())) {
      reg = reg_alias_to_name.at(peekToken(1).value);
      block.setRs1(reg);
      reg = reg_alias_to_name.at(peekToken(3).value);
      block.setRs2(reg);
      if (symbol_table_.find(peekToken(5).value)!=symbol_table_.end()
          && !symbol_table_[peekToken(5).value].isData) {
        uint64_t address = symbol_table_[peekToken(5).value].address;
        auto offset = static_cast<int64_t>(address - instruction_index_*4);
        if (-4096 <= offset && offset <= 4095) {
          block.setImm(std::to_string(offset));
          block.setLabel(peekToken(5).value);
        } else {
          errors_.count++;
          recordError(ParseError(peekToken(5).line_number, "Immediate value out of range"));
          errors_.all_errors.emplace_back(errors::ImmediateOutOfRangeError("Immediate value out of range",
                                                                           "Expected: -4096 <= imm <= 4095",
                                                                           filename_,
                                                                           peekToken(5).line_number,
                                                                           peekToken(5).column_number,
                                                                           GetLineFromFile(filename_,
                                                                                           peekToken(5).line_number)));
          skipCurrentLine();
          return true;
        }
      } else {
        back_patch_.push_back(instruction_index_);
        block.setLabel(peekToken(5).value);
        intermediate_code_.emplace_back(block, false);
        instruction_number_line_number_mapping_[instruction_index_] = block.getLineNumber();
        instruction_index_++;
        skipCurrentLine();
        return true;
      }
    }
    skipCurrentLine();
    intermediate_code_.emplace_back(block, true);
    instruction_number_line_number_mapping_[instruction_index_] = block.getLineNumber();
    instruction_index_++;
    return true;
  }
  return false;
}

bool Parser::parse_O_GPR_C_GPR_C_DL() {
  return true;
}

bool Parser::parse_O_GPR_C_IL() {
  if (peekToken(1).line_number==currentToken().line_number
      && peekToken(1).type==TokenType::GP_REGISTER
      && peekToken(2).line_number==currentToken().line_number
      && peekToken(2).type==TokenType::COMMA
      && peekToken(3).line_number==currentToken().line_number
      && peekToken(3).type==TokenType::LABEL_REF
      && (peekToken(4).type==TokenType::EOF_ || peekToken(4).line_number!=currentToken().line_number)
      ) {
    ICUnit block;
    block.setOpcode(currentToken().value);
    block.setLineNumber(currentToken().line_number);
    block.setInstructionIndex(instruction_index_);
    if (instruction_set::isValidJTypeInstruction(block.getOpcode())) {
      std::string reg;
      reg = reg_alias_to_name.at(peekToken(1).value);
      block.setRd(reg);
      if (symbol_table_.find(peekToken(3).value)!=symbol_table_.end()
          && !symbol_table_[peekToken(3).value].isData) {
        uint64_t address = symbol_table_[peekToken(3).value].address;
        auto offset = static_cast<int64_t>(address - instruction_index_*4);
        if (-1048576 <= offset && offset <= 1048575) {
          block.setImm(std::to_string(offset));
          block.setLabel(peekToken(3).value);
        } else {
          errors_.count++;
          recordError(ParseError(peekToken(3).line_number, "Immediate value out of range"));
          errors_.all_errors.emplace_back(errors::ImmediateOutOfRangeError("Immediate value out of range",
                                                                           "Expected: -1048576 <= imm <= 1048575",
                                                                           filename_,
                                                                           peekToken(3).line_number,
                                                                           peekToken(3).column_number,
                                                                           GetLineFromFile(filename_,
                                                                                           peekToken(3).line_number)));
          skipCurrentLine();
          return true;
        }
      } else {
        back_patch_.push_back(instruction_index_);
        block.setLabel(peekToken(3).value);
        intermediate_code_.emplace_back(block, false);
        instruction_number_line_number_mapping_[instruction_index_] = block.getLineNumber();
        instruction_index_++;
        skipCurrentLine();
        return true;
      }
    }
    skipCurrentLine();
    intermediate_code_.emplace_back(block, true);
    instruction_number_line_number_mapping_[instruction_index_] = block.getLineNumber();
    instruction_index_++;
    return true;
  }
  return false;
}

bool Parser::parse_O_GPR_C_DL() {
  if (peekToken(1).line_number == currentToken().line_number &&
      peekToken(1).type == TokenType::GP_REGISTER &&
      peekToken(2).line_number == currentToken().line_number &&
      peekToken(2).type == TokenType::COMMA &&
      peekToken(3).line_number == currentToken().line_number &&
      peekToken(3).type == TokenType::LABEL_REF &&
      (peekToken(4).type == TokenType::EOF_ || peekToken(4).line_number != currentToken().line_number)) {

    std::string reg = reg_alias_to_name.at(peekToken(1).value);
    std::string label = peekToken(3).value;
    std::string opcode = currentToken().value;

    // if (opcode != "ld" && opcode != "lw" && opcode != "lh" && opcode != "lb") {
    //   errors_.count++;
    //   recordError(ParseError(peekToken(3).line_number, "Unsupported opcode for load pseudo-instruction"));
    //   errors_.all_errors.emplace_back(
    //     errors::InvalidOpcodeError(
    //       "Unsupported opcode for pseudo-instruction",
    //       "Expected: ld, lw, lh, or lb",
    //       filename_,
    //       peekToken(3).line_number,
    //       peekToken(3).column_number,
    //       GetLineFromFile(filename_, peekToken(3).line_number)
    //     )
    //   );
    //   skipCurrentLine();
    //   return true;
    // }

    if (symbol_table_.find(label) == symbol_table_.end() || !symbol_table_[label].isData) {
      errors_.count++;
      recordError(ParseError(peekToken(3).line_number, "Invalid label reference"));
      errors_.all_errors.emplace_back(
        errors::InvalidLabelRefError(
          "Invalid label reference",
          "Expected: Label defined in .data section",
          filename_,
          peekToken(3).line_number,
          peekToken(3).column_number,
          GetLineFromFile(filename_, peekToken(3).line_number)
        )
      );
      skipCurrentLine();
      return true;
    }

    uint64_t address = symbol_table_[label].address;
    uint64_t data_section_start = vm_config::config.getDataSectionStart();
    uint64_t symbol_addr = data_section_start + address;
    uint64_t pc = instruction_index_ * 4;

    int64_t offset = static_cast<int64_t>(symbol_addr) - static_cast<int64_t>(pc);
    int32_t hi20 = (offset + 0x800) >> 12;
    int32_t lo12 = offset - (hi20 << 12);

    ICUnit auipc_instr;
    auipc_instr.setOpcode("auipc");
    auipc_instr.setLineNumber(currentToken().line_number);
    auipc_instr.setInstructionIndex(instruction_index_);
    auipc_instr.setRd(reg);
    auipc_instr.setImm(std::to_string(hi20));

    ICUnit load_instr;
    load_instr.setOpcode(opcode);
    load_instr.setLineNumber(currentToken().line_number);
    auipc_instr.setInstructionIndex(instruction_index_+1);
    load_instr.setRd(reg);
    load_instr.setRs1(reg);
    load_instr.setImm(std::to_string(lo12));

    std::cout << "auipc " << reg << ", 0x" << std::hex << hi20 << std::dec << std::endl;

    std::cout << load_instr.getOpcode() << " " << reg << ", " << lo12 << "(" << reg << ")" << std::endl;

    intermediate_code_.emplace_back(auipc_instr, true);
    instruction_number_line_number_mapping_[instruction_index_] = auipc_instr.getLineNumber();
    instruction_index_++;

    intermediate_code_.emplace_back(load_instr, true);
    instruction_number_line_number_mapping_[instruction_index_] = load_instr.getLineNumber();
    instruction_index_++;

    skipCurrentLine();
    return true;
  }

  return false;
}

bool Parser::parse_O_GPR_C_I_LP_GPR_RP() {
  if (peekToken(1).line_number==currentToken().line_number
      && peekToken(1).type==TokenType::GP_REGISTER
      && peekToken(2).line_number==currentToken().line_number
      && peekToken(2).type==TokenType::COMMA
      && peekToken(3).line_number==currentToken().line_number
      && peekToken(3).type==TokenType::NUM
      && peekToken(4).line_number==currentToken().line_number
      && peekToken(4).type==TokenType::LPAREN
      && peekToken(5).line_number==currentToken().line_number
      && peekToken(5).type==TokenType::GP_REGISTER
      && peekToken(6).line_number==currentToken().line_number
      && peekToken(6).type==TokenType::RPAREN
      && (peekToken(7).type==TokenType::EOF_ || peekToken(7).line_number!=currentToken().line_number)
      ) {
    ICUnit block;
    block.setOpcode(currentToken().value);
    block.setLineNumber(currentToken().line_number);
    block.setInstructionIndex(instruction_index_);
    std::string reg;
    if (instruction_set::isValidITypeInstruction(block.getOpcode())) {
      reg = reg_alias_to_name.at(peekToken(1).value);
      block.setRd(reg);
      int64_t imm = std::stoll(peekToken(3).value);
      if (-2048 <= imm && imm <= 2047) {
        block.setImm(std::to_string(imm));
      } else {
        errors_.count++;
        recordError(ParseError(peekToken(3).line_number, "Immediate value out of range"));
        errors_.all_errors.emplace_back(errors::ImmediateOutOfRangeError("Immediate value out of range",
                                                                         "Expected: -2048 <= imm <= 2047",
                                                                         filename_,
                                                                         peekToken(3).line_number,
                                                                         peekToken(3).column_number,
                                                                         GetLineFromFile(filename_,
                                                                                         peekToken(3).line_number)));
        skipCurrentLine();
        return true;
      }
      reg = reg_alias_to_name.at(peekToken(5).value);
      block.setRs1(reg);
    } else if (instruction_set::isValidSTypeInstruction(block.getOpcode())) {
      reg = reg_alias_to_name.at(peekToken(1).value);
      block.setRs2(reg);
      int64_t imm = std::stoll(peekToken(3).value);
      if (-2048 <= imm && imm <= 2047) {
        block.setImm(std::to_string(imm));
      } else {
        errors_.count++;
        recordError(ParseError(peekToken(3).line_number, "Immediate value out of range"));
        errors_.all_errors.emplace_back(errors::ImmediateOutOfRangeError("Immediate value out of range",
                                                                         "Expected: -2048 <= imm <= 2047",
                                                                         filename_,
                                                                         peekToken(3).line_number,
                                                                         peekToken(3).column_number,
                                                                         GetLineFromFile(filename_,
                                                                                         peekToken(3).line_number)));
        skipCurrentLine();
        return true;
      }
      reg = reg_alias_to_name.at(peekToken(5).value);
      block.setRs1(reg);
    }
    skipCurrentLine();
    intermediate_code_.emplace_back(block, true);
    instruction_number_line_number_mapping_[instruction_index_] = block.getLineNumber();
    instruction_index_++;
    return true;
  }
  return false;
}

