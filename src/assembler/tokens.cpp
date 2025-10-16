/** @cond DOXYGEN_IGNORE */
/**
 * File Name: tokens.cpp
 * Author: Vishank Singh
 * Github: https://github.com/VishankSingh
 */
/** @endcond */

#include "assembler/tokens.h"

#include <iostream>
#include <string>


std::ostream &operator<<(std::ostream &os, const Token &token) {
  os << "Token(Type: " << tokenTypeToString(token.type)
     << ", Value: \"" << token.value
     << "\", Line: " << token.line_number
     << ", Column: " << token.column_number << ")";
  return os;
}

std::string tokenTypeToString(TokenType type) {
  switch (type) {
    case TokenType::INVALID:return "INVALID     ";
    case TokenType::EOF_:return "EOF         ";
    case TokenType::IDENTIFIER:return "IDENTIFIER  ";
    case TokenType::DIRECTIVE:return "DIRECTIVE   ";
    case TokenType::OPCODE:return "OPCODE      ";
    case TokenType::GP_REGISTER:return "GP_REGISTER    ";
    case TokenType::FP_REGISTER:return "FP_REGISTER    ";
    case TokenType::VEC_REGISTER:return "VEC_REGISTER   ";
    case TokenType::CSR_REGISTER:return "CSR_REGISTER   ";
    case TokenType::NUM:return "NUM         ";
    case TokenType::FLOAT:return "FLOAT       ";
    case TokenType::LABEL:return "LABEL       ";
    case TokenType::LABEL_REF:return "LABEL_REF   ";
    case TokenType::COMMA:return "COMMA       ";
    case TokenType::LPAREN:return "LPAREN      ";
    case TokenType::RPAREN:return "RPAREN      ";
    case TokenType::STRING:return "STRING      ";
    case TokenType::RM:return "RM          ";
    default:return "UNKNOWN     ";
  }
}