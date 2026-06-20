/** @cond DOXYGEN_IGNORE */
/**
 * File Name: assembler.h
 * Author: Vishank Singh
 * Github: https://github.com/VishankSingh
 */
/** @endcond */

#ifndef ASSEMBLER_H
#define ASSEMBLER_H

#include "lexer.h"
#include "parser.h"
    
#include "common/assembled_program.h"

namespace Kites
{
/**
 * @brief Assembles the intermediate code into machine code.
 *
 * This function takes a vector of intermediate code blocks and assembles them into machine code.
 * It uses the functions generateRTypeMachineCode, generateITypeMachineCode,
 * generateSTypeMachineCode, generateBTypeMachineCode, generateUTypeMachineCode, and
 * generateJTypeMachineCode to generate the machine code for each block.
 *
 * @param filename The name of the file containing the intermediate code.
 * @return A vector of strings representing the machine code.
 */
AssembledProgram assemble(const std::string &filename);

}  // namespace Kites

#endif // ASSEMBLER_H
