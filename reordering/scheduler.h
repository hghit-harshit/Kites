#pragma once
#include "DAG.h"
#include "heurestic.h"
#include "parser.h"

// helper functions

class scheduler
{
  private:
    std::vector<DAG_matrix> depend_matrix_full;
    bool forwarding;
    std::vector<std::vector<parsed_instruction>> blocks;

  public:
    scheduler(std::vector<std::vector<parsed_instruction>> blocks,
              std::vector<std::vector<std::vector<depend_description>>> given, bool tem1);
    std::vector<int>
    block_scheduler(const std::vector<parsed_instruction> &instrs,
                    const std::vector<std::vector<depend_description>> &block);

    int delay(dpd_format type, parsed_instruction a, parsed_instruction);
    std::vector<parsed_instruction> code_scheduler(int * cycles_before,int * cycles_after);

    int block_cycles(std::vector<parsed_instruction>& instrs,std::vector<std::vector<depend_description>>& block,std::vector<int>& order);
};