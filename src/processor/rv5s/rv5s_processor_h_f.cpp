/**
 * @file rv5s_processor_h_f.cpp
 * @brief Implementation for the 5-stage pipelined VM (RV5S) in Mode 4: Hazard Detection, With
 * Forwarding.
 * * NOTE: This is a PURE INTEGER (GPR) implementation.
 * * Stall Rule: Only Load-Use GPR dependencies require a 1-cycle stall. All others are solved by
 * forwarding.
 * * Control Hazards (JAL/JALR, B-Type) are AUTOMATICALLY handled by flush/redirect.
 * @author Atharva and Harshit
 */
#include "processor/rv5s/rv5s_processor_h_f.h" // Assuming this header now defines RV5StageProcessorHF
#include "common/instructions.h"
#include "config/config.h"
#include "ui/processor_tab/processor_designs/rv5s_processor_h_f_circuit_scene.h"
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

// Define stall count constants (used internally, defined in rv5s_hdu.h)
// constexpr int STALL_NONE = 0;
// constexpr int STALL_ONE_CYCLE = 1;

// --- RV5StageProcessorHF Class Implementation ---

RV5StageProcessorHF::RV5StageProcessorHF()
    : RV5StageVM_Base(/*hazardDetection=*/true, /*forwarding=*/true)
{
    // Initialize ProcessorBase members
    // program_counter_ = 0;
    // instructions_retired_ = 0;
    // cycle_s_ = 0;
    // stall_cycles_ = 0;

    // // Initialize local members
    // stall_fetch_and_decode_ = false;

    // Reset components and history
#ifndef DISABLE_GUI
    circuit_scene_ = std::make_unique<Kites::RV5StageVM_H_F_CircuitScene>();
    connect(this, &ProcessorBase::updateCircuitStateSignal, circuit_scene_.get(),
            &Kites::RV5StageVM_H_F_CircuitScene::updateCircuitState);
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
    active_wires_.append("RdP4_to_FU");
    active_wires_.append("P4EB_to_FWD_unit");
    active_wires_.append("P2_to_FDU_rs1");
    active_wires_.append("P2_to_FDU_RS2");
    active_wires_.append("ALU_to_P3");
    active_wires_.append("P3ALUres_to_DMup");
    active_wires_.append("ALUcontrol_to_ALU");
    active_wires_.append("Fmux1_to_ALU");
    active_wires_.append("Fmux_to_ALUMux_up");
    active_wires_.append("P2_to_FMUX");
    active_wires_.append("P2_to_FMUX2_UP");
    active_wires_.append("P2_to_ALUMux");
    active_wires_.append("P2_to_ALUcontrol_Control");

    always_active_wires_count_ = active_wires_.size();
}

void RV5StageProcessorHF::SetActiveWireNames()
{
    // Clear and populate canonical wires for the Hazard+Forwarding (H_F) circuit
    active_wires_.resize(always_active_wires_count_);
    // Backbone / always-visible wires (names sourced from H_F_Processor.json)

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
    if (ex_mem_reg_.prev_mem_read || ex_mem_reg_.mem_read)
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
    if (mem_wb_reg_.prev_reg_write || mem_wb_reg_.reg_write)
    {
        active_wires_.append("P4_wbcontrol_to_WBmux");
        active_wires_.append("RdP4_to_FU");
        active_wires_.append("WBMux_to_RF");
    }

    // Forwarding unit activity: show FWD wires when forwarding is relevant
    if (ex_mem_reg_.reg_write && ex_mem_reg_.rd != 0)
    {
        if (ex_mem_reg_.rd == id_ex_reg_.rs1)
        {
            active_wires_.append("P2_to_FDU_rs1");
            active_wires_.append("P4EB_to_FWD_unit");
            active_wires_.append("FDU_to_FMUX1_cntrl");
            active_wires_.append("FDU_to_fMUX2");
        }
        if (ex_mem_reg_.rd == id_ex_reg_.rs2)
        {
            active_wires_.append("P2_to_FDU_RS2");
            active_wires_.append("P4EB_to_FWD_unit");
            active_wires_.append("FDU_to_FMUX3");
        }
    }

    if (mem_wb_reg_.prev_reg_write && mem_wb_reg_.prev_rd != 0)
    {
        if (mem_wb_reg_.prev_rd == id_ex_reg_.rs1)
        {
            active_wires_.append("P4EB_to_FWD_unit");
            active_wires_.append("FDU_to_FMUX1_cntrl");
        }
        if (mem_wb_reg_.prev_rd == id_ex_reg_.rs2)
        {
            active_wires_.append("P4EB_to_FWD_unit");
            active_wires_.append("FDU_to_FMUX3");
        }
    }

    // Branch / PC selection wires
    if (ex_mem_reg_.prev_branch_taken || ex_mem_reg_.branch_taken)
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

    // Hazard Detection Unit (HDU) / stall indicators
    if (stall_fetch_and_decode_ || stall_cycles_ > 0)
    {
        active_wires_.append("HDU_to_P1");
        active_wires_.append("HDUMux_to_P2");
        active_wires_.append("HDU_mux_lowest_toP2");
        active_wires_.append("HDU_to_HDU_mUX");
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
