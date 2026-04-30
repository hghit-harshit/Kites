#pragma once

#include "vm/vm_base.h"

#include "rv5s_control_unit.h"

#include <stack>
#include <vector>
#include <iostream>
#include <cstdint>

// TODO: use a circular buffer instead of a stack for undo/redo


/**
 * @file rv5s_vm.h
 * @brief Header for the 5-stage pipelined VM (RV5S).
 * @author Atharva and Harshit
 */
#pragma once

#include "vm/rv5s_vms/rv5s_vm_base.h"
#include "vm/pipeline_registers.h"
#include "vm/rv5s_vms/rv5s_control_unit.h"
#include <stack>
#include <vector>



class RV5StageVM_NH_NF : public RV5StageVM_Base
{
public:
    RV5StageVM_NH_NF() ;
    ~RV5StageVM_NH_NF() = default;

    // Overridden virtual functions from VmBase
    // void Run() override;
    // void DebugRun() override;
    void Step() override;
    void Reset() override;

    // --- VM Control Functions ---
    void PrintType()
    {
        std::cout << "rv5s_vm" << std::endl;
    }
    void SetActiveWireNames()  override;
    //void SetVMStateMap() override{};
private:
    // Pipeline Registers
    // IF_ID_Register if_id_reg_;
    // ID_EX_Register id_ex_reg_;
    // EX_MEM_Register ex_mem_reg_;
    // MEM_WB_Register mem_wb_reg_;

    // The control unit for the pipeline
    //RV5SControlUnit control_unit_;

    //void print_pipeline_registers_debug();
    //bool is_pipeline_drained() const;
    // --- Private methods for each pipeline stage ---
    void pipeline_fetch() override;
    //void pipeline_decode() override;
    void pipeline_execute() override;

    uint64_t pipeline_execute_float() override;
    uint64_t pipeline_execute_double() override;
    //void pipeline_memory() override;
    // void pipeline_writeback() override;

   // uint64_t execute_float();
  // uint64_t execute_double();
    //void memory_float();


    // --- Specialized handler functions (called from pipeline stages) ---
    
    void handle_syscall() override;
};


