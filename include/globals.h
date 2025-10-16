/**
 * @file globals.h
 * @brief Contains global definitions and includes for the assembler.
 * @author Vishank Singh, https://github.com/VishankSingh
 */
#ifndef GLOBALS_H
#define GLOBALS_H

#include <filesystem>

namespace globals {
extern std::filesystem::path invokation_path;
extern std::filesystem::path vm_state_directory;
extern std::filesystem::path config_file_path;
extern std::filesystem::path disassembly_file_path;
extern std::filesystem::path errors_dump_file_path;
extern std::filesystem::path registers_dump_file_path;
extern std::filesystem::path memory_dump_file_path;
extern std::filesystem::path cache_dump_file_path;
extern std::filesystem::path vm_state_dump_file_path;
//extern std::string output_file;

extern bool verbose_errors_print;
extern bool verbose_warnings;
extern bool vm_as_backend;

extern unsigned int text_section_start;
extern unsigned int data_section_start;

void initGlobals();
}

#endif // GLOBALS_H