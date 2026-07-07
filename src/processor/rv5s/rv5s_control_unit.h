/**
 * @file rv5s_control_unit.h
 * @brief Header for the 5-stage pipelined Control Unit.
 * @author Atharva and Harshit
 */
#ifndef RV5S_CONTROL_UNIT_H
#define RV5S_CONTROL_UNIT_H

#include "processor/alu.h"
#include "processor/control_unit_base.h"

namespace Kites
{
class RV5SControlUnit : public ControlUnit
{
  public:
    void SetControlSignals(uint32_t instruction) override;
    alu::AluOp GetAluSignal(uint32_t instruction, bool ALUOp) override;
};
}//namespace Kites
#endif // RV5S_CONTROL_UNIT_H