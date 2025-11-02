/**
 * @file rv5s_vm_nh_nf.h
 * @brief Header for the 5-stage pipelined VM (RV5S) supporting RV64G, No Forwarding.
 * @author Atharva and Harshit
 */
#pragma once

// Assuming rv5s_vm_base.h contains the definition of RV5StageVM_Base 
// which includes the pipeline registers (IF_ID_Register, etc.).
#include "vm/rv5s_vms/rv5s_vm_base.h"
#include "vm/pipeline_registers.h"
#include "vm/rv5s_vms/rv5s_control_unit.h"
#include <stack>
#include <iostream>
#include <cstdint>
#include <vector>


class RV5StageVM_NH_NF : public RV5StageVM_Base
{
public:
    RV5StageVM_NH_NF();
    ~RV5StageVM_NH_NF() = default;

    // Overridden virtual functions from VmBase (Public Interface)
    void Run() override;
    void DebugRun() override;
    void Step() override;
    void Undo() override;
    void Redo() override;
    void Reset() override;

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

protected: // Changed to PROTECTED, and methods below were moved here from private
    // --- Pipeline Stage Implementations (Overrides) ---
    void pipeline_fetch();
    void pipeline_decode();
    void pipeline_execute();
    void pipeline_memory();
    void pipeline_writeback();

    void print_pipeline_registers_debug();

    // --- Specialized handler functions (Overrides) ---
    void execute_float();
    void execute_double();
    void execute_csr();
    void handle_syscall();
};
