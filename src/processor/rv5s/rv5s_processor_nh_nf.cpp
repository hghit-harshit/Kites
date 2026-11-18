/**
 * @file rv5s_processor_nh_nf.cpp
 * @brief Implementation for the 5-stage pipelined VM (RV5S) in Mode 1: No Hazard Detection, No
 * Forwarding (NH_NF).
 * * Data Hazards (R-R, L-R, R-Store, L-Store) require 2 NOPs (Programmer Responsibility).
 * * Control Hazards (JAL/JALR, B-Type) are AUTOMATICALLY handled by the pipeline.
 * * Control Penalties: JAL/JALR = 1 bubble (EX stage); B-Type = 2 bubbles (MEM stage, 3-cycle
 * penalty).
 * * @author Atharva and Harshit
 */
#include "processor/rv5s/rv5s_processor_nh_nf.h"
#include "common/instructions.h"
#include "config/config.h"
#include "common/debug_colors.h"
#include "ui/processor_tab/processor_designs/rv5s_processor_nh_nf_circuit_scene.h"
#include "processor/alu.h"
#include "processor/processor_base.h" // For ImmGenerator, etc.


#include <algorithm>
#include <chrono>
#include <iostream>
#include <thread>
#include <tuple>

namespace Kites
{
// --- VmBase Pure Virtual Method Implementations (Run, DebugRun, Reset, Step) ---
RV5StageProcessorNHNF::RV5StageProcessorNHNF()
    : RV5StageVM_Base(/*hazardDetection=*/false, /*forwarding=*/false)
{

// Reset components and history
#ifndef DISABLE_GUI
    circuit_scene_ = std::make_unique<Kites::RV5StageVM_NH_NF_CircuitScene>();
    connect(this, &ProcessorBase::updateCircuitStateSignal, circuit_scene_.get(),
            &Kites::RV5StageVM_NH_NF_CircuitScene::updateCircuitState);
#endif
    Reset();
    active_wires_.append("PC_to_IM");
    active_wires_.append("PCMux_to_PC");
    active_wires_.append("IM_to_P1");
    active_wires_.append("P1_to_P2_PCcarry");
    active_wires_.append("P1_to_Control_RF_andall");
    active_wires_.append("RF_to_P2_UP");
    active_wires_.append("RFdown_to_P2");
    active_wires_.append("P2_to_ALUSRC");
    active_wires_.append("P2_to_ALUSCRMux");
    active_wires_.append("P2_to_ALUControl");
    active_wires_.append("ALUcontrol_to_ALU");
    active_wires_.append("ALUMux_to_ALU");

    active_wires_.append("P2_to_P3_MEMControl");
    active_wires_.append("P2_to_P3_WBcontrol");
    active_wires_.append("P2_to_ALUcontrol_Control");
    // active_wires_.append("P2_to_ALUMux");

    active_wires_.append("Control_to_P3_up");
    active_wires_.append("Control_to_P3_mid");
    active_wires_.append("Control_to_P3_down");

    active_wires_.append("P2_to_P3_WBcontrol");
    active_wires_.append("P2_to_P3_MEMControl");
    active_wires_.append("P2_to_ALUcontrol_Control");

    active_wires_.append("P2_to_ALU2");

    active_wires_.append("P3_to_P4_WBcontrol");

    active_wires_.append("rd_P2_to_P3");
    active_wires_.append("rd_P3_to_P4");

    always_active_wires_count_ = active_wires_.size();
}

void RV5StageProcessorNHNF::SetActiveWireNames()
{
    // Clear and populate canonical wires for the NH_NF circuit (no forwarding)
    active_wires_.erase(active_wires_.begin() + always_active_wires_count_, active_wires_.end());

    // Backbone / always-visible wires (scene JSON uses these names)

    // Conditional wires based on current pipeline signals
    if (id_ex_reg_.alu_src)
    {
        active_wires_.append("Imm_to_P2");
        active_wires_.append("P2Imm_to_ALU2_down");
        active_wires_.append("P2_to_ALUMux");
    }

    // Memory stage activity (EX/MEM)
    // the intruction executed in this cycle was branch/jal/jalr
    if ((ex_mem_reg_.instruction & 0b1111111) == 0b1100011 ||
        (ex_mem_reg_.instruction & 0b1111111) == 0b1100111 ||
        (ex_mem_reg_.instruction & 0b1111111) == 0b1101111)
    {
        active_wires_.append("ALU_zerores_to_P3");
        active_wires_.append("ALU2_to_P3");
    }
    else
    {
        active_wires_.append("ALU_to_P3");
    }

    if ((mem_wb_reg_.instruction & 0b11111111) == 0b1100011 ||
        (mem_wb_reg_.instruction & 0b11111111) == 0b1100111 ||
        (mem_wb_reg_.instruction & 0b11111111) == 0b1101111)
    {
        active_wires_.append("ANDGate_lower_entry");
        active_wires_.append("P3_to_PCMux");
        // active_wires_.append("ANDGATE_to_PCMUX");
    }
    else
    {
        active_wires_.append("P3ALUres_to_DMup");
    }

    if (ex_mem_reg_.prev_branch_taken)
    {
        active_wires_.append("ANDGATE_to_PCMUX");
        active_wires_.append("P3_to_UpperEntryANDGATE");
    }
    else
    {
        active_wires_.append("ALU1_to_PCMuxUp");
    }

    if (ex_mem_reg_.prev_mem_read)
    {
        active_wires_.append("DM_to_P4");
        active_wires_.append("P3_to_DM_Memread");
    }
    if (ex_mem_reg_.prev_mem_write)
    {
        active_wires_.append("P3_TO_DM_control_memwrite");
        active_wires_.append("P3_rs2_to_DMdown");
        // active_wires_.append("P3ALUres_to_DMup");
    }

    if (mem_wb_reg_.prev_reg_write)
    {
        active_wires_.append("P4_wbcontrol_to_WBmux");
        active_wires_.append("P4_to_RF_regwritecontrol");
        active_wires_.append("WBMux_to_RF");
    }

    if (mem_wb_reg_.prev_mem_to_reg)
    {
        active_wires_.append("P4DM_to_lastMux");
        // active_wires_.append("WBMux_to_RF");
    }
    else if (mem_wb_reg_.prev_reg_write)
    {

        active_wires_.append("P4_ALUres_to_Mux");
        active_wires_.append("RdP4_to_RF");
    }
    // Branch / PC selection wires

    // The scene reads `active_wires_` when callers emit `updateCircuitStateSignal`.
}

// This variant resolves SYSTEM instructions in EX: an ECALL stops the processor and any
// SYSTEM instruction (ECALL or CSR) skips the rest of EX, leaving EX/MEM unchanged.
bool RV5StageProcessorNHNF::handleSystemInExecute()
{
    const uint32_t instruction = id_ex_reg_.instruction;
    if ((instruction & 0b1111111) != 0b1110011)
    {
        return false;
    }
    if (((instruction >> 12) & 0x7) == 0b000) // ECALL
    {
        // RequestStop() would Reset() and wipe the final state; just end the run loop.
        stop_requested_ = true;
        output_status_ = "ECALL_EXIT";
        DumpState("vm_state.json");
    }
    return true;
}
} // namespace Kites
