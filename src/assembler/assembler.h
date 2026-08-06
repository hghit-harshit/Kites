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
#include <istream>

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

/**
 * @brief Assembles source read from an in-memory stream, without touching disk.
 *
 * Used for live diagnostics on editor content that hasn't been saved. Unlike the
 * file-based overload, this never writes the disassembly/error dump files (those
 * paths are shared globals and not safe to write from a background/live-typing
 * call), and never throws on parse errors — callers should inspect the returned
 * program together with a diagnostics query on their own Parser if they need
 * errors (see LanguageService, which drives this directly).
 *
 * @param source The stream to assemble.
 * @param virtualFilename A name to report in diagnostics (not a real file).
 * @param diagnosticsOut Populated with every parse error/diagnostic found.
 * @return The assembled program (only meaningful when diagnosticsOut is empty).
 */
AssembledProgram assemble(std::istream &source, const std::string &virtualFilename,
                          std::vector<DiagnosticInfo> &diagnosticsOut);

}  // namespace Kites

#endif // ASSEMBLER_H
