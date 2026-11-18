/**
 * @file rv5s_processor_nh_f.cpp
 * @brief Implementation for the 5-stage pipelined VM (RV5S) with Forwarding.
 * * * This mode implements all four necessary forwarding paths.
 * * Data Hazards: Solved by forwarding, EXCEPT for Load-Use/Load-Branch.
 * * Programmer Responsibility: Manually insert ONE NOP after Load→Use or Load→Branch.
 * * Control Hazards: AUTOMATICALLY handled by the pipeline flush mechanism.
 * * @author Atharva and Harshit
 */
#include "processor/rv5s/rv5s_processor_nh_f.h"
#include "common/instructions.h"
#include "config/config.h"
#include "ui/processor_tab/processor_designs/rv5s_processor_nh_f_circuit_scene.h"
#include "processor/alu.h"
#include "processor/processor_base.h" // For ImmGenerator, etc.
#include <algorithm>
#include <chrono>
#include <iostream>
#include <thread>
#include <tuple>

namespace Kites
{
using namespace alu;

// --- VmBase Pure Virtual Method Implementations (Run, DebugRun, Reset, Step) ---
RV5StageProcessorNHF::RV5StageProcessorNHF()
    : RV5StageVM_Base(/*hazardDetection=*/false, /*forwarding=*/true)
{
// Reset components and history
#ifndef DISABLE_GUI
    circuit_scene_ = std::make_unique<Kites::RV5StageVM_NH_F_CircuitScene>();
    connect(this, &ProcessorBase::updateCircuitStateSignal, circuit_scene_.get(),
            &Kites::RV5StageVM_NH_F_CircuitScene::updateCircuitState);
#endif
    Reset();

    // Always-visible / backbone wires in the NH_F circuit
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

    // Pipeline/ALU control and data path wires
    active_wires_.append("P2_to_FMUX");
    active_wires_.append("P2_to_FMUX2_UP");
    active_wires_.append("P2_to_ALUMux");
    active_wires_.append("P2_to_ALUcontrol_Control");

    always_active_wires_count_ = active_wires_.size();
}

void RV5StageProcessorNHF::SetActiveWireNames()
{
    // Clear current dynamic list and populate canonical always-active wires first
    active_wires_.erase(active_wires_.begin() + always_active_wires_count_, active_wires_.end());

    // Note we are checking the singnals are the execution of these imstructions
    //  so all the signals indicated what happened in this cycle
    // for example if mem_wb_reg_.prev_reg_write is true that means in this cycle
    // we executed an instruction that will write to register file in this cycle
    // so we highlight accordingly
    //  Immediate / ALU source selection

    if (id_ex_reg_.alu_src)
    {
        active_wires_.append("Imm_to_P2");
        active_wires_.append("P2Imm_to_ALU2_down");
    }

    if (id_ex_reg_.mem_write)
    {
        active_wires_.append("Control_to_P2_MEM");
    }
    if (id_ex_reg_.reg_write)
    {
        active_wires_.append("Control_to_P2_WB");
    }

    // Memory control / data path (in EX/MEM stage)
    // mem_read  happened is this cycle
    if (ex_mem_reg_.prev_mem_read)
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
    if (mem_wb_reg_.prev_reg_write)
    {
        active_wires_.append("P4_wbcontrol_to_WBmux");
        active_wires_.append("RdP4_to_FU");
        active_wires_.append("WBMux_to_RF");
    }

    // Forwarding unit activity: drive FWD unit wires when forwarding is relevant
    // EX/MEM forwarding (highest priority)
    if (ex_mem_reg_.reg_write && ex_mem_reg_.rd != 0)
    {
        // If EX/MEM destination matches ID/EX sources, show forwarding path
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

    // MEM/WB forwarding (lower priority)
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
    if (ex_mem_reg_.prev_branch_taken)
    {
        active_wires_.append("ALU_zerores_to_P3");
        active_wires_.append("ANDGate_lower_entry");
        active_wires_.append("ANDGATE_to_PCMUX");
        active_wires_.append("P3_to_PCMux");
        // active_wires_.append("ALU1_to_PCMuxUp");
    }
    else
    {
        active_wires_.append("ALU1_to_PCMuxUp");
    }
}
} // namespace Kites
