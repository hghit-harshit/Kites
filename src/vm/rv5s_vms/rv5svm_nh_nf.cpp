/**
 * @file rv5svm_nh_nf.cpp
 * @brief Implementation for the 5-stage pipelined VM (RV5S) in Mode 1: No Hazard Detection, No
 * Forwarding (NH_NF).
 * * Data Hazards (R-R, L-R, R-Store, L-Store) require 2 NOPs (Programmer Responsibility).
 * * Control Hazards (JAL/JALR, B-Type) are AUTOMATICALLY handled by the pipeline.
 * * Control Penalties: JAL/JALR = 1 bubble (EX stage); B-Type = 2 bubbles (MEM stage, 3-cycle
 * penalty).
 * * @author Atharva and Harshit
 */
#include "vm/rv5s_vms/rv5svm_nh_nf.h"
#include "common/instructions.h"
#include "config/config.h"
#include "common/debug_colors.h"
#include "ui/processor/processor_designs/rv5svm_nh_nf_circuit_scene.h"
#include "vm/alu.h"
#include "vm/vm_base.h" // For ImmGenerator, etc.


#include <algorithm>
#include <chrono>
#include <iostream>
#include <thread>
#include <tuple>

namespace Kites
{
// NOP instruction: ADDI x0, x0, 0
constexpr uint32_t NOP = 0x00000013;

// --- VmBase Pure Virtual Method Implementations (Run, DebugRun, Reset, Step) ---
RV5StageVM_NH_NF::RV5StageVM_NH_NF() : RV5StageVM_Base()
{

// Reset components and history
#ifndef DISABLE_GUI
    circuit_scene_ = std::make_unique<Kites::RV5StageVM_NH_NF_CircuitScene>();
    connect(this, &VmBase::updateCircuitStateSignal, circuit_scene_.get(),
            &Kites::RV5StageVM_NH_NF_CircuitScene::updateCircuitState);
    connect(this, &VmBase::vmStateChangedSignal, circuit_scene_.get(),
            &Kites::RV5StageVM_NH_NF_CircuitScene::vmStateChangedSlot);
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

void RV5StageVM_NH_NF::SetActiveWireNames()
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

void RV5StageVM_NH_NF::Reset()
{
    RV5StageVM_Base::Reset();
}

void RV5StageVM_NH_NF::Step()
{
    // Capture PC before potential redirection in EX/MEM stages
    uint64_t old_pc_before_redirect = program_counter_;

    begin_step_delta();

    pipeline_writeback();
    pipeline_memory();
    pipeline_execute();
    pipeline_decode();
    pipeline_fetch();

    uint64_t next_pc = program_counter_;

    // If no redirect happened in EX or MEM, advance sequentially.
    if (next_pc == old_pc_before_redirect)
    {
        next_pc = old_pc_before_redirect + 4;
    }

    // Commit the new PC for the Fetch stage
    program_counter_ = next_pc;
    cycle_s_++;

    finalize_step_delta();
}

void RV5StageVM_NH_NF::pipeline_fetch()
{
    if (program_counter_ < program_size_)
    {
        // Latch the instruction and PC for the next stage (IF/ID register)
        if_id_reg_.instruction = memory_controller_.ReadInstruction(program_counter_);
        if_id_reg_.pc = program_counter_;
    }
    else
    {
        // Once past the end of the program, inject NOPs to drain the pipeline.
        if_id_reg_.reset();
    }
}

void RV5StageVM_NH_NF::pipeline_execute()
{
    uint32_t instruction = id_ex_reg_.instruction;
    uint8_t opcode = instruction & 0b1111111;
    uint8_t funct3 = (instruction >> 12) & 0x7;

    if (opcode == 0b1110011 && funct3 == 0b000)
    {
        handle_syscall();
        return;
    }

    if (opcode == 0b1110011)
    {
        execute_csr();
        return;
    }

    // Select ALU inputs
    uint64_t alu_in1 = id_ex_reg_.reg1_data;
    uint64_t alu_in2 =
        id_ex_reg_.alu_src ? static_cast<uint64_t>(id_ex_reg_.imm) : id_ex_reg_.reg2_data;

    // Get the specific ALU operation
    alu::AluOp alu_operation =
        control_unit_.GetAluSignal(id_ex_reg_.instruction, id_ex_reg_.alu_op > 0);

    // Execute the operation
    bool overflow; // Ignored for this simple model
    uint64_t alu_result = 0;
    const bool is_f_instruction = instruction_set::isFInstruction(instruction);
    const bool is_d_instruction = instruction_set::isDInstruction(instruction);
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
        std::tie(alu_result, overflow) = alu::Alu::execute(alu_operation, alu_in1, alu_in2);
    }
    if ((id_ex_reg_.instruction & 0b1111111) == 0b0110111) // lui
    {
        alu_result = static_cast<uint64_t>(id_ex_reg_.imm << 12);
    }

    // Latch data for EX/MEM Register
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
    ex_mem_reg_.reg2_data = id_ex_reg_.reg2_data;
    ex_mem_reg_.freg2_data = id_ex_reg_.freg2_data;
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

    // --- Conditional Branch Check (B-type: BLT, BGE, etc.) ---
    if (id_ex_reg_.branch && opcode == 0b1100011)
    {
        bool condition_met = false;

        // This fully implements all six branch conditions using the ALU subtraction/comparison
        // result.
        switch (funct3)
        {
        case 0b000:
            condition_met = (alu_result == 0);
            break; // BEQ (Result of Subtraction is Zero)
        case 0b001:
            condition_met = (alu_result != 0);
            break; // BNE (Result of Subtraction is Non-Zero)
        case 0b100:
            condition_met = (alu_result == 1);
            break; // BLT (Result of kSlt is 1)
        case 0b101:
            condition_met = (alu_result == 0);
            break; // BGE (Result of kSlt is 0 - Not Less Than)
        case 0b110:
            condition_met = (alu_result == 1);
            break; // BLTU (Result of kSltu is 1)
        case 0b111:
            condition_met = (alu_result == 0);
            break; // BGEU (Result of kSltu is 0 - Not Less Than Unsigned)
        }

        if (condition_met)
        {
            // Misprediction detected. Flag MEM stage to handle the 3-cycle flush.
            ex_mem_reg_.branch_taken = true;
            ex_mem_reg_.branch_target_pc = id_ex_reg_.pc + id_ex_reg_.imm;
        }
    }
    // --- Unconditional Jump Check (JAL/JALR: 1-cycle penalty) ---
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

        // 1. Redirect PC (Auto-Advance)
        program_counter_ = jump_target;

        // 2. Kill the next instruction (IF/ID register) to incur the 1-bubble penalty.
        if_id_reg_.reset();

        // 3. Mark as taken (for link register write)
        ex_mem_reg_.branch_taken = true;
    }
}

// --- Undo/Redo Implementations ---

void RV5StageVM_NH_NF::handle_syscall()
{
    if ((id_ex_reg_.instruction & 0x7F) == 0b1110011 &&
        ((id_ex_reg_.instruction >> 12) & 0x7) == 0b000)
    {
        RequestStop();
        output_status_ = "ECALL_EXIT";
        DumpState("vm_state.json");
    }
}

// --- FP Execute Handlers ---

uint64_t RV5StageVM_NH_NF::pipeline_execute_float()
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

    alu::AluOp aluOperation = control_unit_.GetAluSignal(instruction, id_ex_reg_.alu_op > 0);
    std::tie(alu_result, fcsr_status) =
        alu::Alu::fpexecute(aluOperation, reg1_value, reg2_value, reg3_value, rm);

    registers_.WriteCsr(0x003, fcsr_status);
    return alu_result;
}

uint64_t RV5StageVM_NH_NF::pipeline_execute_double()
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

    alu::AluOp alu_operation = control_unit_.GetAluSignal(instruction, id_ex_reg_.alu_op > 0);
    std::tie(alu_result, fcsr_status) =
        alu::Alu::dfpexecute(alu_operation, reg1_value, reg2_value, reg3_value, rm);

    registers_.WriteCsr(0x003, fcsr_status);
    return alu_result;
}
}//namespace Kites
