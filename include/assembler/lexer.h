/**
 * @file lexer.h
 * @brief Contains the definition of the Lexer class.
 * @author Vishank Singh, https://github.com/VishankSingh
 */

#ifndef LEXER_H
#define LEXER_H

#include "tokens.h"
#include <vector>
#include <fstream>

/**
 * @class Lexer
 * @brief A class responsible for tokenizing the input source code.
 * 
 * This class reads an input file, processes its contents, and generates a sequence of tokens.
 * It handles various types of tokens such as identifiers, numbers, directives, and string literals.
 */
class Lexer {
 private:
  std::string filename_; ///< The name of the input file.
  std::ifstream input_; ///< Input stream for reading the source code.
  std::string current_line_; ///< The current line being processed.
  unsigned int line_number_; ///< The current line number in the source code.
  unsigned int column_number_; ///< The current column number in the source code.
  size_t pos_; ///< The current position within the current line.

  std::vector<Token> tokens_; ///< A list of tokens generated during the lexing process.

  /**
   * @brief Skips whitespace characters (spaces, tabs, etc.) in the input.
   *
   * This function moves the lexer position past any whitespace characters
   * in the current line.
   */
  void skipWhitespace();

  /**
   * @brief Skips comments in the input.
   *
   * This function handles single-line and multi-line comments, moving the lexer
   * position past the entire comment block.
   */
  void skipComment();

  /**
   * @brief Skips the remainder of the current line.
   *
   * This function moves the lexer position past all characters on the current line
   * after the current position, effectively skipping the entire rest of the line.
   */
  void skipLine();

  /**
   * @brief Tokenizes an identifier.
   *
   * This function handles identifiers (e.g., variable names, function names) in the source code.
   *
   * @return A Token object representing the identifier.
   */
  Token identifier();

  /**
   * @brief Tokenizes a number.
   *
   * This function handles numeric literals in the source code.
   *
   * @return A Token object representing the number.
   */
  Token number();

  /**
   * @brief Tokenizes a directive.
   *
   * This function handles directives (e.g., preprocessor directives) in the source code.
   *
   * @return A Token object representing the directive.
   */
  Token directive();

  /**
   * @brief Tokenizes a string literal.
   *
   * This function handles string literals in the source code.
   *
   * @return A Token object representing the string literal.
   */
  Token stringLiteral();

  /**
   * @brief Retrieves the next token from the input stream.
   *
   * This function processes the input stream and returns the next valid token.
   *
   * @return The next Token object from the input.
   */

  Token getNextToken();
 public:
  /**
   * @brief Constructs a Lexer object for a given file.
   *
   * This constructor initializes the lexer with the input file, preparing it to process the code.
   *
   * @param filename The name of the source code file to be tokenized.
   */
  explicit Lexer(std::string filename);

  /**
   * @brief Destructor that closes the input file stream.
   */
  ~Lexer();

  /**
   * @brief Retrieves the name of the input file.
   *
   * This function returns the name of the source code file being processed.
   *
   * @return The name of the input file.
   */
  std::string getFilename() const;

  /**
   * @brief Retrieves the complete list of tokens.
   *
   * This function returns a vector of all tokens generated during the lexing process.
   *
   * @return A vector containing all the tokens.
   */
  std::vector<Token> getTokenList();

};

#endif // LEXER_H
