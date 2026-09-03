#include "globals.h"
#include <cstdlib>
#include <filesystem>

namespace Kites
{
namespace globals
{
namespace
{
std::filesystem::path getWritableDataDirectory()
{
#ifdef __APPLE__
    if (const char *home = std::getenv("HOME"); home != nullptr && *home != '\0')
    {
        return std::filesystem::path(home) /
               "Library" /
               "Application Support" /
               "Kites";
    }
#endif

    return std::filesystem::current_path();
}
} // namespace

std::filesystem::path invokation_path = getWritableDataDirectory();

std::filesystem::path vm_state_directory = invokation_path / "vm_state";
std::filesystem::path config_file_path =
    (invokation_path / "vm_state" / "config.ini");
std::filesystem::path disassembly_file_path =
    (invokation_path / "vm_state" / "disassembly.txt");
std::filesystem::path temporary_assembly_file_path =
    (invokation_path / "vm_state" / "temp.asm");
std::filesystem::path errors_dump_file_path =
    (invokation_path / "vm_state" / "errors_dump.json");
std::filesystem::path registers_dump_file_path =
    (invokation_path / "vm_state" / "registers_dump.json");
std::filesystem::path memory_dump_file_path =
    (invokation_path / "vm_state" / "memory_dump.json");
std::filesystem::path cache_dump_file_path =
    (invokation_path / "vm_state" / "cache_dump.json");
std::filesystem::path vm_state_dump_file_path =
    (invokation_path / "vm_state" / "vm_state_dump.json");
std::filesystem::path custom_pseudo_instructions_file_path =
    (invokation_path / "vm_state" / "custom_pseudo_instructions.json");

bool verbose_errors_print = false;
bool verbose_warnings = false;
bool vm_as_backend = false;

unsigned int text_section_start = 0x00000000;
unsigned int data_section_start = 0x10000000;
}//namespace globals
}//namespace Kites