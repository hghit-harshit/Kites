/**
 * @file rv5s_processor_h_nf.cpp
 * @brief Implementation for the 5-stage pipelined VM (RV5S) in Mode 3: Hazard Detection, No
 * Forwarding.
 * * NOTE: This VM is fully autonomous for all hazards (data and control).
 * * Stall Rule: Any GPR data dependency results in a 2-cycle stall (2 bubbles).
 * * Control Hazards (JAL/JALR, B-Type) are AUTOMATICALLY handled by flush/redirect.
 * @author Atharva and Harshit
 */
#include "processor/rv5s/rv5s_processor_h_nf.h"
#include "common/instructions.h"
#include "config/config.h"
#include "ui/processor_tab/processor_designs/rv5s_processor_h_nf_circuit_scene.h"
#include "processor/alu.h"
#include "processor/pipeline_registers.h"
#include "processor/processor_base.h"
#include <algorithm>
#include <chrono>
#include <iostream>
#include <thread>
#include <tuple>

namespace Kites
{
using namespace alu;

// --- RV5StageProcessorHNF Class Implementation ---

RV5StageProcessorHNF::RV5StageProcessorHNF()
    : RV5StageVM_Base(/*hazardDetection=*/true, /*forwarding=*/false)
{
#ifndef DISABLE_GUI
    circuit_scene_ = std::make_unique<Kites::RV5StageVM_H_NF_CircuitScene>();
    connect(this, &ProcessorBase::updateCircuitStateSignal, circuit_scene_.get(),
            &Kites::RV5StageVM_H_NF_CircuitScene::updateCircuitState);
#endif
    Reset();

    active_wires_.append("PC_to_IM");
    active_wires_.append("IM_to_P1");
    active_wires_.append("P1_to_P2_PCcarry");
    active_wires_.append("P1_to_Control_RF_andall");
    active_wires_.append("RF_to_P2_UP");
    active_wires_.append("RFdown_to_P2");
    active_wires_.append("PCMux_to_PC");
    active_wires_.append("P2_to_ALU2");
    active_wires_.append("P2_to_P3_MEMControl");
    active_wires_.append("P2_to_P3_WBcontrol");
    active_wires_.append("ALU2_to_P3");
    active_wires_.append("P3_to_P4_WBcontrol");
    active_wires_.append("P2_to_ALUControl");
    active_wires_.append("ALU_to_P3");
    active_wires_.append("P3ALUres_to_DMup");
    active_wires_.append("ALUcontrol_to_ALU");
    active_wires_.append("P2_to_ALUMux");
    active_wires_.append("P2_to_ALUcontrol_Control");

    always_active_wires_count_ = active_wires_.size();
}

void RV5StageProcessorHNF::SetActiveWireNames()
{
    // Clear and populate canonical wires for the Hazard Detection + No Forwarding (H_NF) circuit
    active_wires_.erase(active_wires_.begin() + always_active_wires_count_, active_wires_.end());

    // Immediate / ALU source selection
    if (id_ex_reg_.alu_src)
    {
        active_wires_.append("Imm_to_P2");
        active_wires_.append("P2Imm_to_ALU2_down");
    }

    // Control signals in ID stage reflected on P2 control wires
    if (id_ex_reg_.mem_write)
    {
        active_wires_.append("P2_to_P3_MEMControl");
    }
    if (id_ex_reg_.reg_write)
    {
        active_wires_.append("P2_to_P3_WBcontrol");
    }

    // Memory control / data path (EX/MEM stage)
    if (ex_mem_reg_.mem_read)
    {
        active_wires_.append("P3_to_DM_Memread");
        active_wires_.append("DMMux_to_DM");
    }
    if (ex_mem_reg_.mem_write)
    {
        active_wires_.append("P3_TO_DM_control_memwrite");
        active_wires_.append("P3ALUres_to_DMup");
    }

    // Register writeback paths (MEM/WB stage)
    if (mem_wb_reg_.reg_write)
    {
        active_wires_.append("P4_wbcontrol_to_WBmux");
        active_wires_.append("RdP4_to_FU");
        active_wires_.append("WBMux_to_RF");
    }

    // Branch / PC selection wires
    if (ex_mem_reg_.branch_taken || id_ex_reg_.branch)
    {
        active_wires_.append("ALU_zerores_to_P3");
        active_wires_.append("ANDGate_lower_entry");
        active_wires_.append("ANDGATE_to_PCMUX");
        active_wires_.append("P3_to_PCMux");
    }
    else
    {
        active_wires_.append("ALU1_to_PCMuxUp");
    }

    // Hazard Detection Unit (HDU) / stall indicators (key feature of H_NF mode)
    if (stall_fetch_and_decode_ || stall_cycles_ > 0)
    {
        active_wires_.append("HDU_to_P1");
        active_wires_.append("HDU_to_HDU_mUX");
        active_wires_.append("HDUMux_to_P2");
        active_wires_.append("HDU_mux_lowest_toP2");
        active_wires_.append("Control_ControlMux");
    }

    // Preserve ordering while removing duplicates
    QList<QString> uniqueList;
    QSet<QString> seen;
    for (const auto &w : active_wires_)
    {
        if (!seen.contains(w))
        {
            uniqueList.append(w);
            seen.insert(w);
        }
    }
    active_wires_ = uniqueList;

    // The scene will read `active_wires_` when `updateCircuitStateSignal` is emitted by callers.
}
} // namespace Kites
