#pragma once

#include "vm/vm_base.h"

#include "rv5s_control_unit.h"

#include <stack>
#include <vector>
#include <iostream>
#include <cstdint>

// TODO: use a circular buffer instead of a stack for undo/redo

// struct RegisterChange
// {
//     unsigned int reg_index;
//     unsigned int reg_type; // 0 for GPR, 1 for CSR, 2 for FPR
//     uint64_t old_value;
//     uint64_t new_value;
// };

// struct MemoryChange
// {
//     uint64_t address;
//     std::vector<uint8_t> old_bytes_vec;
//     std::vector<uint8_t> new_bytes_vec;
// };

// struct StepDelta
// {
//     uint64_t old_pc;
//     uint64_t new_pc;
//     std::vector<RegisterChange> register_changes;
//     std::vector<MemoryChange> memory_changes;
// };

// class RV5SVM : public VmBase
// {
// public:
//     RVSSControlUnit control_unit_;
//     std::atomic<bool> stop_requested_ = false;

//     std::stack<StepDelta> undo_stack_;
//     std::stack<StepDelta> redo_stack_;
//     // RingUndoRedo history_{1000}; // or however many steps you want to store

//     StepDelta current_delta_;

//     // intermediate variables
//     int64_t execution_result_{};
//     int64_t memory_result_{};
//     // int64_t memory_address_{};
//     // int64_t memory_data_{};
//     uint64_t return_address_{};

//     bool branch_flag_ = false;
//     int64_t next_pc_{}; // for jal, jalr,

//     // CSR intermediate variables
//     uint16_t csr_target_address_{};
//     uint64_t csr_old_value_{};
//     uint64_t csr_write_val_{};
//     uint8_t csr_uimm_{};

//     void Fetch();

//     void Decode();

//     void Execute();
//     void ExecuteFloat();
//     void ExecuteDouble();
//     void ExecuteCsr();
//     void HandleSyscall();

//     void WriteMemory();
//     void WriteMemoryFloat();
//     void WriteMemoryDouble();

//     void WriteBack();
//     void WriteBackFloat();
//     void WriteBackDouble();
//     void WriteBackCsr();

//     RV5SVM();
//     ~RV5SVM();

//     void Run() override;
//     void DebugRun() override;
//     void Step() override;
//     void Undo() override;
//     void Redo() override;
//     void Reset() override;

//     void RequestStop()
//     {
//         stop_requested_ = true;
//     }

//     bool IsStopRequested() const
//     {
//         return stop_requested_;
//     }

//     void ClearStop()
//     {
//         stop_requested_ = false;
//     }

//     void PrintType()
//     {
//         std::cout << "rv5svm" << std::endl;
//     }
// };

/**
 * @file rv5s_vm.h
 * @brief Header for the 5-stage pipelined VM (RV5S).
 * @author Atharva and Harshit
 */
#ifndef RV5S_VM_H
#define RV5S_VM_H

#include "vm/vm_base.h"
#include "vm/pipeline_registers.h"
#include "vm/rv5s/rv5s_control_unit.h"
#include <stack>
#include <vector>

// uselesss well define diffrent classes for each type of vm here
// Enum to manage the different hazard handling modes for future extension
enum class PipelineMode {
    NO_HAZARD_NO_FORWARDING,      // Option 1
    FORWARDING_NO_HAZARD,         // Option 2
    HAZARD_NO_FORWARDING,         // Option 3
    HAZARD_AND_FORWARDING,        // Option 4
    STATIC_BRANCH_PREDICTION,     // Option 5
    DYNAMIC_BRANCH_PREDICTION     // Option 6
};

// --- Data structures for Undo/Redo state tracking ---

struct RegisterChange
{
    unsigned int reg_index;
    unsigned int reg_type; // 0 for GPR, 1 for CSR, 2 for FPR
    uint64_t old_value;
    uint64_t new_value;
};

struct MemoryChange
{
    uint64_t address;
    std::vector<uint8_t> old_bytes_vec;
    std::vector<uint8_t> new_bytes_vec;
};

// A StepDelta records all architectural state changes that complete in a single clock cycle.
struct StepDelta
{
    uint64_t old_pc;
    uint64_t new_pc;
    std::vector<RegisterChange> register_changes; // From the WB stage
    std::vector<MemoryChange> memory_changes;     // From the MEM stage
};

class RV5StageVM : public VmBase
{
public:
    RV5StageVM();
    ~RV5StageVM() = default;

    // Overridden virtual functions from VmBase
    void Run() override;
    void DebugRun() override;
    void Step() override;
    void Undo() override;
    void Redo() override;
    void Reset() override;

    void SetPipelineMode(PipelineMode mode);

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

private:
    // --- Pipeline Components ---
    PipelineMode current_mode_;

    // Pipeline Registers
    IF_ID_Register if_id_reg_;
    ID_EX_Register id_ex_reg_;
    EX_MEM_Register ex_mem_reg_;
    MEM_WB_Register mem_wb_reg_;

    // The control unit for the pipeline
    RV5SControlUnit control_unit_;

    // --- Undo/Redo History ---
    std::stack<StepDelta> undo_stack_;
    std::stack<StepDelta> redo_stack_;
    StepDelta current_delta_;

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

#endif // RV5S_VM_H
