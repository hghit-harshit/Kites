/**
 * @file rv5svm_nh_f.h
 * @brief Header for the 5-stage pipelined VM (RV5S) in Mode 2: No Hazard Detection, With Forwarding.
 * @author Atharva and Harshit
 */
#pragma once

#include "vm/rv5s_vms/rv5s_vm_base.h"
#include "vm/pipeline_registers.h"
#include "vm/rv5s_vms/rv5s_control_unit.h"
#include <stack>
#include <vector>
#include <iostream>

// --- Programmer NOP Requirements for Mode 2 ---
// * Data Hazards (ALU-ALU): 0 NOPs (Handled by Forwarding)
// * Load-Use Hazards: 1 NOP (Hardware cannot forward in time)
// * Conditional Branch Hazards: 3 NOPs (Same as Mode 1, no prediction/detection)
// * Unconditional Jumps (JAL/JALR): 1 NOP (Same as Mode 1)

class RV5StageVM_NH_F : public RV5StageVM_Base
{
public:
    // Constructor and Destructor
    RV5StageVM_NH_F();
    ~RV5StageVM_NH_F() = default;

    // Overridden virtual functions from VmBase
    // void Run() override;
    // void DebugRun() override;
    void Step() override;
    void Reset() override;

    void PrintType()
    {
        std::cout << "RV5StageVM_NH_F" << std::endl;
    }
    void SetActiveWireNames()  override;
private:
    // --- Private methods for each pipeline stage ---
    void pipeline_fetch() override;
    // void pipeline_decode() override;
    void pipeline_execute() override;
    uint64_t pipeline_execute_float() override;
    uint64_t pipeline_execute_double() override;
    //void pipeline_memory() override;
    // void pipeline_writeback() override;

    //void print_pipeline_registers_debug();
    void handle_syscall() override;
};
