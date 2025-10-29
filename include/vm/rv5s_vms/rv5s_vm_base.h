#pragma once 
#include "vm/vm_base.h"
#include "vm/pipeline_registers.h"
#include "rv5s_control_unit.h"


class RV5StageVM_Base : public VmBase
{
public:
    RV5StageVM_Base() = default;
    ~RV5StageVM_Base() = default;

    // --- VM Control Functions ---
    void RequestStop()
    {
        stop_requested_ = true;
    }
    bool IsStopRequested() const
    {
        return stop_requested_;
    }
    void ClearStop()
    {
        stop_requested_ = false;
    }
    void PrintType()
    {
        std::cout << "rv5s_vm" << std::endl;
    }

protected:

    // Pipeline Registers
    IF_ID_Register if_id_reg_;
    ID_EX_Register id_ex_reg_;
    EX_MEM_Register ex_mem_reg_;
    MEM_WB_Register mem_wb_reg_;

    // The control unit for the pipeline
    RV5SControlUnit control_unit_;

    
    void print_pipeline_registers_debug();
    // --- Private methods for each pipeline stage ---
    void pipeline_fetch();
    void pipeline_decode();
    void pipeline_execute();
    void pipeline_memory();
    void pipeline_writeback();

    // --- Specialized handler functions (called from pipeline stages) ---
    void execute_float();
    void execute_double();
    void execute_csr();
    void handle_syscall();
};