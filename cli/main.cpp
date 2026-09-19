/**
 * @file main.cpp
 * @brief Entry point for the Kites command-line interface.
 */
#include "assembler/assembler.h"
#include "command_handler/command_handler.h"
#include "common/assembled_program.h"
#include "common/globals.h"
#include "processor/processor_manager.h"
#include "processor/processor_types.h"
#include "repl.h"
#include "utils/utils.h"

#include <QCoreApplication>
#include <QObject>
#include <QString>

#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

namespace
{
void printUsage(const char *programName)
{
    std::cerr
        << "Usage: " << programName
        << " [assembly-file] [--vm <name>] [--mem-dump <hex_addr> <row_count>]... [--repl|-i]\n"
        << "  assembly-file   optional; if given without --repl, assembles, runs to\n"
        << "                  completion, and dumps state (batch mode)\n"
        << "  --vm <name>     rvss (default) | rv5-nh-nf | rv5-h-nf | rv5-nh-f | rv5-h-f\n"
        << "  --mem-dump      dump <row_count> 8-byte rows starting at <hex_addr>; "
           "repeatable (batch mode only)\n"
        << "  --repl, -i      drop into the interactive REPL instead of auto-running;\n"
        << "                  if assembly-file is given it is loaded before the prompt\n"
        << "  (running with no assembly-file also enters the REPL)\n";
}

const std::unordered_map<std::string, Kites::ProcessorType> processorTypes = {
    {"rvss", Kites::ProcessorType::RVSS},
    {"rv5-nh-nf", Kites::ProcessorType::RV5Stage_NH_NF},
    {"rv5-h-nf", Kites::ProcessorType::RV5Stage_H_NF},
    {"rv5-nh-f", Kites::ProcessorType::RV5Stage_NH_F},
    {"rv5-h-f", Kites::ProcessorType::RV5Stage_H_F},
};
} // namespace

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    Kites::setupVmStateDirectory();

    std::string assemblyFile;
    std::string processorTypeStr = "rvss";
    std::vector<std::string> memDumpArgs;
    bool replRequested = false;

    for (int i = 1; i < argc; ++i)
    {
        std::string arg = argv[i];
        if (arg == "--vm")
        {
            if (i + 1 >= argc)
            {
                std::cerr << "Error: --vm requires a value.\n";
                printUsage(argv[0]);
                return 1;
            }
            processorTypeStr = argv[++i];
        }
        else if (arg == "--mem-dump")
        {
            if (i + 2 >= argc)
            {
                std::cerr << "Error: --mem-dump requires <hex_addr> <row_count>.\n";
                printUsage(argv[0]);
                return 1;
            }
            memDumpArgs.push_back(argv[++i]);
            memDumpArgs.push_back(argv[++i]);
        }
        else if (arg == "--repl" || arg == "-i")
        {
            replRequested = true;
        }
        else if (!arg.empty() && arg[0] == '-')
        {
            std::cerr << "Error: unknown argument '" << arg << "'.\n";
            printUsage(argv[0]);
            return 1;
        }
        else if (assemblyFile.empty())
        {
            assemblyFile = arg;
        }
        else
        {
            std::cerr << "Error: unexpected extra argument '" << arg << "'.\n";
            printUsage(argv[0]);
            return 1;
        }
    }

    auto processorTypeIt = processorTypes.find(processorTypeStr);
    if (processorTypeIt == processorTypes.end())
    {
        std::cerr << "Error: unknown VM type '" << processorTypeStr << "'.\n";
        printUsage(argv[0]);
        return 1;
    }

    if (replRequested || assemblyFile.empty())
    {
        Kites::ProcessorManager manager(nullptr, processorTypeIt->second);
        manager.setStepDelay(0);

        Kites::command_handler::ReplState state;
        if (!assemblyFile.empty())
        {
            try
            {
                Kites::AssembledProgram program = Kites::assemble(assemblyFile);
                manager.loadProgram(program);
                state.currentProgram = program;
                state.programLoaded = true;
                std::cout << "Loaded '" << assemblyFile << "': " << program.text_buffer.size()
                           << " instruction word(s), " << program.data_buffer.size()
                           << " data item(s).\n";
            }
            catch (const std::exception &e)
            {
                std::cerr << "Assembly failed: " << e.what() << "\n";
            }
        }

        return Kites::RunRepl(manager, state);
    }

    Kites::AssembledProgram program;
    try
    {
        program = Kites::assemble(assemblyFile);
    }
    catch (const std::exception &e)
    {
        std::cerr << "Assembly failed: " << e.what() << std::endl;
        return 1;
    }

    Kites::ProcessorManager manager(nullptr, processorTypeIt->second);

    bool runFailed = false;
    std::string runErrorMsg;
    QObject::connect(&manager, &Kites::ProcessorManager::runErrorSignal, &manager,
                      [&](const QString &message, int /*sourceLine*/)
                      {
                          runFailed = true;
                          runErrorMsg = message.toStdString();
                      });

    // ProcessorBase::step_delay_ defaults to 1000ms and is meant to pace the
    // GUI's per-instruction animation; it must be zeroed here or an N-instruction
    // program takes N seconds to run.
    manager.setStepDelay(0);
    manager.loadProgram(program);

    // Known limitation: a program hitting a blocking read syscall will hang here
    // forever, since nothing in this CLI feeds terminal stdin into the VM's input
    // queue yet (that's separate, future work).
    manager.run();

    if (runFailed)
    {
        std::cerr << "Run failed: " << runErrorMsg << std::endl;
    }

    Kites::DumpRegisters(Kites::globals::registers_dump_file_path, *manager.getRegisterFile());

    if (!memDumpArgs.empty())
    {
        try
        {
            manager.getMemoryController()->dumpMemory(memDumpArgs);
        }
        catch (const std::exception &e)
        {
            std::cerr << "Memory dump failed: " << e.what() << std::endl;
            return 3;
        }
    }

    std::cout << "Assembled '" << assemblyFile << "': " << program.text_buffer.size()
               << " instruction word(s), " << program.data_buffer.size() << " data item(s).\n"
               << "Run " << (runFailed ? "failed" : "completed") << ".\n"
               << "Registers   -> " << Kites::globals::registers_dump_file_path.string() << "\n"
               << "Disassembly -> " << Kites::globals::disassembly_file_path.string() << "\n"
               << "Errors      -> " << Kites::globals::errors_dump_file_path.string();
    if (!memDumpArgs.empty())
    {
        std::cout << "\nMemory      -> " << Kites::globals::memory_dump_file_path.string();
    }
    std::cout << std::endl;

    return runFailed ? 2 : 0;
}
