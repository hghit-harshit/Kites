/**
 * @file main.cpp
 * @brief Entry point for the Kites command-line interface.
 */
#include "assembler/assembler.h"
#include "common/assembled_program.h"
#include "common/globals.h"
#include "processor/processor_manager.h"
#include "processor/processor_types.h"
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
    std::cerr << "Usage: " << programName
               << " <assembly-file> [--vm <name>] [--mem-dump <hex_addr> <row_count>]...\n"
               << "  --vm <name>   rvss (default) | rv5-nh-nf | rv5-h-nf | rv5-nh-f | rv5-h-f\n"
               << "  --mem-dump    dump <row_count> 8-byte rows starting at <hex_addr>; "
                  "repeatable\n";
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

    if (argc < 2)
    {
        printUsage(argv[0]);
        return 1;
    }

    std::string processorTypeStr = "rvss";
    std::vector<std::string> memDumpArgs;

    for (int i = 2; i < argc; ++i)
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
        else
        {
            std::cerr << "Error: unknown argument '" << arg << "'.\n";
            printUsage(argv[0]);
            return 1;
        }
    }

    auto vmTypeIt = processorTypes.find(processorTypeStr);
    if (vmTypeIt == processorTypes.end())
    {
        std::cerr << "Error: unknown VM type '" << processorTypeStr << "'.\n";
        printUsage(argv[0]);
        return 1;
    }

    Kites::AssembledProgram program;
    try
    {
        program = Kites::assemble(argv[1]);
    }
    catch (const std::exception &e)
    {
        std::cerr << "Assembly failed: " << e.what() << std::endl;
        return 1;
    }

    Kites::ProcessorManager manager(nullptr, vmTypeIt->second);

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

    std::cout << "Assembled '" << argv[1] << "': " << program.text_buffer.size()
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
