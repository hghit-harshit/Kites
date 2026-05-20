/**
 * @file rv5svm_h_f.cpp
 * @brief Implementation for the 5-stage pipelined VM (RV5S) in Mode 4: Hazard Detection, With
 * Forwarding.
 * * NOTE: This is a PURE INTEGER (GPR) implementation.
 * * Stall Rule: Only Load-Use GPR dependencies require a 1-cycle stall. All others are solved by
 * forwarding.
 * * Control Hazards (JAL/JALR, B-Type) are AUTOMATICALLY handled by flush/redirect.
 * @author Atharva and Harshit
 */
#include "vm/rv5s_vms/rv5svm_h_f.h" // Assuming this header now defines RV5StageVM_H_F
#include "common/instructions.h"
#include "config.h"
#include "ui/processor_designs/rv5svm_h_f_circuit_scene.h"
#include "vm/alu.h"
#include "vm/pipeline_registers.h"
#include "vm/vm_base.h"
#include <algorithm>
#include <chrono>
#include <iostream>
#include <thread>
#include <tuple>


// NOP instruction: ADDI x0, x0, 0 (Used for flushing)
constexpr uint32_t NOP = 0x00000013;

using namespace alu;

// Define stall count constants (used internally, defined in rv5s_hdu.h)
// constexpr int STALL_NONE = 0;
// constexpr int STALL_ONE_CYCLE = 1;

// --- RV5StageVM_H_F Class Implementation ---

RV5StageVM_H_F::RV5StageVM_H_F() : RV5StageVM_Base()
{
    // Initialize VmBase members
    // program_counter_ = 0;
    // instructions_retired_ = 0;
    // cycle_s_ = 0;
    // stall_cycles_ = 0;

    // // Initialize local members
    // stall_fetch_and_decode_ = false;

    // Reset components and history
#ifndef DISABLE_GUI
    circuit_scene_ = std::make_unique<Kites::RV5StageVM_H_F_CircuitScene>();
    connect(this, &VmBase::updateCircuitStateSignal, circuit_scene_.get(),
            &Kites::RV5StageVM_H_F_CircuitScene::updateCircuitState);
    connect(this, &VmBase::vmStateChangedSignal, circuit_scene_.get(),
            &Kites::RV5StageVM_H_F_CircuitScene::vmStateChangedSlot);
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

void RV5StageVM_H_F::SetActiveWireNames()
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

void RV5StageVM_H_F::Reset()
{
    RV5StageVM_Base::Reset();
    stall_fetch_and_decode_ = false;
}

void RV5StageVM_H_F::Step()
{
    // Capture PC before potential redirection in EX/MEM stages
    uint64_t old_pc_before_redirect = program_counter_;

    begin_step_delta();

    // 1. Execute back stages (WB -> MEM -> EX)
    pipeline_writeback();
    pipeline_memory();
    pipeline_execute();

    // --- Hazard Detection and Stall Logic (The core of Mode 4) ---

    int stalls_needed = 0;
    bool currently_stalling = (stall_cycles_ > 0);

    if (currently_stalling)
    {
        // Stall is already active. Decrement counter and continue stall.
        stall_cycles_--;
        stalls_needed = 1; // Indicate we are continuing the stall (1 cycle consumed)
    }
    else
    {
        // Pipeline is free. Check for a NEW hazard using the external HDU function.
        stalls_needed = check_data_hazard(if_id_reg_, id_ex_reg_, ex_mem_reg_,
                                          true /* is_forwarding_enabled */);
        if (stalls_needed > 0)
        {
            // Start the stall: stalls_needed cycles total. 1 cycle is handled now.
            stall_cycles_ = stalls_needed - 1;
        }
    }

    // 2. Process Decode/Fetch based on stall status
    if (stalls_needed > 0)
    {
        // STALL: Freeze IF/ID (by preventing its update) and inject NOP into ID/EX
        id_ex_reg_.reset();
        // Skip pipeline_decode() - The instruction in IF/ID stays put.
        stall_fetch_and_decode_ = true;
    }
    else
    {
        // NO STALL: Advance pipeline normally
        pipeline_decode();
        stall_fetch_and_decode_ = false;
    }

    // Fetch the instruction at the committed PC address.
    pipeline_fetch();
    // 3. PC Update and Fetch
    uint64_t next_pc = program_counter_;

    // Only advance the PC if we were not stalling this cycle (and thus fetched an instruction).
    // Note: Control hazards (JAL/Branch) already override program_counter_ in EX/MEM.
    if (!stall_fetch_and_decode_ && next_pc == old_pc_before_redirect)
    {
        next_pc = old_pc_before_redirect + 4;
    }

    // Commit the new PC for the Fetch stage
    program_counter_ = next_pc;

    cycle_s_++; // One clock cycle has passed

    finalize_step_delta();
}

// --- Pipeline Stage Implementations (Mode 4: H_F) ---

void RV5StageVM_H_F::pipeline_fetch()
{
    // If a stall is active, we prevent IF/ID from updating.
    if (stall_fetch_and_decode_)
    {
        // IF/ID register is intentionally NOT updated, holding the stalled instruction.
        return;
    }

    if (program_counter_ < program_size_)
    {
        if_id_reg_.instruction = memory_controller_.ReadInstruction(program_counter_);
        if_id_reg_.pc = program_counter_;
    }
    else
    {
        if_id_reg_.reset();
    }
}

void RV5StageVM_H_F::pipeline_execute()
{
    uint32_t instruction = id_ex_reg_.instruction;
    bool overflow;
    uint64_t alu_result;

    bool is_f_instruction = instruction_set::isFInstruction(instruction);
    bool is_d_instruction = instruction_set::isDInstruction(instruction);

    if (is_f_instruction)
    {
        alu_result = pipeline_execute_float();
    }
    else if (is_d_instruction)
    {
        alu_result = pipeline_execute_double();
    }
    else
    {
        uint64_t alu_in1 = id_ex_reg_.reg1_data;
        uint64_t alu_in2 = id_ex_reg_.reg2_data;

        // --- GPR FORWARDING LOGIC ---
        uint8_t forward_a = 0; // 10 = EX/MEM, 01 = MEM/WB
        uint8_t forward_b = 0;

        // **Priority 1: EX/MEM Forward** (R-R, R-Store hazard solved)
        if (ex_mem_reg_.reg_write && (ex_mem_reg_.rd != 0))
        {
            if (ex_mem_reg_.rd == id_ex_reg_.rs1)
                forward_a = 2;
            if (ex_mem_reg_.rd == id_ex_reg_.rs2)
                forward_b = 2;
        }

        // **Priority 2: MEM/WB Forward** (R-R, L-Store hazard solved)
        if (mem_wb_reg_.prev_reg_write && (mem_wb_reg_.prev_rd != 0))
        {
            // Forward A from MEM/WB unless EX/MEM is already forwarding
            if (mem_wb_reg_.prev_rd == id_ex_reg_.rs1 && forward_a != 2)
                forward_a = 1;
            // Forward B from MEM/WB unless EX/MEM is already forwarding
            if (mem_wb_reg_.prev_rd == id_ex_reg_.rs2 && forward_b != 2)
                forward_b = 1;
        }

        // --- FORWARDING APPLICATION ---

        if (forward_a == 2)
        {
            alu_in1 = ex_mem_reg_.alu_result;
        }
        else if (forward_a == 1)
        {
            alu_in1 = mem_wb_reg_.prev_mem_to_reg ? mem_wb_reg_.prev_memory_data
                                                  : mem_wb_reg_.prev_alu_result;
        }

        if (forward_b == 2)
        {
            alu_in2 = ex_mem_reg_.alu_result;
        }
        else if (forward_b == 1)
        {
            alu_in2 = mem_wb_reg_.prev_mem_to_reg ? mem_wb_reg_.prev_memory_data
                                                  : mem_wb_reg_.prev_alu_result;
        }

        // Re-apply immediate value check (must happen AFTER forwarding application)
        if (id_ex_reg_.alu_src)
        {
            alu_in2 = static_cast<uint64_t>(id_ex_reg_.imm);
        }

        // --- EXECUTION ---
        alu::AluOp alu_operation = control_unit_.GetAluSignal(instruction, id_ex_reg_.alu_op > 0);
        std::tie(alu_result, overflow) = alu::Alu::execute(alu_operation, alu_in1, alu_in2);
        // temporary fix for lui instruction

        if ((id_ex_reg_.instruction & 0b1111111) == 0b0110111) // lui
        {
            alu_result = static_cast<uint64_t>(id_ex_reg_.imm << 12);
        }

        // Latch data for EX/MEM Register

        // Store Data: CRITICAL FORWARDING - Store data must also be forwarded!
        uint64_t store_data = id_ex_reg_.reg2_data;
        if (forward_b == 2)
        {
            store_data = ex_mem_reg_.alu_result;
        }
        else if (forward_b == 1)
        {
            store_data = mem_wb_reg_.prev_mem_to_reg ? mem_wb_reg_.prev_memory_data
                                                     : mem_wb_reg_.prev_alu_result;
        }

        ex_mem_reg_.reg2_data = store_data;
    }

    ex_mem_reg_.prev_reg_write = ex_mem_reg_.reg_write;
    ex_mem_reg_.prev_rd = ex_mem_reg_.rd;
    ex_mem_reg_.prev_freg_write = ex_mem_reg_.freg_write;
    ex_mem_reg_.prev_frd = ex_mem_reg_.frd;
    ex_mem_reg_.pc = id_ex_reg_.pc;
    ex_mem_reg_.instruction = id_ex_reg_.instruction;
    ex_mem_reg_.alu_result = alu_result;
    ex_mem_reg_.f_alu_result = (is_f_instruction || is_d_instruction) ? alu_result : 0;
    ex_mem_reg_.rd = id_ex_reg_.rd;
    ex_mem_reg_.frd = id_ex_reg_.frd;
    // NOTE: reg2_data and freg2_data are set inside the integer/FP execute
    // branches above with proper forwarding applied — do NOT overwrite here.
    ex_mem_reg_.reg_write = id_ex_reg_.reg_write;
    ex_mem_reg_.freg_write = id_ex_reg_.freg_write;
    ex_mem_reg_.mem_to_reg = id_ex_reg_.mem_to_reg;
    ex_mem_reg_.prev_mem_read = ex_mem_reg_.mem_read;
    ex_mem_reg_.prev_mem_write = ex_mem_reg_.mem_write;
    ex_mem_reg_.mem_read = id_ex_reg_.mem_read;
    ex_mem_reg_.mem_write = id_ex_reg_.mem_write;
    ex_mem_reg_.prev_branch_taken = ex_mem_reg_.branch_taken;
    ex_mem_reg_.branch_taken = false;
    ex_mem_reg_.branch_target_pc = 0;

    uint8_t opcode = instruction & 0b1111111;

    // --- Control Hazard Logic (Automated) ---

    // Conditional Branch Check (B-type)
    if (id_ex_reg_.branch && opcode == 0b1100011)
    {
        bool condition_met = false;
        uint8_t funct3 = (instruction >> 12) & 0x7;

        switch (funct3)
        {
        case 0b000:
            condition_met = (alu_result == 0);
            break; // BEQ
        case 0b001:
            condition_met = (alu_result != 0);
            break; // BNE
        case 0b100:
            condition_met = (alu_result == 1);
            break; // BLT
        case 0b101:
            condition_met = (alu_result == 0);
            break; // BGE
        case 0b110:
            condition_met = (alu_result == 1);
            break; // BLTU
        case 0b111:
            condition_met = (alu_result == 0);
            break; // BGEU
        }

        if (condition_met)
        {
            ex_mem_reg_.branch_taken = true;
            ex_mem_reg_.branch_target_pc = id_ex_reg_.pc + id_ex_reg_.imm;
        }
    }
    // Unconditional Jump Check (JAL/JALR: 1-cycle penalty)
    else if (opcode == 0b1101111 || opcode == 0b1100111)
    {
        uint64_t jump_target;
        if (opcode == 0b1101111)
        {
            jump_target = id_ex_reg_.pc + id_ex_reg_.imm; // JAL
        }
        else
        {
            jump_target = alu_result & ~1;              // JALR (ALU result is Reg + Imm)
            ex_mem_reg_.alu_result = id_ex_reg_.pc + 4; // Set link address (PC+4)
        }

        program_counter_ = jump_target;
        if_id_reg_.reset();
        ex_mem_reg_.branch_taken = true;
    }
}

void RV5StageVM_H_F::handle_syscall()
{
    if ((id_ex_reg_.instruction & 0x7F) == 0b1110011 &&
        ((id_ex_reg_.instruction >> 12) & 0x7) == 0b000)
    {
        RequestStop();
        output_status_ = "ECALL_EXIT";
        // DumpState("vm_state.json");
    }
}

uint64_t RV5StageVM_H_F::pipeline_execute_float()
{
    uint32_t instruction = id_ex_reg_.instruction;
    uint8_t opcode = instruction & 0b1111111;
    uint8_t funct3 = (instruction >> 12) & 0b111;
    uint8_t funct7 = (instruction >> 25) & 0b1111111;
    uint8_t rm = funct3;

    uint8_t fcsr_status = 0;
    uint64_t alu_result = 0;

    if (rm == 0b111)
    {
        rm = registers_.ReadCsr(0x002);
    }

    uint64_t reg1_value = id_ex_reg_.freg1_data;
    uint64_t reg2_value = id_ex_reg_.freg2_data;
    uint64_t reg3_value = id_ex_reg_.freg3_data;

    if (funct7 == 0b1101000 || funct7 == 0b1111000 || opcode == 0b0000111 || opcode == 0b0100111)
    {
        reg1_value = registers_.ReadGpr(id_ex_reg_.rs1);
    }

    if (id_ex_reg_.alu_src)
    {
        reg2_value = static_cast<uint64_t>(static_cast<int64_t>(id_ex_reg_.imm));
    }

    // --- FORWARDING for FPR sources (frs1/frs2/frs3) ---
    uint8_t fwd_a = 0; // 2 = EX/MEM, 1 = MEM/WB
    uint8_t fwd_b = 0;
    uint8_t fwd_c = 0;

    // EX/MEM forwarding (highest priority)
    if (ex_mem_reg_.freg_write && (ex_mem_reg_.frd != 0))
    {
        if (ex_mem_reg_.frd == id_ex_reg_.frs1)
            fwd_a = 2;
        if (ex_mem_reg_.frd == id_ex_reg_.frs2)
            fwd_b = 2;
        if (ex_mem_reg_.frd == id_ex_reg_.frs3)
            fwd_c = 2;
    }

    // MEM/WB forwarding (lower priority)
    if (mem_wb_reg_.prev_freg_write && (mem_wb_reg_.prev_frd != 0))
    {
        if (mem_wb_reg_.prev_frd == id_ex_reg_.frs1 && fwd_a != 2)
            fwd_a = 1;
        if (mem_wb_reg_.prev_frd == id_ex_reg_.frs2 && fwd_b != 2)
            fwd_b = 1;
        if (mem_wb_reg_.prev_frd == id_ex_reg_.frs3 && fwd_c != 2)
            fwd_c = 1;
    }

    // Apply forwarding to source values
    if (fwd_a == 2)
        reg1_value = ex_mem_reg_.f_alu_result;
    else if (fwd_a == 1)
        reg1_value = mem_wb_reg_.prev_mem_to_reg ? mem_wb_reg_.prev_f_memory_data
                                                 : mem_wb_reg_.prev_f_alu_result;

    if (fwd_b == 2)
        reg2_value = ex_mem_reg_.f_alu_result;
    else if (fwd_b == 1)
        reg2_value = mem_wb_reg_.prev_mem_to_reg ? mem_wb_reg_.prev_f_memory_data
                                                 : mem_wb_reg_.prev_f_alu_result;

    if (fwd_c == 2)
        reg3_value = ex_mem_reg_.f_alu_result;
    else if (fwd_c == 1)
        reg3_value = mem_wb_reg_.prev_mem_to_reg ? mem_wb_reg_.prev_f_memory_data
                                                 : mem_wb_reg_.prev_f_alu_result;

    // Re-apply GPR-read override (for conversion ops) after forwarding decision: if instruction
    // requires GPR, use it.
    if (funct7 == 0b1101000 || funct7 == 0b1111000 || opcode == 0b0000111 || opcode == 0b0100111)
    {
        reg1_value = registers_.ReadGpr(id_ex_reg_.rs1);
    }

    // Re-apply immediate if ALUSrc was set (immediates take precedence for reg2)
    if (id_ex_reg_.alu_src)
    {
        reg2_value = static_cast<uint64_t>(static_cast<int64_t>(id_ex_reg_.imm));
    }

    // Forward store data for FPR stores (frs2)
    uint64_t fstore_data = id_ex_reg_.freg2_data;
    if (fwd_b == 2)
        fstore_data = ex_mem_reg_.f_alu_result;
    else if (fwd_b == 1)
        fstore_data = mem_wb_reg_.prev_mem_to_reg ? mem_wb_reg_.prev_f_memory_data
                                                  : mem_wb_reg_.prev_f_alu_result;

    ex_mem_reg_.freg2_data = fstore_data;
    // Execute FP operation
    alu::AluOp aluOperation = control_unit_.GetAluSignal(instruction, id_ex_reg_.alu_op > 0);
    std::tie(alu_result, fcsr_status) =
        alu::Alu::fpexecute(aluOperation, reg1_value, reg2_value, reg3_value, rm);

    registers_.WriteCsr(0x003, fcsr_status);

    return alu_result;
}

uint64_t RV5StageVM_H_F::pipeline_execute_double()
{
    uint32_t instruction = id_ex_reg_.instruction;
    uint8_t opcode = instruction & 0b1111111;
    uint8_t funct3 = (instruction >> 12) & 0b111;
    uint8_t funct7 = (instruction >> 25) & 0b1111111;
    uint8_t rm = funct3;

    uint8_t fcsr_status = 0;
    uint64_t alu_result = 0;

    if (rm == 0b111)
    {
        rm = registers_.ReadCsr(0x002);
    }

    uint64_t reg1_value = id_ex_reg_.freg1_data;
    uint64_t reg2_value = id_ex_reg_.freg2_data;
    uint64_t reg3_value = id_ex_reg_.freg3_data;

    if (funct7 == 0b1101001 || funct7 == 0b1111001 || opcode == 0b0000111 || opcode == 0b0100111)
    {
        reg1_value = registers_.ReadGpr(id_ex_reg_.rs1);
    }

    if (id_ex_reg_.alu_src)
    {
        reg2_value = static_cast<uint64_t>(static_cast<int64_t>(id_ex_reg_.imm));
    }

    // --- FORWARDING for FPR sources (frs1/frs2/frs3) ---
    uint8_t fwd_a = 0; // 2 = EX/MEM, 1 = MEM/WB
    uint8_t fwd_b = 0;
    uint8_t fwd_c = 0;

    if (ex_mem_reg_.freg_write && (ex_mem_reg_.frd != 0))
    {
        if (ex_mem_reg_.frd == id_ex_reg_.frs1)
            fwd_a = 2;
        if (ex_mem_reg_.frd == id_ex_reg_.frs2)
            fwd_b = 2;
        if (ex_mem_reg_.frd == id_ex_reg_.frs3)
            fwd_c = 2;
    }

    if (mem_wb_reg_.prev_freg_write && (mem_wb_reg_.prev_frd != 0))
    {
        if (mem_wb_reg_.prev_frd == id_ex_reg_.frs1 && fwd_a != 2)
            fwd_a = 1;
        if (mem_wb_reg_.prev_frd == id_ex_reg_.frs2 && fwd_b != 2)
            fwd_b = 1;
        if (mem_wb_reg_.prev_frd == id_ex_reg_.frs3 && fwd_c != 2)
            fwd_c = 1;
    }

    if (fwd_a == 2)
        reg1_value = ex_mem_reg_.f_alu_result;
    else if (fwd_a == 1)
        reg1_value = mem_wb_reg_.prev_mem_to_reg ? mem_wb_reg_.prev_f_memory_data
                                                 : mem_wb_reg_.prev_f_alu_result;

    if (fwd_b == 2)
        reg2_value = ex_mem_reg_.f_alu_result;
    else if (fwd_b == 1)
        reg2_value = mem_wb_reg_.prev_mem_to_reg ? mem_wb_reg_.prev_f_memory_data
                                                 : mem_wb_reg_.prev_f_alu_result;

    if (fwd_c == 2)
        reg3_value = ex_mem_reg_.f_alu_result;
    else if (fwd_c == 1)
        reg3_value = mem_wb_reg_.prev_mem_to_reg ? mem_wb_reg_.prev_f_memory_data
                                                 : mem_wb_reg_.prev_f_alu_result;

    // Re-apply GPR-read override for specific instructions
    if (funct7 == 0b1101001 || funct7 == 0b1111001 || opcode == 0b0000111 || opcode == 0b0100111)
    {
        reg1_value = registers_.ReadGpr(id_ex_reg_.rs1);
    }

    // Re-apply immediate for reg2
    if (id_ex_reg_.alu_src)
    {
        reg2_value = static_cast<uint64_t>(static_cast<int64_t>(id_ex_reg_.imm));
    }

    // Forward store data for FPR stores
    uint64_t fstore_data = id_ex_reg_.freg2_data;
    if (fwd_b == 2)
        fstore_data = ex_mem_reg_.f_alu_result;
    else if (fwd_b == 1)
        fstore_data = mem_wb_reg_.prev_mem_to_reg ? mem_wb_reg_.prev_f_memory_data
                                                  : mem_wb_reg_.prev_f_alu_result;

    ex_mem_reg_.freg2_data = fstore_data;

    alu::AluOp alu_operation = control_unit_.GetAluSignal(instruction, id_ex_reg_.alu_op > 0);
    std::tie(alu_result, fcsr_status) =
        alu::Alu::dfpexecute(alu_operation, reg1_value, reg2_value, reg3_value, rm);

    registers_.WriteCsr(0x003, fcsr_status);

    return alu_result;
}
