/**
 * @file rv5s_processor_nh_nf.h
 * @brief 5-stage pipelined processor (RV5S): no hazard detection, no forwarding.
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
class RV5StageProcessorNHNF : public RV5StageVM_Base
{
  public:
    RV5StageProcessorNHNF();
    ~RV5StageProcessorNHNF() = default;

    void PrintType()
    {
        std::cout << "RV5StageProcessorNHNF" << std::endl;
    }

    void SetActiveWireNames() override;

  private:
    bool handleSystemInExecute() override;
};
} // namespace Kites
