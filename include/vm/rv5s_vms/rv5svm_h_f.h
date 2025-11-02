/**
 * @file rv5svm_h_f.h
 * @brief Header for the 5-stage pipelined VM (RV5S) in Mode 4: Hazard Detection, With Forwarding.
 * @author Atharva and Harshit
 */
#pragma once

#include "vm/rv5s_vms/rv5s_vm_base.h"
#include "vm/pipeline_registers.h"
#include "vm/rv5s_vms/rv5s_control_unit.h"
#include "vm/rv5s_vms/rv5s_hdu.h" // <-- Including the HDU

#include <stack>
#include <vector>
#include <iostream>

class RV5StageVM_H_F : public RV5StageVM_Base
{
public:
    // Constructor and Destructor
    RV5StageVM_H_F();
    ~RV5StageVM_H_F() = default;

    // Overridden virtual functions from VmBase
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
        std::cout << "RV5StageVM_H_F" << std::endl;
    }

private:

    // This flag controls freezing the front-end (IF/ID registers and PC)
    bool stall_fetch_and_decode_ = false; 

    // --- Undo/Redo History (Managed internally) ---
    std::stack<StepDelta> undo_stack_;
    std::stack<StepDelta> redo_stack_;
    StepDelta current_delta_;

    // --- Private methods for each pipeline stage ---
    void pipeline_fetch();
    void pipeline_decode();
    void pipeline_execute();
    void pipeline_memory();
    void pipeline_writeback();

    void execute_float();
    void execute_double();
    void execute_csr();
    void handle_syscall();
    
    // Helper function to consolidate hazard checks (now only Load-Use)
    bool check_for_hazard();

    void print_pipeline_registers_debug();
};
