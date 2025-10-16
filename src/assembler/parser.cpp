/**
 * @file parser.cpp
 * @brief Contains the implementation of the Parser class for parsing tokens and generating intermediate code.
 * @author Vishank Singh, https://github.com/VishankSingh
 */

#include "assembler/parser.h"

#include "common/instructions.h"
#include "vm/registers.h"
#include "utils.h"

#include <cstdint>
#include <iostream>
#include <vector>

Token Parser::prevToken() {
  if (pos_ > 0) {
    return tokens_[pos_ - 1];
  }
  return {TokenType::EOF_, "", 1, 1};
}

Token Parser::currentToken() {
  if (pos_ < tokens_.size()) {
    return tokens_[pos_];
  }
  return {TokenType::EOF_, "", 1, 1};
}

Token Parser::nextToken() {
  if (pos_ < tokens_.size()) {
    return tokens_[pos_++];
  }
  return {TokenType::EOF_, "", 1, 1};
}

Token Parser::peekToken(int n) {
  if (pos_ + n < tokens_.size()) {
    return tokens_[pos_ + n];
  }
  return {TokenType::EOF_, "", 1, 1};
}

void Parser::skipCurrentLine() {
  unsigned int currLine = currentToken().line_number;
  while (currentToken().line_number==currLine && currentToken().type!=TokenType::EOF_) {
    nextToken();
  }
}

void Parser::recordError(const ParseError &error) {
  errors_.parse_errors.emplace_back(error);
  errors_.count++;
}



//=================================================================================


void Parser::parseDataDirective() {
  auto align = [&](unsigned int alignment) {
    if (data_index_ % alignment != 0)
      data_index_ += alignment - (data_index_ % alignment);
  };
  while (currentToken().value!="text"
      && currentToken().value!="data"
      && currentToken().value!="bss"
      && currentToken().value!="section"
      && currentToken().type!=TokenType::EOF_) {
      
        
    if (currentToken().type==TokenType::LABEL) {
      if (peekToken(1).value=="word") {
        align(4);
      } else if (peekToken(1).value=="dword") {
        align(8);
      } else if (peekToken(1).value=="halfword") {
        align(2);
      } else if (peekToken(1).value=="byte") {
        align(1);
      } else if (peekToken(1).value=="float") {
        align(4);
      } else if (peekToken(1).value=="double") {
        align(8);
      } else if (peekToken(1).value=="string") {
        align(1);
      } else if (peekToken(1).value=="zero") {
        align(1);
      } else {
        errors_.count++;
        recordError(
          ParseError(
            currentToken().line_number, 
            "Invalid directive: Expected .dword, .word, .halfword, .byte, .float, .double, .string, .zero"
          )
        );
        errors_.all_errors.emplace_back(
          errors::SyntaxError(
            "Invalid directive", "Expected .dword, .word, .halfword, .byte, .float, .double, .string, .zero",
            filename_, currentToken().line_number,
            currentToken().column_number,
            GetLineFromFile(filename_, currentToken().line_number)
          )
        );
      }
      symbol_table_[currentToken().value] = {data_index_, currentToken().line_number, true};
      nextToken();
      continue;
    }

    if (currentToken().value=="dword") {
      nextToken();
      while (currentToken().type!=TokenType::EOF_
          && (currentToken().type==TokenType::NUM
              || currentToken().type==TokenType::COMMA)) {
        if (currentToken().type==TokenType::NUM) {
          align(8);
          data_buffer_.emplace_back(static_cast<uint64_t>(std::stoull(currentToken().value)));
          data_index_ += 8;
        }
        nextToken();
      }
    } else if (currentToken().value=="word") {
      nextToken();
      while (currentToken().type!=TokenType::EOF_
          && (currentToken().type==TokenType::NUM
              || currentToken().type==TokenType::COMMA)) {
        if (currentToken().type==TokenType::NUM) {
          align(4);
          data_buffer_.emplace_back(static_cast<uint32_t>(std::stoull(currentToken().value)));
          data_index_ += 4;
        }
        nextToken();
      }

    } else if (currentToken().value=="halfword") {
      nextToken();
      while (currentToken().type!=TokenType::EOF_
          && (currentToken().type==TokenType::NUM
              || currentToken().type==TokenType::COMMA)) {
        if (currentToken().type==TokenType::NUM) {
          align(2);
          data_buffer_.emplace_back(static_cast<uint16_t>(std::stoull(currentToken().value)));
          data_index_ += 2;
        }
        nextToken();
      }
    } else if (currentToken().value=="byte") {
      nextToken();
      while (currentToken().type!=TokenType::EOF_
          && (currentToken().type==TokenType::NUM
              || currentToken().type==TokenType::COMMA)) {
        if (currentToken().type==TokenType::NUM) {
          align(1);
          data_buffer_.emplace_back(static_cast<uint8_t>(std::stoull(currentToken().value)));
          data_index_ += 1;
        }
        nextToken();
      }
    } else if (currentToken().value=="float") {
      nextToken();
      while (currentToken().type!=TokenType::EOF_
          && (currentToken().type==TokenType::FLOAT
              || currentToken().type==TokenType::COMMA)) {
        if (currentToken().type==TokenType::FLOAT) {
          align(4);
          data_buffer_.emplace_back(static_cast<float>(std::stof(currentToken().value)));
          data_index_ += 4;
        }
        nextToken();
      }
    } else if (currentToken().value=="double") {
      nextToken();
      while (currentToken().type!=TokenType::EOF_
          && (currentToken().type==TokenType::FLOAT
              || currentToken().type==TokenType::COMMA)) {
        if (currentToken().type==TokenType::FLOAT) {
          align(8);
          data_buffer_.emplace_back(static_cast<double>(std::stod(currentToken().value)));
          data_index_ += 8;
        }
        nextToken();
      }
    } else if (currentToken().value=="zero") {
      nextToken();
      while (currentToken().type!=TokenType::EOF_
          && (currentToken().type==TokenType::NUM
              || currentToken().type==TokenType::COMMA)) {
        if (currentToken().type==TokenType::NUM) {
          unsigned long long num = std::stoull(currentToken().value);
          if (num > 0) {
            align(1);
            for (unsigned long long i = 0; i < num; ++i) {
              data_buffer_.emplace_back(static_cast<uint8_t>(0));
              data_index_ += 1;
            }
          } else {
            errors_.count++;
            recordError(
              ParseError(
                currentToken().line_number, "Invalid zero directive: Expected a positive number"
              )
            );
            errors_.all_errors.emplace_back(
              errors::SyntaxError(
                "Invalid zero directive", "Expected a positive number",
                filename_, currentToken().line_number,
                currentToken().column_number,
                GetLineFromFile(filename_, currentToken().line_number)
              )
            );
          }
        }
        nextToken();
      }
    } else if (currentToken().value=="string") {
      nextToken();
      while (currentToken().type!=TokenType::EOF_
          && (currentToken().type==TokenType::STRING
              || currentToken().type==TokenType::COMMA)) {

        if (currentToken().type==TokenType::STRING) {
          std::string rawString = currentToken().value;
          std::string processedString = ParseEscapedString(rawString);
          processedString.push_back('\0');
          align(1); 
          data_buffer_.emplace_back(static_cast<std::string>(processedString));
          data_index_ += processedString.size();
        }
        nextToken();
      }
    } else {
      errors_.count++;
      recordError(
        ParseError(
          currentToken().line_number,
          "Invalid directive: Expected .dword, .word, .halfword, .byte, .string, .float, .double, .zero"
        )
      );
      errors_.all_errors.emplace_back(
        errors::SyntaxError(
          "Invalid directive", "Expected .dword, .word, .halfword, .byte, .string, .float, .double, .zero",
          filename_, currentToken().line_number,
          currentToken().column_number,
          GetLineFromFile(filename_, currentToken().line_number)
        )
      );
      nextToken();
    }
  }
}

void Parser::parseTextDirective() {

  while (currentToken().type!=TokenType::DIRECTIVE
      && currentToken().type!=TokenType::EOF_) {

    if (currentToken().type==TokenType::LABEL) {
      if (symbol_table_.find(currentToken().value)!=symbol_table_.end()) {
        errors_.count++;
        recordError(ParseError(currentToken().line_number,
                               "Label redefinition: already defined at line " + std::to_string(
                                   symbol_table_[currentToken().value].line_number)));
        errors_.all_errors.emplace_back(errors::LabelRedefinitionError("Label redefinition",
                                                                       "Label already defined at line " +
                                                                           std::to_string(
                                                                               symbol_table_[currentToken().value].line_number),
                                                                       filename_,
                                                                       currentToken().line_number,
                                                                       currentToken().column_number,
                                                                       GetLineFromFile(filename_,
                                                                                       currentToken().line_number)));
        nextToken();
        continue;
      }
      symbol_table_[currentToken().value] = {instruction_index_*4, currentToken().line_number, false};
      nextToken();
    } else if (currentToken().type==TokenType::OPCODE) {
      std::vector<instruction_set::SyntaxType>
          syntaxes = instruction_set::instruction_syntax_map[currentToken().value];

      bool valid_syntax = false;

      for (instruction_set::SyntaxType &syntax : syntaxes) {
        switch (syntax) {
          case instruction_set::SyntaxType::O_GPR_C_GPR_C_GPR: {
            valid_syntax = parse_O_GPR_C_GPR_C_GPR();
            break;
          }

          case instruction_set::SyntaxType::O_GPR_C_GPR_C_I: {
            valid_syntax = parse_O_GPR_C_GPR_C_I();
            break;
          }

          case instruction_set::SyntaxType::O_GPR_C_GPR_C_IL: {
            valid_syntax = parse_O_GPR_C_GPR_C_IL();
            break;
          }

          case instruction_set::SyntaxType::O_GPR_C_I_LP_GPR_RP: {
            valid_syntax = parse_O_GPR_C_I_LP_GPR_RP();
            break;
          }

          case instruction_set::SyntaxType::O_GPR_C_I: {
            valid_syntax = parse_O_GPR_C_I();
            break;
          }

          case instruction_set::SyntaxType::O_GPR_C_IL: {
            valid_syntax = parse_O_GPR_C_IL();
            break;
          }

          case instruction_set::SyntaxType::O_GPR_C_DL: {
            valid_syntax = parse_O_GPR_C_DL();
            break;
          }

          case instruction_set::SyntaxType::O_GPR_C_GPR_C_DL: {
            break;
          }

          case instruction_set::SyntaxType::O: {
            valid_syntax = parse_O();
            break;
          }

          case instruction_set::SyntaxType::PSEUDO: {
            valid_syntax = parse_pseudo();
            break;
          }

          case instruction_set::SyntaxType::O_GPR_C_CSR_C_GPR: {
            valid_syntax = parse_O_GPR_C_CSR_C_GPR();
            break;
          }

          case instruction_set::SyntaxType::O_GPR_C_CSR_C_I: {
            valid_syntax = parse_O_GPR_C_CSR_C_I();
            break;
          }

          case instruction_set::SyntaxType::O_FPR_C_FPR_C_FPR_C_FPR: {
            valid_syntax = parse_O_FPR_C_FPR_C_FPR_C_FPR();
            break;
          }

          case instruction_set::SyntaxType::O_FPR_C_FPR_C_FPR_C_FPR_C_RM: {
            valid_syntax = parse_O_FPR_C_FPR_C_FPR_C_FPR_C_RM();
            break;
          }

          case instruction_set::SyntaxType::O_FPR_C_FPR_C_FPR: {
            valid_syntax = parse_O_FPR_C_FPR_C_FPR();
            break;
          }

          case instruction_set::SyntaxType::O_FPR_C_FPR_C_FPR_C_RM: {
            valid_syntax = parse_O_FPR_C_FPR_C_FPR_C_RM();
            break;
          }

          case instruction_set::SyntaxType::O_FPR_C_FPR: {
            valid_syntax = parse_O_FPR_C_FPR();
            break;
          }

          case instruction_set::SyntaxType::O_FPR_C_FPR_C_RM: {
            valid_syntax = parse_O_FPR_C_FPR_C_RM();
            break;
          }

          case instruction_set::SyntaxType::O_FPR_C_GPR: {
            valid_syntax = parse_O_FPR_C_GPR();
            break;
          }

          case instruction_set::SyntaxType::O_FPR_C_GPR_C_RM: {
            valid_syntax = parse_O_FPR_C_GPR_C_RM();
            break;
          }

          case instruction_set::SyntaxType::O_GPR_C_FPR: {
            valid_syntax = parse_O_GPR_C_FPR();
            break;
          }

          case instruction_set::SyntaxType::O_GPR_C_FPR_C_RM: {
            valid_syntax = parse_O_GPR_C_FPR_C_RM();
            break;
          }

          case instruction_set::SyntaxType::O_GPR_C_FPR_C_FPR: {
            valid_syntax = parse_O_GPR_C_FPR_C_FPR();
            break;
          }

          case instruction_set::SyntaxType::O_FPR_C_I_LP_GPR_RP: {
            valid_syntax = parse_O_FPR_C_I_LP_GPR_RP();
            break;
          }

          default: {
            break;
          }
        }
        if (valid_syntax) {
          break;
        }
      }
      if (!valid_syntax) {
        errors_.count++;
        recordError(ParseError(currentToken().line_number,
                               "Invalid syntax: Expected: "
                                   + instruction_set::getExpectedSyntaxes(currentToken().value)));
        errors_.all_errors.emplace_back(
            errors::SyntaxError("Syntax error",
                                "Expected: " + instruction_set::getExpectedSyntaxes(currentToken().value),
                                filename_,
                                currentToken().line_number,
                                currentToken().column_number,
                                GetLineFromFile(filename_, currentToken().line_number)));

        skipCurrentLine();
      }

    } else {
      errors_.count++;
      recordError(ParseError(currentToken().line_number, "Unexpected token: " + currentToken().value));
      errors_.all_errors.emplace_back(errors::UnexpectedTokenError("Unexpected token",
                                                                   filename_,
                                                                   currentToken().line_number,
                                                                   currentToken().column_number,
                                                                   GetLineFromFile(filename_,
                                                                                   currentToken().line_number)));

      skipCurrentLine();
      continue;
    }
  }
}

void Parser::parseBSSDirective() {
  while (currentToken().value!="text"
      && currentToken().value!="data"
      && currentToken().value!="bss"
      && currentToken().value!="section"
      && currentToken().type!=TokenType::EOF_) {
      
    nextToken();

    // if (currentToken().type==TokenType::LABEL) {
    //   if (peekToken(1).value=="space") {
    //     nextToken();
    //     nextToken();
    //     if (currentToken().type==TokenType::NUM) {
    //       symbol_table_[currentToken().value] = {data_index_, currentToken().line_number, true};
    //       data_index_ += std::stoull(currentToken().value);
    //       nextToken();
    //     } else {
    //       errors_.count++;
    //       recordError(ParseError(currentToken().line_number,
    //                              "Invalid syntax: Expected a number after .space"));
    //       errors_.all_errors.emplace_back(
    //           errors::SyntaxError("Invalid syntax", "Expected a number after .space",
    //                               filename_, currentToken().line_number,
    //                               currentToken().column_number,
    //                               GetLineFromFile(filename_, currentToken().line_number)));
    //       nextToken();
    //     }
    //   } else {
    //     errors_.count++;
    //     recordError(ParseError(currentToken().line_number,
    //                            "Invalid label: Expected .space"));
    //     errors_.all_errors.emplace_back(
    //         errors::SyntaxError("Invalid label", "Expected: .space",
    //                             filename_, currentToken().line_number,
    //                             currentToken().column_number,
    //                             GetLineFromFile(filename_, currentToken().line_number)));
    //     nextToken();
    //   }
    // } else {
    //   errors_.count++;
    //   recordError(ParseError(currentToken().line_number,
    //                          "Invalid directive: Expected .space"));
    //   errors_.all_errors.emplace_back(
    //       errors::SyntaxError("Invalid directive", "Expected: .space",
    //                           filename_, currentToken().line_number,
    //                           currentToken().column_number,
    //                           GetLineFromFile(filename_, currentToken().line_number)));
    //   nextToken();
    // }
  }
}

// TODO: implement bss directive

void Parser::parse() {
  instruction_index_ = 0;
  data_index_ = 0;

  // first pass: skip sections and directives and collect labels in data section and bss section
  while (currentToken().type!=TokenType::EOF_) {
    if (currentToken().value == "section" && currentToken().type == TokenType::DIRECTIVE) {
      nextToken();
    } else if (currentToken().value=="data" && currentToken().type==TokenType::DIRECTIVE) {
      nextToken();
      parseDataDirective();
    } else if (currentToken().value=="bss" && currentToken().type==TokenType::DIRECTIVE) {
      nextToken();
      parseBSSDirective();
    }
    
    
    
    
    
    else if ((currentToken().value=="text" && currentToken().type==TokenType::DIRECTIVE) 
          || (currentToken().type==TokenType::LABEL || currentToken().type==TokenType::OPCODE)) {
      while (currentToken().type!=TokenType::EOF_ && currentToken().value!="data" && currentToken().value!="section" && currentToken().value!="bss") {
        nextToken();
      }
    }
      
    else {
      errors_.count++;
      recordError(ParseError(currentToken().line_number,
                             "Invalid token: Expected .data, .text, or <opcode> or <label>"));
      errors_.all_errors.emplace_back(
          errors::SyntaxError("Invalid token", "Expected: .data, .text, or <opcode> or <label>",
                              filename_,
                              currentToken().line_number,
                              currentToken().column_number,
                              GetLineFromFile(filename_, currentToken().line_number)));
      nextToken();
    }
  }

  // second pass: parse text section and generate intermediate code
  pos_ = 0; // reset position to start parsing text section
  instruction_index_ = 0; // reset instruction index for text section

  while (currentToken().type!=TokenType::EOF_) {
    if (currentToken().value == "section" && currentToken().type == TokenType::DIRECTIVE) {
      nextToken();
    } else if (currentToken().value=="data" && currentToken().type==TokenType::DIRECTIVE) {
      while (currentToken().type!=TokenType::EOF_ && currentToken().value!="text") {
        nextToken();
      }
    } else if (currentToken().value=="bss" && currentToken().type==TokenType::DIRECTIVE) {
      while (currentToken().type!=TokenType::EOF_ && currentToken().value!="text") {
        nextToken();
      }
    }
    
    else if (currentToken().value=="text" && currentToken().type==TokenType::DIRECTIVE) {
      nextToken();
      parseTextDirective();
    }
    else if (currentToken().type==TokenType::LABEL || currentToken().type==TokenType::OPCODE) {
      parseTextDirective();
    } else {
      errors_.count++;
      recordError(ParseError(currentToken().line_number,
                             "Invalid token: Expected .data, .text, .bss or <opcode> or <label>"));
      errors_.all_errors.emplace_back(
          errors::SyntaxError("Invalid token", "Expected: .data, .text, .bss or <opcode> or <label>",
                              filename_,
                              currentToken().line_number,
                              currentToken().column_number,
                              GetLineFromFile(filename_, currentToken().line_number)));
      nextToken();
    }
  }

  for (unsigned int index : back_patch_) {
    ICUnit block = intermediate_code_[index].first;
    if (symbol_table_.find(block.getLabel())!=symbol_table_.end()) {

      if (instruction_set::isValidBTypeInstruction(block.getOpcode())) {
        if (!symbol_table_[block.getLabel()].isData) {
          uint64_t address = symbol_table_[block.getLabel()].address;
          auto offset = static_cast<int64_t>(address - index*4);
          if (-4096 <= offset && offset <= 4095) {
            block.setImm(std::to_string(offset));
          } else {
            errors_.count++;
            recordError(ParseError(block.getLineNumber(), "Immediate value out of range"));
            errors_.all_errors.emplace_back(errors::ImmediateOutOfRangeError("Immediate value out of range",
                                                                             "Expected: -4096 <= imm <= 4095",
                                                                             filename_,
                                                                             block.getLineNumber(),
                                                                             0,
                                                                             GetLineFromFile(filename_,
                                                                                             block.getLineNumber())));
            continue;
          }
        } else {
          errors_.count++;
          recordError(ParseError(block.getLineNumber(), "Invalid label reference: Label references data"));
          errors_.all_errors.emplace_back(
              errors::InvalidLabelRefError("Invalid label reference", "Label references data",
                                           filename_,
                                           block.getLineNumber(),
                                           0,
                                           GetLineFromFile(filename_, block.getLineNumber())));
        }
      } else if (instruction_set::isValidJTypeInstruction(block.getOpcode())) {
        if (!symbol_table_[block.getLabel()].isData) {
          uint64_t address = symbol_table_[block.getLabel()].address;
          auto offset = static_cast<int64_t>(address - index*4);
          if (-1048576 <= offset && offset <= 1048575) {
            block.setImm(std::to_string(offset));
            // block.setLabel(block.getImm());
          } else {
            errors_.count++;
            recordError(ParseError(block.getLineNumber(), "Immediate value out of range"));
            errors_.all_errors.emplace_back(errors::ImmediateOutOfRangeError("Immediate value out of range",
                                                                             "Expected: -1048576 <= imm <= 1048575",
                                                                             filename_,
                                                                             block.getLineNumber(),
                                                                             0,
                                                                             GetLineFromFile(filename_,
                                                                                             block.getLineNumber())));
            continue;
          }
        } else {
          uint64_t address = symbol_table_[block.getLabel()].address;
          auto offset = static_cast<int64_t>(address - index*4);
          if (-1048576 <= offset && offset <= 1048575) {
            block.setImm(std::to_string(offset));
            // block.setLabel(block.getImm());
          } else {
            errors_.count++;
            recordError(ParseError(block.getLineNumber(), "Immediate value out of range"));
            errors_.all_errors.emplace_back(errors::ImmediateOutOfRangeError("Immediate value out of range",
                                                                             "Expected: -1048576 <= imm <= 1048575",
                                                                             filename_,
                                                                             block.getLineNumber(),
                                                                             0,
                                                                             GetLineFromFile(filename_,
                                                                                             block.getLineNumber())));
            continue;
          }
        }
      } 
      else {
        errors_.count++;
        recordError(ParseError(block.getLineNumber(), "Invalid label reference: Label references data"));
        errors_.all_errors.emplace_back(
            errors::InvalidLabelRefError("Invalid label reference", "Label reference not found", filename_,
                                         block.getLineNumber(), 0,
                                         GetLineFromFile(filename_, block.getLineNumber())));
        continue;
      }
      intermediate_code_[index].first = block;
      intermediate_code_[index].second = true;
    } else {
      errors_.count++;
      recordError(ParseError(block.getLineNumber(), "Invalid label reference: Label reference not found"));
      errors_.all_errors.emplace_back(
          errors::InvalidLabelRefError("Invalid label reference", "Label reference not found", filename_,
                                       block.getLineNumber(), 0,
                                       GetLineFromFile(filename_, block.getLineNumber())));
    }
  }

}

unsigned int Parser::getErrorCount() const {
  return errors_.count;
}

const std::vector<ParseError> &Parser::getErrors() const {
  return errors_.parse_errors;
}

const std::map<std::string, SymbolData> &Parser::getSymbolTable() const {
  return symbol_table_;
}

void Parser::printErrors() const {
  for (const auto &error : errors_.all_errors) {
    std::visit([](auto &&arg) {
      std::cout << arg;
    }, error);
  }
}

void Parser::printSymbolTable() const {
  if (symbol_table_.empty()) {
    std::cout << "Symbol table is empty." << std::endl;
    return;
  }

  for (const auto &pair : symbol_table_) {
    std::cout << pair.first << " -> " << pair.second.address << " " << pair.second.isData << '\n';
  }
}

std::vector<std::variant<uint8_t, uint16_t, uint32_t, uint64_t, std::string, float, double>> &Parser::getDataBuffer() {
  return data_buffer_;
}

void Parser::printDataBuffers() const {
  auto stringToHex = [](const std::string &str) -> std::string {
    std::string hexString;
    hexString.reserve(str.size()*3);
    char buffer[4];
    for (unsigned char ch : str) {
      std::snprintf(buffer, sizeof(buffer), "%02x ", ch);
      hexString.append(buffer);
    }
    return hexString;
  };

  for (const auto &data : data_buffer_) {
    std::cout << std::hex;
    if (std::holds_alternative<uint8_t>(data)) {
      std::cout << std::get<uint8_t>(data);
    } else if (std::holds_alternative<uint16_t>(data)) {
      std::cout << std::get<uint16_t>(data);
    } else if (std::holds_alternative<uint32_t>(data)) {
      std::cout << std::get<uint32_t>(data);
    } else if (std::holds_alternative<uint64_t>(data)) {
      std::cout << std::get<uint64_t>(data);
    } else if (std::holds_alternative<std::string>(data)) {
      std::cout << stringToHex(std::get<std::string>(data));
    }

    std::cout << std::dec;
    std::cout << '\n';
  }

}

void Parser::printIntermediateCode() const {
  if (intermediate_code_.empty()) {
    std::cout << "Intermediate code is empty." << std::endl;
    return;
  }

  for (const auto &pair : intermediate_code_) {
    std::cout << pair.first << " -> " << pair.second << '\n';
  }
}

const std::vector<std::pair<ICUnit, bool>> &Parser::getIntermediateCode() const {
  return intermediate_code_;
}

const std::map<unsigned int, unsigned int> &Parser::getInstructionNumberLineNumberMapping() const {
  return instruction_number_line_number_mapping_;
}


