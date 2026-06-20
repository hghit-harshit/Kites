/**
 * @file rv5svm_h_nf.cpp
 * @brief Implementation for the 5-stage pipelined VM (RV5S) in Mode 3: Hazard Detection, No
 * Forwarding.
 * * NOTE: This VM is fully autonomous for all hazards (data and control).
 * * Stall Rule: Any GPR data dependency results in a 2-cycle stall (2 bubbles).
 * * Control Hazards (JAL/JALR, B-Type) are AUTOMATICALLY handled by flush/redirect.
 * @author Atharva and Harshit
 */
#include "vm/rv5s_vms/rv5svm_h_nf.h"
#include "common/instructions.h"
#include "config/config.h"
#include "ui/processor/processor_designs/rv5svm_h_nf_circuit_scene.h"
#include "vm/alu.h"
#include "vm/pipeline_registers.h"
#include "vm/vm_base.h"
#include <algorithm>
#include <chrono>
#include <iostream>
#include <thread>
#include <tuple>

namespace Kites
{
// NOP instruction: ADDI x0, x0, 0
constexpr uint32_t NOP = 0x00000013;

using namespace alu;

// --- RV5StageVM_H_NF Class Implementation ---

RV5StageVM_H_NF::RV5StageVM_H_NF() : RV5StageVM_Base()
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
    circuit_scene_ = std::make_unique<Kites::RV5StageVM_H_NF_CircuitScene>();
    connect(this, &VmBase::updateCircuitStateSignal, circuit_scene_.get(),
            &Kites::RV5StageVM_H_NF_CircuitScene::updateCircuitState);
    connect(this, &VmBase::vmStateChangedSignal, circuit_scene_.get(),
            &Kites::RV5StageVM_H_NF_CircuitScene::vmStateChangedSlot);
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

void RV5StageVM_H_NF::SetActiveWireNames()
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

void RV5StageVM_H_NF::Reset()
{
    RV5StageVM_Base::Reset();
    stall_fetch_and_decode_ = false;
}

void RV5StageVM_H_NF::Step()
{
    uint64_t old_pc_before_redirect = program_counter_;

    begin_step_delta();

    pipeline_writeback();
    pipeline_memory();
    pipeline_execute();

    int stalls_needed = 0;
    bool currently_stalling = (stall_cycles_ > 0);

    if (currently_stalling)
    {
        stall_cycles_--;
    }
    else
    {
        stalls_needed = check_data_hazard(if_id_reg_, id_ex_reg_, ex_mem_reg_, false);
        if (stalls_needed > 0)
        {
            stall_cycles_ = stalls_needed - 1;
        }
    }

    if (currently_stalling || stalls_needed > 0)
    {
        id_ex_reg_.reset();
        stall_fetch_and_decode_ = true;
    }
    else
    {
        pipeline_decode();
        stall_fetch_and_decode_ = false;
    }

    pipeline_fetch();
    uint64_t next_pc = program_counter_;
    if (!stall_fetch_and_decode_ && next_pc == old_pc_before_redirect)
    {
        next_pc = old_pc_before_redirect + 4;
    }

    program_counter_ = next_pc;
    cycle_s_++;

    finalize_step_delta();

    // if (program_counter_ >= program_size_ && id_ex_reg_.instruction == NOP)
    // {
    //     RequestStop();
    // }
}

void RV5StageVM_H_NF::pipeline_fetch()
{
    if (stall_fetch_and_decode_)
    {
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

void RV5StageVM_H_NF::pipeline_execute()
{
    uint32_t cur_instruction = id_ex_reg_.instruction;
    const bool is_f_instruction = instruction_set::isFInstruction(cur_instruction);
    const bool is_d_instruction = instruction_set::isDInstruction(cur_instruction);

    uint64_t alu_in1 = id_ex_reg_.reg1_data;
    uint64_t alu_in2 =
        id_ex_reg_.alu_src ? static_cast<uint64_t>(id_ex_reg_.imm) : id_ex_reg_.reg2_data;

    alu::AluOp alu_operation = control_unit_.GetAluSignal(cur_instruction, id_ex_reg_.alu_op > 0);
    uint64_t alu_result = 0;

    bool overflow;
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
    if ((cur_instruction & 0b1111111) == 0b0110111) // lui
    {
        alu_result = static_cast<uint64_t>(id_ex_reg_.imm << 12);
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

    uint32_t instruction = id_ex_reg_.instruction;
    uint8_t opcode = instruction & 0b1111111;

    if (id_ex_reg_.branch && opcode == 0b1100011)
    {
        std::cout << "Branch instruction detected in EX stage." << std::endl;
        bool condition_met = false;
        uint8_t funct3 = (instruction >> 12) & 0x7;

        switch (funct3)
        {
        case 0b000:
            std::cout << "BEQ check: " << alu_result << std::endl;
            condition_met = (alu_result == 0);
            break;
        case 0b001:
            condition_met = (alu_result != 0);
            break;
        case 0b100:
            condition_met = (alu_result == 1);
            break;
        case 0b101:
            condition_met = (alu_result == 0);
            break;
        case 0b110:
            condition_met = (alu_result == 1);
            break;
        case 0b111:
            condition_met = (alu_result == 0);
            break;
        }

        if (condition_met)
        {
            ex_mem_reg_.branch_taken = true;
            ex_mem_reg_.branch_target_pc = id_ex_reg_.pc + id_ex_reg_.imm;
            // program_counter_ = ex_mem_reg_.branch_target_pc;
            std::cout << "Jump size: " << id_ex_reg_.imm << std::endl;
            std::cout << "Branch taken to PC: 0x" << std::hex << ex_mem_reg_.branch_target_pc
                      << std::dec << std::endl;
        }
    }
    else if (opcode == 0b1101111 || opcode == 0b1100111)
    {
        uint64_t jump_target;
        if (opcode == 0b1101111)
        {
            jump_target = id_ex_reg_.pc + id_ex_reg_.imm;
        }
        else
        {
            jump_target = alu_result & ~1;
            ex_mem_reg_.alu_result = id_ex_reg_.pc + 4;
        }

        program_counter_ = jump_target;
        if_id_reg_.reset();
        ex_mem_reg_.branch_taken = true;
    }
}

void RV5StageVM_H_NF::handle_syscall()
{
    if ((id_ex_reg_.instruction & 0x7F) == 0b1110011 &&
        ((id_ex_reg_.instruction >> 12) & 0x7) == 0b000)
    {
        RequestStop();
        output_status_ = "ECALL_EXIT";
    }
}

uint64_t RV5StageVM_H_NF::pipeline_execute_float()
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

uint64_t RV5StageVM_H_NF::pipeline_execute_double()
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
