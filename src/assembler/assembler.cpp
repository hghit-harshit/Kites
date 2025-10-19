/** @cond DOXYGEN_IGNORE */
/**
 * File Name: assembler.cpp
 * Author: Vishank Singh
 * Github: https://github.com/VishankSingh
 */
/** @endcond */

#include "assembler/assembler.h"
#include "utils.h"
#include "globals.h"

#include <string>
#include <memory>
#include <stdexcept>
#include <vector>
#include <map>
#include <iostream>
#include <algorithm>

AssembledProgram assemble(const std::string &filename)
{
  std::unique_ptr<Lexer> lexer;
  try {
    lexer = std::make_unique<Lexer>(filename);
  } catch (const std::runtime_error &e) {
    throw std::runtime_error("Failed to open file: " + filename);
  }

  std::vector<Token> tokens = lexer->getTokenList();
  // int previous_line = -1;
  // for (const Token& token : tokens) {
  //     if (token.line_number != previous_line) {
  //         if (previous_line != -1) {
  //             std::cout << std::endl;
  //         }
  //         previous_line = token.line_number;
  //     }
  //     std::cout << token << std::endl;
  // }

  Parser parser(lexer->getFilename(), tokens);
  parser.parse();

  AssembledProgram program;
  program.filename = filename;

  if (parser.getErrorCount()==0) {

    std::vector<uint32_t> machine_code_bits = generateMachineCode(parser.getIntermediateCode());

    program.data_buffer = parser.getDataBuffer();
    program.intermediate_code = parser.getIntermediateCode();
    program.text_buffer = machine_code_bits;
    program.instruction_number_line_number_mapping = parser.getInstructionNumberLineNumberMapping();

    program.line_number_instruction_number_mapping = [&]() {
      std::map<unsigned int, unsigned int> line_number_instruction_number_mapping;
      if (program.instruction_number_line_number_mapping.empty()) {
        return line_number_instruction_number_mapping;
      }
      unsigned int prev_instruction = 0;
      unsigned int prev_line = 1;

      for (const auto &[instruction, line] : program.instruction_number_line_number_mapping) {
        for (unsigned int i = prev_line; i <= line; ++i) {
          line_number_instruction_number_mapping[i] = prev_instruction;
        }
        prev_instruction += 1;
        prev_line = line + 1;
      }
      return line_number_instruction_number_mapping;
    }();

    program.symbol_table = parser.getSymbolTable();

    
    DumpDisasssembly(globals::disassembly_file_path, program);

    DumpNoErrors(globals::errors_dump_file_path);

  } else {
    DumpErrors(globals::errors_dump_file_path, parser.getErrors());
    if (globals::verbose_errors_print) {
      parser.printErrors();
    }
    throw std::runtime_error("Failed to parse file: " + filename);
  }

  return program;
}





