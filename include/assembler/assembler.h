/** @cond DOXYGEN_IGNORE */
/**
 * File Name: assembler.h
 * Author: Vishank Singh
 * Github: https://github.com/VishankSingh
 */
/** @endcond */

#ifndef ASSEMBLER_H
#define ASSEMBLER_H


#include "assembler/lexer.h"
#include "assembler/parser.h"

#include "code_generator.h"
#include "vm_asm_mw.h"

/**
 * @brief Assembles the intermediate code into machine code.
 * 
 * This function takes a vector of intermediate code blocks and assembles them into machine code.
 * It uses the functions generateRTypeMachineCode, generateITypeMachineCode, generateSTypeMachineCode,
 * generateBTypeMachineCode, generateUTypeMachineCode, and generateJTypeMachineCode to generate the
 * machine code for each block.
 * 
 * @param IntermediateCode A vector of pairs containing ICUnit and a boolean flag.
 * @return A vector of strings representing the machine code.
 */
AssembledProgram assemble(const std::string &filename);

#endif // ASSEMBLER_H
