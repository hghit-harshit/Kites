/**
 * @file rv5s_processor_h_nf.h
 * @brief 5-stage pipelined processor (RV5S): hazard detection without forwarding.
 * @author Atharva and Harshit
 *
 * The pipeline itself lives in RV5StageVM_Base; this variant only selects the
 * hazard-detection/forwarding configuration and its circuit wiring.
 */
#pragma once

#include "processor/rv5s/rv5s_processor_base.h"

#include <iostream>

namespace Kites
{
class RV5StageProcessorHNF : public RV5StageVM_Base
{
  public:
    RV5StageProcessorHNF();
    ~RV5StageProcessorHNF() = default;

    void PrintType()
    {
        std::cout << "RV5StageProcessorHNF" << std::endl;
    }

    void SetActiveWireNames() override;
};
} // namespace Kites
