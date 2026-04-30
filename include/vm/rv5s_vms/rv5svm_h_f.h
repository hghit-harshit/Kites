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
    // void Run() override;
    // void DebugRun() override;
    void Step() override;
    void Reset() override;
    
    void PrintType()
    {
        std::cout << "RV5StageVM_H_F" << std::endl;
    }

    void SetActiveWireNames() override; 
private:
    // The Hazard Detection Unit instance
    //RV5SHazardUnit hazard_unit_;

    // This flag controls freezing the front-end (IF/ID registers and PC)
    bool stall_fetch_and_decode_ = false; 

    // --- Private methods for each pipeline stage ---
    void pipeline_fetch() override;
    // void pipeline_decode() override;
    void pipeline_execute() override;
    uint64_t pipeline_execute_float() override;
    uint64_t pipeline_execute_double() override;
    //void pipeline_memory() override;
    // void pipeline_writeback() override;


    // Helper function to consolidate hazard checks (now only Load-Use)

    //void print_pipeline_registers_debug();
    void handle_syscall() override;
};
