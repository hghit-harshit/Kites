/**
 * @file control_unit_base.h
 * @brief Control unit base class definition
 * @author Vishank Singh, https://github.com/VishankSingh
 */
#ifndef CONTROL_UNIT_BASE_H
#define CONTROL_UNIT_BASE_H

#include "registers.h"
#include "alu.h"

/**
 * @brief The ControlUnit class is the base class for the control unit of the CPU.
 */
class ControlUnit {
 public:
  virtual ~ControlUnit() = default;

  void Reset() {
    alu_src_ = false;
    mem_to_reg_ = false;
    reg_write_ = false;
    mem_read_ = false;
    mem_write_ = false;
    branch_ = false;
    alu_op_ = 0;
  }

  virtual void SetControlSignals(uint32_t instruction) = 0;
  virtual alu::AluOp GetAluSignal(uint32_t instruction, bool ALUOp) = 0;

  [[nodiscard]] bool GetAluSrc() const;
  [[nodiscard]] bool GetMemToReg() const;
  [[nodiscard]] bool GetRegWrite() const;
  [[nodiscard]] bool GetMemRead() const;
  [[nodiscard]] bool GetMemWrite() const;
  [[nodiscard]] uint8_t GetAluOp() const;
  [[nodiscard]] bool GetBranch() const;

 protected:
  bool reg_write_ = false;
  bool branch_ = false;
  bool alu_src_ = false;
  bool mem_read_ = false;
  bool mem_write_ = false;
  bool mem_to_reg_ = false;
  bool pc_src_ = false;

  uint8_t alu_op_{};
};

#endif // CONTROL_UNIT_BASE_H
