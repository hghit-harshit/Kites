/**
 * @file rv5svm_h_nf.cpp
 * @brief Implementation for the 5-stage pipelined VM (RV5S) in Mode 3: Hazard Detection, No Forwarding.
 * * NOTE: This VM is fully autonomous for all hazards (data and control).
 * * Stall Rule: Any GPR data dependency results in a 2-cycle stall (2 bubbles).
 * * Control Hazards (JAL/JALR, B-Type) are AUTOMATICALLY handled by flush/redirect.
 * @author Atharva and Harshit
 */
#include "vm/rv5s_vms/rv5svm_h_nf.h"
#include "common/instructions.h"
#include "config.h"
#include "vm/alu.h"
#include "vm/vm_base.h"
#include "vm/pipeline_registers.h"
#include "ui/processor_designs/rv5svm_h_nf_circuit_scene.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <tuple>
#include <algorithm>

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
    circuit_scene_ = std::make_unique<Kites::RV5StageVM_H_NF_CircuitScene>();
    connect(this, &VmBase::updateCircuitStateSignal,
            circuit_scene_.get(), &Kites::RV5StageVM_H_NF_CircuitScene::updateCircuitState);
    Reset();
}

void RV5StageVM_H_NF::Run()
{
    ClearStop();
    while (!stop_requested_ && (program_counter_ < program_size_ || !is_pipeline_drained()))
    {
        Step();
    }
}

void RV5StageVM_H_NF::DebugRun()
{
    ClearStop();
    while (!stop_requested_ && (program_counter_ < program_size_ || !is_pipeline_drained()))
    {
        // if (CheckBreakpoint(program_counter_))
        // {
        //     std::cout << "VM_BREAKPOINT_HIT " << program_counter_ << std::endl;
        //     output_status_ = "VM_BREAKPOINT_HIT";
        //     break;
        // }
        print_pipeline_registers_debug();
        Step();
        std::cout << "Cycle: " << cycle_s_ << " | PC: 0x" << std::hex << program_counter_ << std::dec << std::endl;
    }
    print_pipeline_registers_debug();
}

void RV5StageVM_H_NF::Reset()
{
    program_counter_ = 0;
    instructions_retired_ = 0;
    cycle_s_ = 0;
    stop_requested_ = false;
    stall_fetch_and_decode_ = false;
    stall_cycles_ = 0; // Initialize stall counter

    registers_.Reset();
    memory_controller_.Reset();
    control_unit_.Reset();

    if_id_reg_.reset();
    id_ex_reg_.reset();
    ex_mem_reg_.reset();
    mem_wb_reg_.reset();

    current_delta_ = StepDelta();
    while (!undo_stack_.empty())
        undo_stack_.pop();
    while (!redo_stack_.empty())
        redo_stack_.pop();
}

void RV5StageVM_H_NF::Step()
{
    uint64_t old_pc_before_redirect = program_counter_;

    current_delta_ = StepDelta();
    current_delta_.old_pc = program_counter_;

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

    current_delta_.new_pc = program_counter_;

    if (!current_delta_.register_changes.empty() || !current_delta_.memory_changes.empty())
    {
        undo_stack_.push(current_delta_);
        while (!redo_stack_.empty())
        {
            redo_stack_.pop();
        }
    }

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
        if_id_reg_.instruction = memory_controller_.ReadWord(program_counter_);
        if_id_reg_.pc = program_counter_;
    }
    else
    {
        if_id_reg_.reset();
    }
}

void RV5StageVM_H_NF::pipeline_decode()
{
    uint32_t instruction = if_id_reg_.instruction;
    if (instruction == NOP) {
        // Pass through fields as needed
        id_ex_reg_.pc = if_id_reg_.pc;
        id_ex_reg_.instruction = instruction;
        id_ex_reg_.imm = 0;
        id_ex_reg_.rs1 = id_ex_reg_.rs2 = id_ex_reg_.rd = 0;
        id_ex_reg_.reg1_data = 0;
        id_ex_reg_.reg2_data = 0;

        // Critically: zero *all* control signals so downstream stages are idle
        id_ex_reg_.reg_write = false;
        id_ex_reg_.branch    = false;
        id_ex_reg_.alu_src   = false;
        id_ex_reg_.mem_read  = false;
        id_ex_reg_.mem_write = false;
        id_ex_reg_.mem_to_reg= false;
        id_ex_reg_.alu_op    = 0;
        return;
    }
    control_unit_.SetControlSignals(instruction);

    id_ex_reg_.pc = if_id_reg_.pc;
    id_ex_reg_.instruction = instruction;
    id_ex_reg_.imm = ImmGenerator(instruction);

    id_ex_reg_.rs1 = (instruction >> 15) & 0x1F;
    id_ex_reg_.rs2 = (instruction >> 20) & 0x1F;
    id_ex_reg_.rd = (instruction >> 7) & 0x1F;

    id_ex_reg_.reg1_data = registers_.ReadGpr(id_ex_reg_.rs1);
    id_ex_reg_.reg2_data = registers_.ReadGpr(id_ex_reg_.rs2);

    id_ex_reg_.reg_write = control_unit_.GetRegWrite();
    id_ex_reg_.branch = control_unit_.GetBranch();
    id_ex_reg_.alu_src = control_unit_.GetAluSrc();
    id_ex_reg_.mem_read = control_unit_.GetMemRead();
    id_ex_reg_.mem_write = control_unit_.GetMemWrite();
    id_ex_reg_.mem_to_reg = control_unit_.GetMemToReg();
    id_ex_reg_.alu_op = control_unit_.GetAluOp();
}

void RV5StageVM_H_NF::pipeline_execute()
{
    uint64_t alu_in1 = id_ex_reg_.reg1_data;
    uint64_t alu_in2 = id_ex_reg_.alu_src ? static_cast<uint64_t>(id_ex_reg_.imm) : id_ex_reg_.reg2_data;

    alu::AluOp alu_operation = control_unit_.GetAluSignal(id_ex_reg_.instruction, id_ex_reg_.alu_op > 0);
    uint64_t alu_result = 0;

    bool overflow;
    std::tie(alu_result, overflow) = alu::Alu::execute(alu_operation, alu_in1, alu_in2);

    ex_mem_reg_.prev_reg_write = ex_mem_reg_.reg_write;
    ex_mem_reg_.prev_rd = ex_mem_reg_.rd; 
    ex_mem_reg_.instruction = id_ex_reg_.instruction;
    ex_mem_reg_.alu_result = alu_result;
    ex_mem_reg_.rd = id_ex_reg_.rd;
    ex_mem_reg_.reg2_data = id_ex_reg_.reg2_data;

    ex_mem_reg_.reg_write = id_ex_reg_.reg_write;
    ex_mem_reg_.mem_to_reg = id_ex_reg_.mem_to_reg;
    ex_mem_reg_.mem_read = id_ex_reg_.mem_read;
    ex_mem_reg_.mem_write = id_ex_reg_.mem_write;
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
            //program_counter_ = ex_mem_reg_.branch_target_pc;
            std::cout << "Jump size: " << id_ex_reg_.imm << std::endl;
            std::cout << "Branch taken to PC: 0x" << std::hex << ex_mem_reg_.branch_target_pc << std::dec << std::endl;
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

void RV5StageVM_H_NF::pipeline_memory()
{
    if (ex_mem_reg_.branch_taken && (ex_mem_reg_.instruction & 0b1111111) == 0b1100011)
    {
        std::cout <<"branch branch branch branch branch branch branch branch branch\n";
        program_counter_ = ex_mem_reg_.branch_target_pc;
        if_id_reg_.reset();
        id_ex_reg_.reset();
        branch_mispredictions_++;
    }

    
    mem_wb_reg_.instruction = ex_mem_reg_.instruction;
    mem_wb_reg_.alu_result = ex_mem_reg_.alu_result;
    mem_wb_reg_.rd = ex_mem_reg_.rd;
    mem_wb_reg_.reg_write = ex_mem_reg_.reg_write;
    mem_wb_reg_.mem_to_reg = ex_mem_reg_.mem_to_reg;

    if (ex_mem_reg_.mem_read)
    {
        uint64_t data = memory_controller_.ReadDoubleWord(ex_mem_reg_.alu_result);
        mem_wb_reg_.memory_data = data;
    }
    else if (ex_mem_reg_.mem_write)
    {
        memory_controller_.WriteDoubleWord(ex_mem_reg_.alu_result, ex_mem_reg_.reg2_data);
    }
}

void RV5StageVM_H_NF::pipeline_writeback()
{
    if (mem_wb_reg_.reg_write && mem_wb_reg_.rd != 0)
    {
        uint64_t write_data = mem_wb_reg_.mem_to_reg ? mem_wb_reg_.memory_data : mem_wb_reg_.alu_result;

        uint64_t old_value = registers_.ReadGpr(mem_wb_reg_.rd);
        if (old_value != write_data)
        {
            current_delta_.register_changes.push_back({mem_wb_reg_.rd,
                                                       0,
                                                       old_value,
                                                       write_data});
        }

        registers_.WriteGpr(mem_wb_reg_.rd, write_data);
        instructions_retired_++;
    }
}

void RV5StageVM_H_NF::Undo()
{
    if (undo_stack_.empty())
    {
        std::cout << "VM_NO_MORE_UNDO" << std::endl;
        return;
    }

    StepDelta last = undo_stack_.top();
    undo_stack_.pop();

    for (const auto &change : last.register_changes)
    {
        if (change.reg_type == 0)
            registers_.WriteGpr(change.reg_index, change.old_value);
    }

    for (const auto &change : last.memory_changes)
    {
        for (size_t i = 0; i < change.old_bytes_vec.size(); ++i)
        {
            memory_controller_.WriteByte(change.address + i, change.old_bytes_vec[i]);
        }
    }

    program_counter_ = last.old_pc;

    if_id_reg_.reset();
    id_ex_reg_.reset();
    ex_mem_reg_.reset();
    mem_wb_reg_.reset();

    redo_stack_.push(last);
    std::cout << "VM_UNDO_COMPLETED" << std::endl;
}

void RV5StageVM_H_NF::Redo()
{
    if (redo_stack_.empty())
    {
        std::cout << "VM_NO_MORE_REDO" << std::endl;
        return;
    }

    StepDelta next = redo_stack_.top();
    redo_stack_.pop();

    for (const auto &change : next.register_changes)
    {
        if (change.reg_type == 0)
            registers_.WriteGpr(change.reg_index, change.new_value);
    }

    for (const auto &change : next.memory_changes)
    {
        for (size_t i = 0; i < change.new_bytes_vec.size(); ++i)
        {
            memory_controller_.WriteByte(change.address + i, change.new_bytes_vec[i]);
        }
    }

    program_counter_ = next.new_pc;

    if_id_reg_.reset();
    id_ex_reg_.reset();
    ex_mem_reg_.reset();
    mem_wb_reg_.reset();

    undo_stack_.push(next);
    std::cout << "VM_REDO_COMPLETED" << std::endl;
}

// void RV5StageVM_H_NF::print_pipeline_registers_debug()
// {
//     std::cout << "--- Pipeline Debug (Cycle " << cycle_s_ << ") ---" << std::endl;
//     std::cout << "PC: 0x" << std::hex << program_counter_ << std::dec << std::endl;

//     auto inst_to_mnemonic = [](uint32_t inst) -> const char *
//     {
//         if (inst == NOP)
//             return "NOP";
//         uint8_t opc = inst & 0x7F;
//         switch (opc)
//         {
//         case 0b1101111:
//             return "JAL";
//         case 0b1100111:
//             return "JALR";
//         case 0b1100011:
//             return "BR";
//         case 0b0000011:
//             return "LOAD";
//         case 0b0100011:
//             return "STORE";
//         case 0b0010011:
//             return "ALU_IMM";
//         case 0b0110011:
//             return "ALU_REG";
//         case 0b1110011:
//             return "SYSTEM";
//         default:
//             return "OTHER";
//         }
//     };

//     // Pretty-print pipeline registers in an ASCII box
//     // Compact table: Stage | Instruction (hex) | Mnemonic
//     auto print_row = [&](const char *stage, uint32_t inst)
//     {
//         std::cout << "| " << stage << " | 0x" << std::hex << inst << std::dec
//                   << " | " << inst_to_mnemonic(inst) << " |\n";
//     };

//     std::cout << "+-----------------------------------------------+\n";
//     std::cout << "| Stage   | Instruction (hex) | Mnemonic         |\n";
//     std::cout << "+-----------------------------------------------+\n";
//     print_row("IF/ID", if_id_reg_.instruction);
//     print_row("ID/EX", id_ex_reg_.instruction);
//     print_row("EX/MEM", ex_mem_reg_.instruction);
//     print_row("MEM/WB", mem_wb_reg_.instruction);
//     std::cout << "+-----------------------------------------------+\n";
// }

void RV5StageVM_H_NF::handle_syscall()
{
    if ((id_ex_reg_.instruction & 0x7F) == 0b1110011 && ((id_ex_reg_.instruction >> 12) & 0x7) == 0b000)
    {
        RequestStop();
        output_status_ = "ECALL_EXIT";
    }
}
