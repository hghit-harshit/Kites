/**
 * File Name: command_handler.cpp
 * Author: Vishank Singh
 * Github: https://github.com/VishankSingh
 */
#include "command_handler.h"

#include "assembler/assembler.h"
#include "processor/processor_types.h"

#include <algorithm>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace Kites
{
namespace command_handler
{
Command ParseCommand(const std::string &input)
{
    std::istringstream iss(input);
    std::string command_str;
    iss >> command_str;
    command_handler::CommandType command_type = command_handler::CommandType::INVALID;

    if (command_str == "modify_config" || command_str == "mconfig")
    {
        command_type = command_handler::CommandType::MODIFY_CONFIG;
    }
    else if (command_str == "load" || command_str == "l")
    {
        command_type = command_handler::CommandType::LOAD;
    }
    else if (command_str == "run")
    {
        command_type = command_handler::CommandType::RUN;
    }
    else if (command_str == "stop")
    {
        command_type = command_handler::CommandType::STOP;
    }
    else if (command_str == "run_debug" || command_str == "rd")
    {
        command_type = command_handler::CommandType::DEBUG_RUN;
    }
    else if (command_str == "step" || command_str == "s")
    {
        command_type = command_handler::CommandType::STEP;
    }
    else if (command_str == "undo" || command_str == "u")
    {
        command_type = command_handler::CommandType::UNDO;
    }
    else if (command_str == "redo" || command_str == "r")
    {
        command_type = command_handler::CommandType::REDO;
    }
    else if (command_str == "reset")
    {
        command_type = command_handler::CommandType::RESET;
    }
    else if (command_str == "modify_register" || command_str == "mreg")
    {
        command_type = command_handler::CommandType::MODIFY_REGISTER;
    }
    else if (command_str == "dump_mem" || command_str == "dmem")
    {
        command_type = command_handler::CommandType::DUMP_MEMORY;
    }
    else if (command_str == "print_mem" || command_str == "pmem")
    {
        command_type = command_handler::CommandType::PRINT_MEMORY;
    }
    else if (command_str == "get_mem_point" || command_str == "gmp")
    {
        command_type = command_handler::CommandType::GET_MEMORY_POINT;
    }
    else if (command_str == "dump_cache")
    {
        command_type = command_handler::CommandType::DUMP_CACHE;
    }
    else if (command_str == "add_breakpoint")
    {
        command_type = command_handler::CommandType::ADD_BREAKPOINT;
    }
    else if (command_str == "remove_breakpoint")
    {
        command_type = command_handler::CommandType::REMOVE_BREAKPOINT;
    }
    else if (command_str == "vm_stdin" || command_str == "vmsin")
    {
        command_type = command_handler::CommandType::VM_STDIN;
    }
    else if (command_str == "exit" || command_str == "quit" || command_str == "q")
    {
        command_type = command_handler::CommandType::EXIT;
    }

    std::vector<std::string> args;
    std::string arg;
    bool in_quotes = false;
    std::ostringstream current_arg;

    while (iss)
    {
        char c = iss.get();
        if (!iss)
            break;

        if (c == '"')
        {
            in_quotes = !in_quotes;
            if (!in_quotes)
            {
                args.push_back(current_arg.str());
                current_arg.str("");
                current_arg.clear();
            }
        }
        else if (std::isspace(c) && !in_quotes)
        {
            if (!current_arg.str().empty())
            {
                args.push_back(current_arg.str());
                current_arg.str("");
                current_arg.clear();
            }
        }
        else
        {
            current_arg << c;
        }
    }

    if (!current_arg.str().empty())
    {
        args.push_back(current_arg.str());
    }

    return Command(command_type, args);
}

namespace
{
const std::unordered_map<std::string, ProcessorType> kProcessorTypeNames = {
    {"rvss", ProcessorType::RVSS},
    {"rv5-nh-nf", ProcessorType::RV5Stage_NH_NF},
    {"rv5-h-nf", ProcessorType::RV5Stage_H_NF},
    {"rv5-nh-f", ProcessorType::RV5Stage_NH_F},
    {"rv5-h-f", ProcessorType::RV5Stage_H_F},
};

void ExecuteLoad(const std::vector<std::string> &args, ProcessorManager &manager,
                  ReplState &state)
{
    if (args.empty())
    {
        throw std::invalid_argument("load requires a file path: load <file> [--vm <name>]");
    }

    const std::string &filePath = args[0];
    for (size_t i = 1; i < args.size(); ++i)
    {
        if (args[i] == "--vm")
        {
            if (i + 1 >= args.size())
            {
                throw std::invalid_argument("--vm requires a value");
            }
            const std::string &vmName = args[++i];
            auto it = kProcessorTypeNames.find(vmName);
            if (it == kProcessorTypeNames.end())
            {
                throw std::invalid_argument("unknown VM type '" + vmName + "'");
            }
            manager.changeProcessor(it->second);
        }
        else
        {
            throw std::invalid_argument("unknown load argument '" + args[i] + "'");
        }
    }

    AssembledProgram program = assemble(filePath);
    manager.loadProgram(program);
    state.currentProgram = program;
    state.breakpoints.clear();
    state.programLoaded = true;
}

int ParseStepCount(const std::vector<std::string> &args)
{
    if (args.empty())
    {
        return 1;
    }
    int n = std::stoi(args[0]);
    if (n <= 0)
    {
        throw std::invalid_argument("step count must be positive");
    }
    return n;
}

void ExecuteModifyRegister(const std::vector<std::string> &args, ProcessorManager &manager)
{
    if (args.size() < 2)
    {
        throw std::invalid_argument("modify_register requires <reg> <value>");
    }
    uint64_t value = std::stoull(args[1], nullptr, 0);
    manager.getRegisterFile()->ModifyRegister(args[0], value);
}

void ExecutePrintMemory(const std::vector<std::string> &args, ProcessorManager &manager)
{
    if (args.size() < 2)
    {
        throw std::invalid_argument("print_mem requires <hex_addr> <row_count>");
    }
    uint64_t address = std::stoull(args[0], nullptr, 16);
    unsigned int rows = static_cast<unsigned int>(std::stoul(args[1]));
    manager.getMemoryController()->printMemory(address, rows);
}

void ExecuteAddBreakpoint(const std::vector<std::string> &args, ProcessorManager &manager,
                           ReplState &state)
{
    if (args.empty())
    {
        throw std::invalid_argument("add_breakpoint requires <line>");
    }
    uint64_t line = std::stoull(args[0]);
    if (std::find(state.breakpoints.begin(), state.breakpoints.end(), line) ==
        state.breakpoints.end())
    {
        state.breakpoints.push_back(line);
    }
    manager.setBreakpoints(state.breakpoints);
}

void ExecuteRemoveBreakpoint(const std::vector<std::string> &args, ProcessorManager &manager,
                              ReplState &state)
{
    if (args.empty())
    {
        throw std::invalid_argument("remove_breakpoint requires <line>");
    }
    uint64_t line = std::stoull(args[0]);
    state.breakpoints.erase(
        std::remove(state.breakpoints.begin(), state.breakpoints.end(), line),
        state.breakpoints.end());
    manager.setBreakpoints(state.breakpoints);
}

void ExecuteVmStdin(const std::vector<std::string> &args, ProcessorManager &manager)
{
    if (args.empty())
    {
        throw std::invalid_argument("vm_stdin requires text to feed the running program");
    }
    manager.pushInput(args[0]);
}
} // namespace

void ExecuteCommand(const Command &command, ProcessorManager &manager, ReplState &state)
{
    switch (command.type)
    {
    case CommandType::LOAD:
        ExecuteLoad(command.args, manager, state);
        break;
    case CommandType::RUN:
        manager.run();
        break;
    case CommandType::DEBUG_RUN:
        manager.debugRun();
        break;
    case CommandType::STEP:
    {
        int n = ParseStepCount(command.args);
        for (int i = 0; i < n; ++i)
        {
            manager.step();
        }
        break;
    }
    case CommandType::STOP:
        manager.stop();
        break;
    case CommandType::UNDO:
        manager.undo();
        break;
    case CommandType::REDO:
        manager.redo();
        break;
    case CommandType::RESET:
        manager.reset();
        break;
    case CommandType::MODIFY_REGISTER:
        ExecuteModifyRegister(command.args, manager);
        break;
    case CommandType::DUMP_MEMORY:
        manager.getMemoryController()->dumpMemory(command.args);
        break;
    case CommandType::PRINT_MEMORY:
        ExecutePrintMemory(command.args, manager);
        break;
    case CommandType::ADD_BREAKPOINT:
        ExecuteAddBreakpoint(command.args, manager, state);
        break;
    case CommandType::REMOVE_BREAKPOINT:
        ExecuteRemoveBreakpoint(command.args, manager, state);
        break;
    case CommandType::VM_STDIN:
        ExecuteVmStdin(command.args, manager);
        break;
    case CommandType::EXIT:
        // Handled by the REPL loop itself.
        break;
    case CommandType::MODIFY_CONFIG:
    case CommandType::DUMP_CACHE:
    case CommandType::GET_MEMORY_POINT:
        throw std::invalid_argument("command not yet supported");
    case CommandType::INVALID:
    default:
        throw std::invalid_argument("unknown command");
    }
}

} // namespace command_handler
}//namespace Kites
