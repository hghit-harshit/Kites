/**
 * @file repl.h
 * @brief Interactive REPL loop for kites-cli.
 */
#pragma once

#include "command_handler/command_handler.h"
#include "processor/processor_manager.h"

namespace Kites
{
/**
 * @brief Runs the interactive command loop against an already-constructed VM manager until the
 * user exits (via 'exit'/'quit'/'q' or EOF on stdin).
 * @return process exit code (0 on a clean exit).
 */
int RunRepl(ProcessorManager &manager, command_handler::ReplState &state);
} // namespace Kites

