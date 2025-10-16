/**
 * @file control_unit_base.cpp
 * @brief Control unit base class implementation
 * @author Vishank Singh, https://github.com/VishankSingh
 */

#include "vm/control_unit_base.h"

#include <cstdint>

bool ControlUnit::GetAluSrc() const {
  return alu_src_;
}

bool ControlUnit::GetMemToReg() const {
  return mem_to_reg_;
}

bool ControlUnit::GetRegWrite() const {
  return reg_write_;
}

bool ControlUnit::GetMemRead() const {
  return mem_read_;
}

bool ControlUnit::GetMemWrite() const {
  return mem_write_;
}

uint8_t ControlUnit::GetAluOp() const {
  return alu_op_;
}

bool ControlUnit::GetBranch() const {
  return branch_;
}