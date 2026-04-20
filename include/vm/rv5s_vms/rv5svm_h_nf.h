/**
 * @file rv5svm_h_nf.h
 * @brief Header for the 5-stage pipelined VM (RV5S) in Mode 3: Hazard Detection, No Forwarding.
 * @author Atharva and Harshit
 */
#pragma once

#include "vm/rv5s_vms/rv5s_vm_base.h"
#include "vm/pipeline_registers.h"
#include "vm/rv5s_vms/rv5s_control_unit.h"
#include "vm/rv5s_vms/rv5s_hdu.h" // <-- Including the new HDU

#include <stack>
#include <vector>
#include <iostream>

class RV5StageVM_H_NF : public RV5StageVM_Base
{
public:
    // Constructor and Destructor
    RV5StageVM_H_NF();
    ~RV5StageVM_H_NF() = default;

    // Overridden virtual functions from VmBase
    // void Run() override;
    // void DebugRun() override;
    void Step() override;
    void Undo() override;
    void Redo() override;
    void Reset() override;

    // --- VM Control Functions ---
    void PrintType()
    {
        std::cout << "RV5StageVM_H_NF" << std::endl;
    }
    void SetActiveWireNames()  override;
private:
    // The Hazard Detection Unit instance
    //RV5SHazardUnit hazard_unit_;

    // This flag is used to control the pipeline flow when a stall is detected.
    // If true, the IF stage freezes the PC and does not update IF/ID.
    bool stall_fetch_and_decode_ = false; 

    // --- Undo/Redo History (Managed internally) ---
    std::stack<StepDelta> undo_stack_;
    std::stack<StepDelta> redo_stack_;
    StepDelta current_delta_;

    // --- Private methods for each pipeline stage ---
    void pipeline_fetch() override;
    // void pipeline_decode() override;
    void pipeline_execute() override;
    //void pipeline_memory() override;
    // void pipeline_writeback() override;

    // Helper function to consolidate hazard checks using the dedicated unit
    bool check_for_hazard();

    //void print_pipeline_registers_debug();
    void handle_syscall() override;
};
