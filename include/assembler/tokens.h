/**
 * @file tokens.h
 * @brief Contains the definition of the TokenType enum and the Token struct.
 * @author Vishank Singh, https://github.com/VishankSingh
 */

#ifndef TOKENS_H
#define TOKENS_H

#include <string>

/**
 * @brief Enum class representing the type of a token.
 * 
 * This class is used to categorize different token types in a lexer.
 */
enum class TokenType {
  INVALID,         ///< Invalid token
  EOF_,            ///< End of file
  IDENTIFIER,      ///< Variable or function name
  DIRECTIVE,       ///< Assembly directive
  OPCODE,          ///< Opcode in an instruction
  GP_REGISTER,        ///< General-purpose register
  FP_REGISTER,        ///< Floating-point register
  VEC_REGISTER,       ///< Vector register
  CSR_REGISTER,       ///< Control and Status Register
  NUM,             ///< Numeric value
  FLOAT,           ///< Floating-point value
  LABEL,           ///< Label in assembly code
  LABEL_REF,       ///< Reference to a label
  COMMA,           ///< Comma (separator) in assembly
  LPAREN,          ///< Left parenthesis '('
  RPAREN,          ///< Right parenthesis ')'
  STRING,          ///< String literal
  RM,            ///< Rounding mode
};

/**
 * @brief Structure representing a token.
 * 
 * A token consists of a type, its value, and its position in the source code (line and column).
 */
struct Token {
  TokenType type;         ///< Type of the token (e.g., IDENTIFIER, OPCODE)
  std::string value;      ///< The value of the token (e.g., the actual string or number)
  unsigned int line_number; ///< Line number where the token appears
  unsigned int column_number; ///< Column number where the token appears

  /**
   * @brief Constructs a Token object.
   *
   * Initializes a token with a given type, value, and position (line and column).
   *
   * @param type The type of the token (default is INVALID).
   * @param value The value of the token (default is an empty string).
   * @param line The line number of the token (default is 0).
   * @param column The column number of the token (default is 0).
   */
  Token(TokenType type = TokenType::INVALID,
        const std::string &value = "",
        unsigned int line = 0,
        unsigned int column = 0)
      : type(type), value(value), line_number(line), column_number(column) {}

  /**
   * @brief Outputs the token as a string.
   *
   * @param os The output stream to write to.
   * @param token The token to output.
   * @return The output stream with the token information.
   */
  friend std::ostream &operator<<(std::ostream &os, const Token &token);
};

/**
 * @brief Converts a TokenType to its string representation.
 * 
 * This function is used to convert an enum value of TokenType to a human-readable string.
 * 
 * @param type The TokenType to convert.
 * @return A string representing the TokenType.
 */
inline std::string tokenTypeToString(TokenType type);

#endif // TOKENS_H
