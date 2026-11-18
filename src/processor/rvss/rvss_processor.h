/**
 * @file rvss_vm.h
 * @brief RVSS VM definition
 * @author Vishank Singh, https://github.com/VishankSingh
 */
#ifndef RVSS_VM_H
#define RVSS_VM_H

#include "processor/processor_base.h"
#include "processor/processor_manager.h"
#include "rvss_control_unit.h"

#include <cstdint>
#include <iostream>
#include <stack>
#include <vector>

namespace Kites
{
struct RVSingleStageStepDelta : public StepDelta
{
    // uint64_t old_pc;
    // uint64_t new_pc;
    // std::vector<RegisterChange> register_changes;
};

class RVSSProcessor : public ProcessorBase
{
  public:
    RVSSControlUnit control_unit_;
    // intermediate variables
    int64_t execution_result_{};
    int64_t memory_result_{};
    // int64_t memory_address_{};
    // int64_t memory_data_{};
    uint64_t return_address_{};

    bool branch_flag_ = false;
    int64_t next_pc_{}; // for jal, jalr,

    // CSR intermediate variables
    uint16_t csr_target_address_{};
    uint64_t csr_old_value_{};
    uint64_t csr_write_val_{};
    uint8_t csr_uimm_{};

    // Per-step deltas; the first entry is an empty sentinel (see UndoBuffer).
    UndoBuffer<StepDelta> m_undoBuffer{UndoBuffer<StepDelta>::unbounded};

    RVSingleStageStepDelta current_delta_;

    void Fetch();

    void Decode();

    void Execute();
    void ExecuteFloat();
    void ExecuteDouble();
    void ExecuteCsr();
    void HandleSyscall();

    void WriteMemory();
    void WriteMemoryFloat();
    void WriteMemoryDouble();

    void WriteBack();
    void WriteBackFloat();
    void WriteBackDouble();
    void WriteBackCsr();

    RVSSProcessor();
    ~RVSSProcessor();

    void Run() override;
    void DebugRun() override;
    void Step() override;
    bool StepAndNotify() override;
    void Undo() override;
    void Redo() override;
    void Reset() override;

    void SetActiveWireNames() override;
    void setProcessorState() override;

    void PrintType()
    {
        std::cout << "rvssvm" << std::endl;
    }
};
}//namespace Kites
#endif // RVSS_VM_H
