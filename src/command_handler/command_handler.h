/**
 * File Name: command_handler.h
 * Author: Vishank Singh
 * Github: https://github.com/VishankSingh
 */
#ifndef COMMAND_HANDLER_H
#define COMMAND_HANDLER_H

#include "common/assembled_program.h"
#include "processor/processor_manager.h"

#include <cstdint>
#include <vector>

namespace Kites
{
namespace command_handler
{
enum class CommandType
{
    INVALID,
    MODIFY_CONFIG,
    LOAD,
    RUN,
    STOP,
    DEBUG_RUN,
    STEP,
    UNDO,
    REDO,
    RESET,
    MODIFY_REGISTER,
    DUMP_MEMORY,
    PRINT_MEMORY,
    GET_MEMORY_POINT,
    DUMP_CACHE,
    ADD_BREAKPOINT,
    REMOVE_BREAKPOINT,
    VM_STDIN,
    EXIT
};

enum class CommandArgumentType
{
    NONE,
    FILE,
    ADDRESS,
    REGISTER,
    VALUE
};

struct Command
{
    CommandType type;
    std::vector<std::string> args;

    Command(CommandType type, const std::vector<std::string> &args) : type(type), args(args)
    {
    }
};

/**
 * @brief State a REPL loop keeps across commands (loaded program, breakpoint set), threaded
 * through ExecuteCommand since ProcessorManager itself doesn't track this bookkeeping.
 */
struct ReplState
{
    AssembledProgram currentProgram{};
    std::vector<uint64_t> breakpoints{};
    bool programLoaded{false};
};

Command ParseCommand(const std::string &input);

/**
 * @brief Executes a parsed command against a running VM instance.
 * @throws std::invalid_argument on malformed/missing arguments or an unsupported command.
 * @throws std::exception (propagated from assembling a file) on LOAD failure.
 */
void ExecuteCommand(const Command &command, ProcessorManager &manager, ReplState &state);

} // namespace command_handler
}//namespace Kites
#endif // COMMAND_HANDLER_H
