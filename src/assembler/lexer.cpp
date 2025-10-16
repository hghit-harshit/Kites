/**
 * @file lexer.cpp
 * @brief Contains the implementation of the Lexer class for tokenizing source code.
 * @author Vishank Singh, https://github.com/VishankSingh
 */


#include "assembler/lexer.h"

#include "common/instructions.h"
#include "vm/registers.h"
#include"common/rounding_modes.h"

#include <utility>
#include <string>
#include <stdexcept>
#include <regex>
#include <iostream>
#include <fstream>

Lexer::Lexer(std::string filename) : filename_(std::move(filename)), line_number_(0), column_number_(0), pos_(0) {
  input_.open(filename_);
  if (!input_) {
    throw std::runtime_error("Failed to open file: " + filename_);
  }
}

std::string Lexer::getFilename() const {
  return filename_;
}

Lexer::~Lexer() {
  if (input_.is_open()) {
    input_.close();
  }
}

void Lexer::skipWhitespace() {
  while (pos_ < current_line_.size() && std::isspace(current_line_[pos_])) {
    if (current_line_[pos_]=='\n') {
      ++line_number_;
      column_number_ = 1;
    } else {
      ++column_number_;
    }
    ++pos_;
  }
}

void Lexer::skipComment() {
  while (pos_ < current_line_.size() && current_line_[pos_]!='\n') {
    ++pos_;
    ++column_number_;
  }
  if (pos_ < current_line_.size() && current_line_[pos_]=='\n') {
    ++line_number_;
    column_number_ = 1;
  }
}

void Lexer::skipLine() {
  while (pos_ < current_line_.size() && current_line_[pos_]!='\n') {
    ++pos_;
    ++column_number_;
  }
  if (pos_ < current_line_.size() && current_line_[pos_]=='\n') {
    ++line_number_;
    column_number_ = 1;
  }
}

// TODO: make this better
Token Lexer::identifier() {
  size_t start_pos = pos_;
  unsigned int start_column = column_number_;
  while (pos_ < current_line_.size() &&
      (std::isalnum(current_line_[pos_])
          || current_line_[pos_]=='_'
          || current_line_[pos_]=='.'
          // || current_line_[pos_] == ':'

      )) {
    ++pos_;
    ++column_number_;
  }
  std::string value = current_line_.substr(start_pos, pos_ - start_pos);

  std::regex label_regex("^[a-zA-Z][a-zA-Z0-9_]*:$");

  if (pos_ < current_line_.size() && current_line_[pos_]==':') {
    if (value.find('.')!=std::string::npos) {
      ++pos_;
      ++column_number_;
      return {TokenType::INVALID, value, line_number_, start_column};
    }
    ++pos_;
    ++column_number_;
    return {TokenType::LABEL, value, line_number_, start_column};
  }

  if (instruction_set::isValidInstruction(value)) {
    return {TokenType::OPCODE, value, line_number_, start_column};
  }
  if (IsValidGeneralPurposeRegister(value)) {
    return {TokenType::GP_REGISTER, value, line_number_, start_column};
  }
  if (IsValidFloatingPointRegister(value)) {
    return {TokenType::FP_REGISTER, value, line_number_, start_column};
  }
  if (IsValidCsr(value)) {
    return {TokenType::CSR_REGISTER, value, line_number_, start_column};
  }

  if (isValidRoundingMode(value)) {
    return {TokenType::RM, value, line_number_, start_column};
  }

  if (pos_ < current_line_.size() && current_line_[pos_]==':') {
    return {TokenType::LABEL, value, line_number_, start_column};
  }
  if (!tokens_.empty() && tokens_.back().type==TokenType::COMMA) {
    return {TokenType::LABEL_REF, value, line_number_, start_column};
  }



  // Default case: invalid token
  return {TokenType::INVALID, value, line_number_, start_column};
}

Token Lexer::number() {
  unsigned int start_pos = pos_;
  unsigned int start_column = column_number_;

  while (pos_ < current_line_.size()
      && (std::isdigit(current_line_[pos_])
          || current_line_[pos_]=='-'
          || current_line_[pos_]=='x'
          || current_line_[pos_]=='X'
          || current_line_[pos_]=='o'
          || current_line_[pos_]=='O'
          || current_line_[pos_]=='b'
          || current_line_[pos_]=='B'
          || current_line_[pos_]=='.'
          || current_line_[pos_]=='e'
          || current_line_[pos_]=='E'
          || current_line_[pos_]=='+')) {
    ++pos_;
    ++column_number_;
  }

  std::string value = current_line_.substr(start_pos, pos_ - start_pos);

  std::regex hex_regex("^-?0[xX][0-9a-fA-F]+$");
  std::regex binary_regex("^-?0[bB][01]+$");
  std::regex octal_regex("^-?0[oO][0-7]+$");
  std::regex decimal_regex("^-?[0-9]+$");
  std::regex float_regex("^-?[0-9]*\\.[0-9]+([eE][-+]?[0-9]+)?$|^-?[0-9]+[eE][-+]?[0-9]+$");

  if (std::regex_match(value, hex_regex)) {
    bool is_negative = value[0]=='-';
    value = (is_negative ? "-" : "") + value.substr(is_negative ? 3 : 2);
    return {TokenType::NUM, std::to_string(std::stoll(value, nullptr, 16)), line_number_, start_column};
  } else if (std::regex_match(value, binary_regex)) {
    bool is_negative = value[0]=='-';
    value = (is_negative ? "-" : "") + value.substr(is_negative ? 3 : 2);
    return {TokenType::NUM, std::to_string(std::stoll(value, nullptr, 2)), line_number_, start_column};
  } else if (std::regex_match(value, octal_regex)) {
    bool is_negative = value[0]=='-';
    value = (is_negative ? "-" : "") + value.substr(is_negative ? 3 : 2);
    return {TokenType::NUM, std::to_string(std::stoll(value, nullptr, 8)), line_number_, start_column};
  } else if (std::regex_match(value, decimal_regex)) {
    return {TokenType::NUM, std::to_string(std::stoll(value, nullptr, 10)), line_number_, start_column};
  } else if (std::regex_match(value, float_regex)) {

    return {TokenType::FLOAT, std::to_string(std::stod(value)), line_number_, start_column};
  }

  return {TokenType::INVALID, "Invalid", line_number_, start_column};

  /*
      The below code is for character by character parsing of numbers.
      The code is given for reference and is not used in the current implementation.
      The regex based implementation, although slower, is more robust and easier to understand.
      It is recommended to use character by character parsing in real world applications for better performance.
  */
  /*
  {
      size_t start_pos = pos_;
      bool is_negative = false;

      // Check for a leading minus sign
      if (current_line_[pos_] == '-') {
          is_negative = true;
          ++start_pos;
          ++pos_;
          ++column_number_;

          // Ensure there's a digit or a valid number after the minus sign
          if (pos_ >= current_line_.size() ||
              (!std::isdigit(current_line_[pos_]) && current_line_[pos_] != '0')) {
              return Token(TokenType::INVALID, "-", line_number_, start_pos);
          }
      }

      // Check for special number formats (hex, octal, binary)
      if (current_line_[pos_] == '0' && pos_ + 1 < current_line_.size()) {
          char next_char = current_line_[pos_ + 1];
          if (next_char == 'x' || next_char == 'X') {  // Hexadecimal
              pos_ += 2;  // Skip "0x"
              column_number_ += 2;
              while (pos_ < current_line_.size() &&
                     (std::isdigit(current_line_[pos_]) ||
                      (current_line_[pos_] >= 'a' && current_line_[pos_] <= 'f') ||
                      (current_line_[pos_] >= 'A' && current_line_[pos_] <= 'F'))) {
                  ++pos_;
                  ++column_number_;
              }
              std::string value = current_line_.substr(start_pos, pos_ - start_pos);
              if (is_negative) {
                  value = "-" + value;
              }
              return Token(TokenType::HEX_NUM, value, line_number_, start_pos);
          } else if (next_char == 'o' || next_char == 'O') {  // Octal
              pos_ += 2;  // Skip "0o"
              column_number_ += 2;
              while (pos_ < current_line_.size() &&
                     current_line_[pos_] >= '0' && current_line_[pos_] <= '7') {
                  ++pos_;
                  ++column_number_;
              }
              std::string value = current_line_.substr(start_pos, pos_ - start_pos);
              if (is_negative) {
                  value = "-" + value;
              }
              return Token(TokenType::OCTAL_NUM, value, line_number_, start_pos);
          } else if (next_char == 'b' || next_char == 'B') {  // Binary
              pos_ += 2;  // Skip "0b"
              column_number_ += 2;
              while (pos_ < current_line_.size() &&
                     (current_line_[pos_] == '0' || current_line_[pos_] == '1')) {
                  ++pos_;
                  ++column_number_;
              }
              std::string value = current_line_.substr(start_pos, pos_ - start_pos);
              if (is_negative) {
                  value = "-" + value;
              }
              return Token(TokenType::BINARY_NUM, value, line_number_, start_pos);
          }
      }

      // Parse decimal numbers
      while (pos_ < current_line_.size() && std::isdigit(current_line_[pos_])) {
          ++pos_;
          ++column_number_;
      }

      // Construct the value string, including the minus sign if necessary
      std::string value = current_line_.substr(start_pos, pos_ - start_pos);
      if (is_negative) {
          value = "-" + value;
      }

      return Token(TokenType::DECIMAL_NUM, value, line_number_, start_pos);
  }
  */

}

Token Lexer::directive() {
  ++pos_;
  size_t start_pos = pos_;
  unsigned int start_column = column_number_;
  while (pos_ < current_line_.size() && std::isalpha(current_line_[pos_])) {
    ++pos_;
    ++column_number_;
  }
  std::string value = current_line_.substr(start_pos, pos_ - start_pos);
  return {TokenType::DIRECTIVE, value, line_number_, start_column};
}

Token Lexer::stringLiteral() {
  ++pos_;
  ++column_number_;
  size_t start_pos = pos_;
  unsigned int start_column = column_number_;

  while (pos_ < current_line_.size() && current_line_[pos_]!='"') {
    ++pos_;
    ++column_number_;
  }

  if (pos_==current_line_.size() || current_line_[pos_]!='"') {
    std::cerr << "Error: Unterminated string literal at line " << line_number_ << std::endl;
    return {TokenType::INVALID, "", line_number_, start_column};
  }

  std::string value = current_line_.substr(start_pos, pos_ - start_pos);
  ++pos_;
  ++column_number_;
  return {TokenType::STRING, value, line_number_, start_column};
}

Token Lexer::getNextToken() {
  skipWhitespace();

  if (pos_ >= current_line_.size()) {
    return {TokenType::EOF_, "", line_number_, static_cast<unsigned int>(pos_)};
  }

  char current_char = current_line_[pos_];

  if (std::isalpha(current_char) || current_char=='_') {
    return identifier();
  } else if (std::isdigit(current_char) || current_char=='-') {
    return number();
  } else if (current_char==',') {
    ++pos_;
    ++column_number_;
    return {TokenType::COMMA, ",", line_number_, column_number_ - 1};
  } else if (current_char=='"') {
    return stringLiteral();
  } else if (current_char=='(') {
    ++pos_;
    ++column_number_;
    return {TokenType::LPAREN, "(", line_number_, column_number_ - 1};
  } else if (current_char==')') {
    ++pos_;
    ++column_number_;
    return {TokenType::RPAREN, ")", line_number_, column_number_ - 1};
  } else if (current_char=='.') {
    return directive();
  } else if (current_char=='#' || current_char==';') {
    skipComment();
    return getNextToken();
  } else {
    skipLine();
    return {TokenType::INVALID, "", line_number_, column_number_ - 1};
  }

}

std::vector<Token> Lexer::getTokenList() {
  while (std::getline(input_, current_line_)) {
    pos_ = 0;
    column_number_ = 1;
    line_number_++;
    while (pos_ < current_line_.size()) {
      Token token = getNextToken();
      if (token.type==TokenType::INVALID) {
      }
      if (/* token.type != TokenType::INVALID && */ token.type!=TokenType::EOF_) {
        tokens_.push_back(token);
      }
    }
  }

  tokens_.emplace_back(TokenType::EOF_, "", line_number_, column_number_);
  return tokens_;
}
