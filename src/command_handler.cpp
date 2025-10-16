/**
 * File Name: command_handler.cpp
 * Author: Vishank Singh
 * Github: https://github.com/VishankSingh
 */
#include "command_handler.h"

#include <string>
#include <sstream>
#include <vector>

namespace command_handler {
Command ParseCommand(const std::string &input) {
  std::istringstream iss(input);
  std::string command_str;
  iss >> command_str;
  command_handler::CommandType command_type = command_handler::CommandType::INVALID;

  if (command_str=="modify_config" || command_str=="mconfig") {
    command_type = command_handler::CommandType::MODIFY_CONFIG;
  } else if (command_str=="load" || command_str=="l") {
    command_type = command_handler::CommandType::LOAD;
  } else if (command_str=="run") {
    command_type = command_handler::CommandType::RUN;
  } else if (command_str=="stop") {
    command_type = command_handler::CommandType::STOP;
  } else if (command_str=="run_debug" || command_str=="rd") {
    command_type = command_handler::CommandType::DEBUG_RUN;
  } else if (command_str=="step" || command_str=="s") {
    command_type = command_handler::CommandType::STEP;
  } else if (command_str=="undo" || command_str=="u") {
    command_type = command_handler::CommandType::UNDO;
  } else if (command_str=="redo" || command_str=="r") {
    command_type = command_handler::CommandType::REDO;
  } else if (command_str=="reset") {
    command_type = command_handler::CommandType::RESET;
  } else if (command_str=="modify_register" || command_str=="mreg") {
    command_type = command_handler::CommandType::MODIFY_REGISTER;
  } else if (command_str=="dump_mem" || command_str=="dmem") {
    command_type = command_handler::CommandType::DUMP_MEMORY;
  } else if (command_str=="print_mem" || command_str=="pmem") {
    command_type = command_handler::CommandType::PRINT_MEMORY;
  } else if (command_str=="get_mem_point" || command_str=="gmp") {
    command_type = command_handler::CommandType::GET_MEMORY_POINT;
  } else if (command_str=="dump_cache") {
    command_type = command_handler::CommandType::DUMP_CACHE;
  } else if (command_str=="add_breakpoint") {
    command_type = command_handler::CommandType::ADD_BREAKPOINT;
  } else if (command_str=="remove_breakpoint") {
    command_type = command_handler::CommandType::REMOVE_BREAKPOINT;
  } else if (command_str=="vm_stdin" || command_str=="vmsin") {
    command_type = command_handler::CommandType::VM_STDIN;
  }
  else if (command_str=="exit" || command_str=="quit" || command_str=="q") {
    command_type = command_handler::CommandType::EXIT;
  }

  std::vector<std::string> args;
  std::string arg;
  bool in_quotes = false;
  std::ostringstream current_arg;

  while (iss) {
    char c = iss.get();
    if (!iss) break;

    if (c == '"') {
      in_quotes = !in_quotes;
      if (!in_quotes) {
        args.push_back(current_arg.str());
        current_arg.str("");
        current_arg.clear();
      }
    } else if (std::isspace(c) && !in_quotes) {
      if (!current_arg.str().empty()) {
        args.push_back(current_arg.str());
        current_arg.str("");
        current_arg.clear();
      }
    } else {
      current_arg << c;
    }
  }

  if (!current_arg.str().empty()) {
    args.push_back(current_arg.str());
  }




  return Command(command_type, args);
}

void ExecuteCommand(const Command &command, RVSSVM& vm) {
  (void)vm;
  (void)command;
}

} // namespace command_handler
