/**
 * File Name: csr_formats.cpp
 * Author: Vishank Singh
 * Github: https://github.com/VishankSingh
 */

#include "assembler/parser.h"
#include "common/instructions.h"
#include "vm/registers.h"
#include "utils.h"


#include <string>

bool Parser::parse_O_GPR_C_CSR_C_GPR() {
  if (peekToken(1).line_number==currentToken().line_number
      && peekToken(1).type==TokenType::GP_REGISTER
      && peekToken(2).line_number==currentToken().line_number
      && peekToken(2).type==TokenType::COMMA
      && peekToken(3).line_number==currentToken().line_number
      && peekToken(3).type==TokenType::CSR_REGISTER
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
    uint32_t csr_value = csr_to_address.at(peekToken(3).value);
    block.setCsr(csr_value);
    reg = reg_alias_to_name.at(peekToken(5).value);
    block.setRs1(reg);

    skipCurrentLine();
    intermediate_code_.emplace_back(block, true);
    instruction_number_line_number_mapping_[instruction_index_] = block.getLineNumber();
    instruction_index_++;
    return true;
  }
  return false;
}

bool Parser::parse_O_GPR_C_CSR_C_I() {
  if (peekToken(1).line_number==currentToken().line_number
      && peekToken(1).type==TokenType::GP_REGISTER
      && peekToken(2).line_number==currentToken().line_number
      && peekToken(2).type==TokenType::COMMA
      && peekToken(3).line_number==currentToken().line_number
      && peekToken(3).type==TokenType::CSR_REGISTER
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

    reg = reg_alias_to_name.at(peekToken(1).value);
    block.setRd(reg);
    uint32_t csr_value = csr_to_address.at(peekToken(3).value);
    block.setCsr(csr_value);
    int64_t imm = std::stoll(peekToken(5).value);
    if (0 <= imm && imm <= 31) {
      block.setImm(std::to_string(imm));
    } else {
      errors_.count++;
      recordError(ParseError(peekToken(5).line_number, "Immediate value out of range"));
      errors_.all_errors.emplace_back(errors::ImmediateOutOfRangeError("Immediate value out of range",
                                                                       "Expected:0 <= imm <= 31",
                                                                       filename_,
                                                                       peekToken(5).line_number,
                                                                       peekToken(5).column_number,
                                                                       GetLineFromFile(filename_,
                                                                                       peekToken(5).line_number)));
      skipCurrentLine();
      return true;
    }

    skipCurrentLine();
    intermediate_code_.emplace_back(block, true);
    instruction_number_line_number_mapping_[instruction_index_] = block.getLineNumber();
    instruction_index_++;
    return true;
  }
  return false;
}
