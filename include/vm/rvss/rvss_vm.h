/**
 * @file rvss_vm.h
 * @brief RVSS VM definition
 * @author Vishank Singh, https://github.com/VishankSingh
 */
#ifndef RVSS_VM_H
#define RVSS_VM_H

#include "vm/vm_base.h"
#include "vm/vm_manager.h"
#include "rvss_control_unit.h"

#include <stack>
#include <vector>
#include <iostream>
#include <cstdint>

// TODO: use a circular buffer instead of a stack for undo/redo



class RVSSVM : public VmBase
{
public:
  RVSSControlUnit control_unit_;
  std::atomic<bool> stop_requested_ = false;

  // std::stack<StepDelta> undo_stack_;
  // std::stack<StepDelta> redo_stack_;
  // RingUndoRedo history_{1000}; // or however many steps you want to store

  //StepDelta current_delta_;

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

  RVSSVM();
  ~RVSSVM();

  void Run() override;
  void DebugRun() override;
  void Step() override;
  void Undo() override;
  void Redo() override;
  void Reset() override;

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
    std::cout << "rvssvm" << std::endl;
  }
};

#endif // RVSS_VM_H
