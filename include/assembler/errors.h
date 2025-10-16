/**
 * @file errors.h
 * @brief Defines error handling structures and error types for syntax and semantic errors in the assembler.
 * @author Vishank Singh, https://github.com/VishankSingh
 */

#ifndef ERRORS_H
#define ERRORS_H

#include <string>
#include <ostream>

namespace errors {

/// ANSI code for red-colored text.
const std::string ANSI_code_red = "\033[31m";

/// ANSI code to Reset text formatting.
const std::string ANSI_code_reset = "\033[0m";

/**
 * @enum ErrorType
 * @brief Enumerates various types of errors that can occur during assembly.
 */
enum class ErrorType {
  UNEXPECTED_TOKEN,      ///< An unexpected token was encountered.
  UNEXPECTED_EOF,        ///< Unexpected end of file.
  UNEXPECTED_OPERAND,    ///< Unexpected operand in the instruction.

  MISSING_OPERAND,       ///< Missing operand in the instruction.
  MISSING_COMMA,         ///< Missing comma in the instruction.
  MISSING_LPAREN,        ///< Missing left parenthesis.
  MISSING_RPAREN,        ///< Missing right parenthesis.

  EXPECTED_REGISTER,     ///< Expected a register but found something else.
  EXPECTED_IMMEDIATE,    ///< Expected an immediate value.
  EXPECTED_LABEL,        ///< Expected a label.

  INVALID_OPERAND,       ///< Operand is invalid.
  INVALID_DIRECTIVE,     ///< Directive is invalid.
  INVALID_INSTRUCTION,   ///< Instruction is invalid.
  INVALID_REGISTER,      ///< Register name is invalid.
  INVALID_IMMEDIATE,     ///< Immediate value is invalid.
  INVALID_LABEL,         ///< Label is invalid.
  INVALID_LABEL_REF,     ///< Label reference is invalid.
  INVALID_TOKEN,         ///< Token is invalid.
  INVALID_SYNTAX,        ///< Syntax is invalid.

  MISALIGNED_IMMEDIATE,  ///< Immediate value is misaligned.
  MISALIGNED_LABEL,      ///< Label is misaligned.
  IMMEDIATE_OUT_OF_RANGE,///< Immediate value is out of range.
  UNKNOWN_ERROR          ///< An unknown error occurred.
};

/**
 * @struct SyntaxError
 * @brief Represents a generic syntax error in the assembler.
 *
 * This structure is used to store details of a syntax error, including the error message,
 * file location, and the line where the error occurred.
 */
struct SyntaxError {
  std::string main_message;   ///< Main error message.
  std::string sub_message;    ///< Sub error message with additional details.
  std::string filename;       ///< Filename where the error occurred.
  unsigned int line_number;   ///< Line number where the error occurred.
  unsigned int column_number; ///< Column number where the error occurred.
  std::string line_text;      ///< Text of the line where the error occurred.

  SyntaxError(std::string main_message, std::string sub_message, std::string filename, unsigned int line_number,
              unsigned int column_number, std::string line_text)
      : main_message(std::move(main_message)), sub_message(std::move(sub_message)),
        filename(std::move(filename)), line_number(line_number), column_number(column_number),
        line_text(std::move(line_text)) {}

  friend std::ostream &operator<<(std::ostream &os, const SyntaxError &error);
};

// Other error structures follow a similar pattern. Brief explanations added for clarity.

/**
 * @struct UnexpectedTokenError
 * @brief Represents an error caused by an unexpected token in the input.
 */
struct UnexpectedTokenError {
  std::string message;        ///< The error message explaining the issue.
  std::string filename;       ///< The name of the file where the error occurred.
  unsigned int line_number;            ///< The line number where the error occurred.
  unsigned int column_number;          ///< The column number where the error occurred.
  std::string line_text;      ///< The text of the line where the error occurred.


  UnexpectedTokenError(std::string message,
                       std::string filename,
                       unsigned int line_number,
                       unsigned int column_number,
                       std::string line_text)
      : message(std::move(message)), filename(std::move(filename)), line_number(line_number),
        column_number(column_number), line_text(std::move(line_text)) {}

  friend std::ostream &operator<<(std::ostream &os, const UnexpectedTokenError &error);
};

/**
 * @struct ImmediateOutOfRangeError
 * @brief Represents an error caused by an immediate value being outside the valid range.
 */
struct ImmediateOutOfRangeError {
  std::string main_message;   ///< Main error message.
  std::string sub_message;    ///< Sub error message with additional details.
  std::string filename;       ///< Filename where the error occurred.
  unsigned int line_number;            ///< Line number where the error occurred.
  unsigned int column_number;          ///< Column number where the error occurred.
  std::string line_text;      ///< Text of the line where the error occurred.

  ImmediateOutOfRangeError(std::string main_message,
                           std::string sub_message,
                           std::string filename,
                           unsigned int line_number,
                           unsigned int column_number,
                           std::string line_text)
      : main_message(std::move(main_message)), sub_message(std::move(sub_message)),
        filename(std::move(filename)), line_number(line_number), column_number(column_number),
        line_text(std::move(line_text)) {}

  friend std::ostream &operator<<(std::ostream &os, const ImmediateOutOfRangeError &error);
};

/**
 * @struct MisalignedImmediateError
 * @brief Represents an error caused by an immediate value being misaligned.
 */
struct MisalignedImmediateError {
  std::string main_message;   ///< Main error message.
  std::string sub_message;    ///< Sub error message with additional details.
  std::string filename;       ///< Filename where the error occurred.
  unsigned int line_number;            ///< Line number where the error occurred.
  unsigned int column_number;          ///< Column number where the error occurred.
  std::string line_text;      ///< Text of the line where the error occurred.

  MisalignedImmediateError(std::string main_message,
                           std::string sub_message,
                           std::string filename,
                           unsigned int line_number,
                           unsigned int column_number,
                           std::string line_text)
      : main_message(std::move(main_message)), sub_message(std::move(sub_message)),
        filename(std::move(filename)), line_number(line_number), column_number(column_number),
        line_text(std::move(line_text)) {}

  friend std::ostream &operator<<(std::ostream &os, const MisalignedImmediateError &error);
};

/**
 * @struct UnexpectedOperandError
 * @brief Represents an error caused by encountering an unexpected operand.
 */
struct UnexpectedOperandError {
  std::string main_message;   ///< Main error message.
  std::string sub_message;    ///< Sub error message with additional details.
  std::string filename;       ///< Filename where the error occurred.
  unsigned int line_number;            ///< Line number where the error occurred.
  unsigned int column_number;          ///< Column number where the error occurred.
  std::string line_text;      ///< Text of the line where the error occurred.

  UnexpectedOperandError(std::string main_message,
                         std::string sub_message,
                         std::string filename,
                         unsigned int line_number,
                         unsigned int column_number,
                         std::string line_text)
      : main_message(std::move(main_message)), sub_message(std::move(sub_message)),
        filename(std::move(filename)), line_number(line_number), column_number(column_number),
        line_text(std::move(line_text)) {}

  friend std::ostream &operator<<(std::ostream &os, const UnexpectedOperandError &error);
};

/**
 * @struct InvalidLabelRefError
 * @brief Represents an error caused by referencing an invalid or undefined label.
 */
struct InvalidLabelRefError {
  std::string main_message;   ///< Main error message.
  std::string sub_message;    ///< Sub error message with additional details.
  std::string filename;       ///< Filename where the error occurred.
  unsigned int line_number;            ///< Line number where the error occurred.
  unsigned int column_number;          ///< Column number where the error occurred.
  std::string line_text;      ///< Text of the line where the error occurred.

  InvalidLabelRefError(std::string main_message,
                       std::string sub_message,
                       std::string filename,
                       unsigned int line_number,
                       unsigned int column_number,
                       std::string line_text)
      : main_message(std::move(main_message)), sub_message(std::move(sub_message)),
        filename(std::move(filename)), line_number(line_number), column_number(column_number),
        line_text(std::move(line_text)) {}

  friend std::ostream &operator<<(std::ostream &os, const InvalidLabelRefError &error);
};

/**
 * @struct LabelRedefinitionError
 * @brief Represents an error caused by redefining a previously defined label.
 */
struct LabelRedefinitionError {
  std::string main_message;   ///< Main error message.
  std::string sub_message;    ///< Sub error message with additional details.
  std::string filename;       ///< Filename where the error occurred.
  unsigned int line_number;            ///< Line number where the error occurred.
  unsigned int column_number;          ///< Column number where the error occurred.
  std::string line_text;      ///< Text of the line where the error occurred.

  LabelRedefinitionError(std::string main_message,
                         std::string sub_message,
                         std::string filename,
                         unsigned int line_number,
                         unsigned int column_number,
                         std::string line_text)
      : main_message(std::move(main_message)), sub_message(std::move(sub_message)),
        filename(std::move(filename)), line_number(line_number), column_number(column_number),
        line_text(std::move(line_text)) {}

  friend std::ostream &operator<<(std::ostream &os, const LabelRedefinitionError &error);
};

struct InvalidRegisterError {
  std::string main_message;   ///< Main error message.
  std::string sub_message;    ///< Sub error message with additional details.
  std::string filename;       ///< Filename where the error occurred.
  unsigned int line_number;            ///< Line number where the error occurred.
  unsigned int column_number;          ///< Column number where the error occurred.
  std::string line_text;      ///< Text of the line where the error occurred.

  InvalidRegisterError(std::string main_message,
                       std::string sub_message,
                       std::string filename,
                       unsigned int line_number,
                       unsigned int column_number,
                       std::string line_text)
      : main_message(std::move(main_message)), sub_message(std::move(sub_message)),
        filename(std::move(filename)), line_number(line_number), column_number(column_number),
        line_text(std::move(line_text)) {}

  friend std::ostream &operator<<(std::ostream &os, const InvalidRegisterError &error);
};

} // namespace errors


#endif // ERRORS_H
