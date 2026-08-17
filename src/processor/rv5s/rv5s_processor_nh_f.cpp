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
RV5StageProcessorNHF::RV5StageProcessorNHF() : RV5StageVM_Base()
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

void RV5StageProcessorNHF::Reset()
{
    RV5StageVM_Base::Reset();
}

void RV5StageProcessorNHF::Step()
{
    // Capture PC before potential redirection in EX/MEM stages
    uint64_t old_pc_before_redirect = program_counter_;

    begin_step_delta();

    // 1. Execute stages (WB -> MEM -> EX -> ID -> IF)
    pipeline_writeback();
    pipeline_memory();
    pipeline_execute();
    pipeline_decode();
    // Fetch the instruction at the committed PC address.
    pipeline_fetch();
    // 2. Determine the next PC (Redirection logic overrides sequential advance)
    uint64_t next_pc = program_counter_;

    // If no redirect happened in EX or MEM, advance sequentially.
    if (next_pc == old_pc_before_redirect)
    {
        next_pc = old_pc_before_redirect + 4;
    }

    // Commit the new PC for the Fetch stage
    program_counter_ = next_pc;
    cycle_s_++; // One clock cycle has passed

    finalize_step_delta();
}

// --- Pipeline Stage Implementations (Full Proof Control + Forwarding) ---

void RV5StageProcessorNHF::pipeline_fetch()
{
    if (program_counter_ < program_size_)
    {
        // Latch the instruction and PC for the next stage (IF/ID register)
        if_id_reg_.instruction = memory_controller_.readInstruction(program_counter_);
        if_id_reg_.pc = program_counter_;
    }
    else
    {
        // Once past the end of the program, inject NOPs to drain the pipeline.
        if_id_reg_.reset();
    }
}

void RV5StageProcessorNHF::pipeline_execute()
{

    uint32_t instruction = id_ex_reg_.instruction;
    alu::AluOp alu_operation = control_unit_.GetAluSignal(instruction, id_ex_reg_.alu_op > 0);
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
        // ALU execution with forwarding

        // Initial ALU inputs are the stale values read from the register file (ID_EX)
        uint64_t alu_in1 = id_ex_reg_.reg1_data;
        uint64_t alu_in2 = id_ex_reg_.reg2_data;

        // If AluSrc is true (I-type ALU, Load, Store), override reg2_data with the immediate
        if (id_ex_reg_.alu_src)
        {
            alu_in2 = static_cast<uint64_t>(id_ex_reg_.imm);
        }

        // Forwarding values (Source = 00 means no forwarding, use ID_EX value)
        uint8_t forward_a = 0; // 2 = EX/MEM, 1 = MEM/WB
        uint8_t forward_b = 0;

        // --- FORWARDING LOGIC (All non-Load-Use/Branch cases solved here) ---

        // **Priority 1: EX/MEM (Data available from current instruction in MEM)**
        // Covers R->R, R->Branch, R->MemAddr, R->Store (Cases 1, 5, 7, 3)
        if (ex_mem_reg_.reg_write && (ex_mem_reg_.rd != 0))
        {
            if (ex_mem_reg_.rd == id_ex_reg_.rs1)
            {
                forward_a = 2; // Forward A from EX/MEM ALU result
            }
            if (ex_mem_reg_.rd == id_ex_reg_.rs2)
            {
                forward_b = 2; // Forward B from EX/MEM ALU result
            }
        }

        // **Priority 2: MEM/WB (Data available from current instruction in WB)**
        // Used for dependencies two cycles ago, and to forward Load data after the 1 NOP delay.
        if (mem_wb_reg_.prev_reg_write && (mem_wb_reg_.prev_rd != 0))
        {
            // Forward A from MEM/WB unless EX/MEM is forwarding to the same register (Priority)
            if (mem_wb_reg_.prev_rd == id_ex_reg_.rs1 && forward_a != 2)
            {
                forward_a = 1; // Forward A from MEM/WB result (ALU or Memory)
            }
            // Forward B from MEM/WB unless EX/MEM is forwarding to the same register (Priority)
            if (mem_wb_reg_.prev_rd == id_ex_reg_.rs2 && forward_b != 2)
            {
                forward_b = 1; // Forward B from MEM/WB result (ALU or Memory)
            }
        }

        // Forwarding Source A
        if (forward_a == 2)
        { // Forward from EX/MEM
            alu_in1 = ex_mem_reg_.alu_result;
        }
        else if (forward_a == 1)
        { // Forward from MEM/WB
            alu_in1 = mem_wb_reg_.prev_mem_to_reg ? mem_wb_reg_.prev_memory_data
                                                  : mem_wb_reg_.prev_alu_result;
        }

        // Forwarding Source B
        if (forward_b == 2)
        { // Forward from EX/MEM
            alu_in2 = ex_mem_reg_.alu_result;
        }
        else if (forward_b == 1)
        { // Forward from MEM/WB
            alu_in2 = mem_wb_reg_.prev_mem_to_reg ? mem_wb_reg_.prev_memory_data
                                                  : mem_wb_reg_.prev_alu_result;
        }

        // Re-apply immediate value check after forwarding for ALU_B if needed
        if (id_ex_reg_.alu_src && id_ex_reg_.alu_op != 0)
        {
            alu_in2 = static_cast<uint64_t>(id_ex_reg_.imm);
        }
        else if (id_ex_reg_.alu_src && (id_ex_reg_.mem_read || id_ex_reg_.mem_write))
        {
            alu_in2 = id_ex_reg_.alu_src ? static_cast<uint64_t>(id_ex_reg_.imm) : alu_in2;
        }

        // --- EXECUTION ---
        std::tie(alu_result, overflow) = alu::Alu::execute(alu_operation, alu_in1, alu_in2);

        if ((id_ex_reg_.instruction & 0b1111111) == 0b0110111) // lui
        {
            alu_result = static_cast<uint64_t>(id_ex_reg_.imm << 12);
        }

        // Latch data for EX/MEM Register

        // CRITICAL: The data to be stored (reg2_data for Store) must ALSO be forwarded!
        uint64_t store_data = id_ex_reg_.reg2_data;
        if (forward_b == 2)
        { // Forward from EX/MEM
            store_data = ex_mem_reg_.alu_result;
        }
        else if (forward_b == 1)
        { // Forward from MEM/WB
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

    // --- Conditional Branch Check (B-type) ---
    if (id_ex_reg_.branch && opcode == 0b1100011)
    {
        bool condition_met = false;
        uint8_t funct3 = (instruction >> 12) & 0x7;

        // Comparison check based on ALU result
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

        program_counter_ = jump_target;
        if_id_reg_.reset();
        ex_mem_reg_.branch_taken = true;
    }
}

uint64_t RV5StageProcessorNHF::pipeline_execute_float()
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

uint64_t RV5StageProcessorNHF::pipeline_execute_double()
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
        // two casts are necessary here the inner one extends the sign
        //  and then we cast it back to uint64_t
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

void RV5StageProcessorNHF::handle_syscall()
{
    if ((id_ex_reg_.instruction & 0x7F) == 0b1110011 &&
        ((id_ex_reg_.instruction >> 12) & 0x7) == 0b000)
    {
        RequestStop();
        output_status_ = "ECALL_EXIT";
        DumpState("vm_state.json");
    }
}
}//namespace Kites
