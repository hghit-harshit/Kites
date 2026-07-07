/**
 * @file rv5s_processor_h_nf.h
 * @brief Header for the 5-stage pipelined VM (RV5S) in Mode 3: Hazard Detection, No Forwarding.
 * @author Atharva and Harshit
 */
#pragma once

#include "processor/pipeline_registers.h"
#include "processor/rv5s/rv5s_control_unit.h"
#include "processor/rv5s/rv5s_hdu.h" // <-- Including the new HDU
#include "processor/rv5s/rv5s_processor_base.h"

#include <iostream>
#include <stack>
#include <vector>

namespace Kites
{
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
    void Reset() override;

    // --- VM Control Functions ---
    void PrintType()
    {
        std::cout << "RV5StageVM_H_NF" << std::endl;
    }
    void SetActiveWireNames() override;

  private:
    // The Hazard Detection Unit instance
    // RV5SHazardUnit hazard_unit_;

    // This flag is used to control the pipeline flow when a stall is detected.
    // If true, the IF stage freezes the PC and does not update IF/ID.
    bool stall_fetch_and_decode_ = false;

    // --- Private methods for each pipeline stage ---
    void pipeline_fetch() override;
    // void pipeline_decode() override;
    void pipeline_execute() override;

    uint64_t pipeline_execute_float() override;
    uint64_t pipeline_execute_double() override;
    // void pipeline_memory() override;
    //  void pipeline_writeback() override;

    // Helper function to consolidate hazard checks using the dedicated unit

    // void print_pipeline_registers_debug();
    void handle_syscall() override;
};
}//namespace Kites