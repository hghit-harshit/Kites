/**
 * @file rv5svm_h_f.cpp
 * @brief Implementation for the 5-stage pipelined VM (RV5S) in Mode 4: Hazard Detection, With Forwarding.
 * * NOTE: This VM resolves ALU-ALU hazards via forwarding and Load-Use hazards via a single-cycle stall.
 * * This achieves near-optimal performance without requiring any manual NOPs.
 * @author Atharva and Harshit
 */
#include "vm/rv5s_vms/rv5svm_h_f.h"
#include "common/instructions.h"
#include "config.h"

#include <iostream>
#include <thread>
#include <chrono>
#include <tuple>
#include <algorithm>

RV5StageVM_H_F::RV5StageVM_H_F() : RV5StageVM_Base()
{
    Reset();
}

void RV5StageVM_H_F::Reset()
{
    program_counter_ = 0;
    instructions_retired_ = 0;
    cycle_s_ = 0;
    stop_requested_ = false;
    stall_fetch_and_decode_ = false;

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

void RV5StageVM_H_F::Step()
{
    // Simulate a single clock cycle in reverse order.
    current_delta_ = StepDelta();
    current_delta_.old_pc = program_counter_;

    pipeline_writeback();
    pipeline_memory();
    pipeline_execute();
    
    // --- Hazard Detection and Stall Logic (The core of Mode 4) ---
    bool hazard_detected = check_for_hazard(); 

    if (hazard_detected) {
        // Stall triggered: Freeze the front-end (Fetch and Decode)
        stall_fetch_and_decode_ = true;
        
        // Inject a NOP (bubble) into the ID/EX register. This resolves the Load-Use or Control hazard.
        id_ex_reg_.reset(); 
        stall_cycles_++;
    } else {
        stall_fetch_and_decode_ = false;
        pipeline_decode();
    }
    
    pipeline_fetch();

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
}

// --- Hazard Detection Logic (Delegated to the dedicated Unit) ---
bool RV5StageVM_H_F::check_for_hazard()
{
    // Mode 4 Logic: Only stall if Forwarding cannot resolve the dependency.
    // 1. Check for Load-Use Hazard: This is the only data hazard that forwarding cannot fix.
    if (hazard_unit_.check_for_load_use_stall(if_id_reg_, id_ex_reg_, ex_mem_reg_)) {
        return true;
    }
    
    // 2. Check for Control Hazards (Conditional Branches)
    if (hazard_unit_.check_for_control_stall(if_id_reg_)) {
        return true;
    }
    
    return false;
}

// --- Pipeline Stage Implementations (Mode 4) ---

void RV5StageVM_H_F::pipeline_fetch()
{
    // If a stall is active, we freeze the PC and IF/ID register.
    if (stall_fetch_and_decode_) {
        return; 
    }

    if (program_counter_ < program_size_)
    {
        if_id_reg_.instruction = memory_controller_.ReadWord(program_counter_);
        if_id_reg_.pc = program_counter_;
        program_counter_ += 4;
    }
    else
    {
        if_id_reg_.reset();
    }
}

void RV5StageVM_H_F::pipeline_decode()
{
    // NOTE: This function only runs if stall_fetch_and_decode_ was false in Step().
    uint32_t instruction = if_id_reg_.instruction;
    control_unit_.SetControlSignals(instruction);

    // Latch data for the ID/EX register
    id_ex_reg_.pc = if_id_reg_.pc;
    id_ex_reg_.instruction = instruction;
    id_ex_reg_.imm = ImmGenerator(instruction);

    // Extract register numbers
    id_ex_reg_.rs1 = (instruction >> 15) & 0x1F;
    id_ex_reg_.rs2 = (instruction >> 20) & 0x1F;
    id_ex_reg_.rd = (instruction >> 7) & 0x1F;

    // Read register data naively (Forwarding will correct this in the next stage)
    id_ex_reg_.reg1_data = registers_.ReadGpr(id_ex_reg_.rs1);
    id_ex_reg_.reg2_data = registers_.ReadGpr(id_ex_reg_.rs2);

    // Pass all control signals to the next stage
    id_ex_reg_.reg_write = control_unit_.GetRegWrite();
    id_ex_reg_.branch = control_unit_.GetBranch();
    id_ex_reg_.alu_src = control_unit_.GetAluSrc();
    id_ex_reg_.mem_read = control_unit_.GetMemRead();
    id_ex_reg_.mem_write = control_unit_.GetMemWrite();
    id_ex_reg_.mem_to_reg = control_unit_.GetMemToReg();
    id_ex_reg_.alu_op = control_unit_.GetAluOp();
}

void RV5StageVM_H_F::pipeline_execute()
{
    uint8_t rs1_id_ex = id_ex_reg_.rs1;
    uint8_t rs2_id_ex = id_ex_reg_.rs2;
    uint64_t alu_in1 = id_ex_reg_.reg1_data;
    uint64_t alu_in2 = id_ex_reg_.alu_src ? static_cast<uint64_t>(id_ex_reg_.imm) : id_ex_reg_.reg2_data;
    
    // --- FORWARDING UNIT LOGIC (The core of Mode 4 speed) ---

    // 1. Forward from EX/MEM Register (Newest ALU result)
    if (ex_mem_reg_.reg_write && ex_mem_reg_.rd != 0) {
        if (ex_mem_reg_.rd == rs1_id_ex) alu_in1 = ex_mem_reg_.alu_result;
        if (ex_mem_reg_.rd == rs2_id_ex) alu_in2 = ex_mem_reg_.alu_result;
    }

    // 2. Forward from MEM/WB Register (Older result or Load result)
    // This check overrides EX/MEM for a three-cycle dependency, ensuring the newest data is used.
    if (mem_wb_reg_.reg_write && mem_wb_reg_.rd != 0) {
        uint64_t forwarded_data = mem_wb_reg_.mem_to_reg ? mem_wb_reg_.memory_data : mem_wb_reg_.alu_result;
        
        if (mem_wb_reg_.rd == rs1_id_ex) alu_in1 = forwarded_data;
        if (mem_wb_reg_.rd == rs2_id_ex) alu_in2 = forwarded_data;
    }
    // --- END FORWARDING ---

    alu::AluOp alu_operation = control_unit_.GetAluSignal(id_ex_reg_.instruction, id_ex_reg_.alu_op > 0);
    bool overflow;
    std::tie(ex_mem_reg_.alu_result, overflow) = alu_.execute(alu_operation, alu_in1, alu_in2);

    // --- Control Hazard Resolution ---
    ex_mem_reg_.branch_taken = false;
    uint8_t opcode = id_ex_reg_.instruction & 0b1111111;

    if (id_ex_reg_.branch && opcode == 0b1100011) // Conditional Branch (B-type)
    {
        bool condition_met = false;
        uint8_t funct3 = (id_ex_reg_.instruction >> 12) & 0x7;
        
        if ((funct3 == 0b000 && ex_mem_reg_.alu_result == 0) || // BEQ
            (funct3 == 0b001 && ex_mem_reg_.alu_result != 0))   // BNE
        {
            condition_met = true;
        }
        
        if (condition_met)
        {
            ex_mem_reg_.branch_taken = true;
            ex_mem_reg_.branch_target_pc = id_ex_reg_.pc + id_ex_reg_.imm;
        }
    }
    else if (opcode == 0b1101111) // JAL (J-type)
    {
        ex_mem_reg_.branch_taken = true;
        ex_mem_reg_.branch_target_pc = id_ex_reg_.pc + id_ex_reg_.imm;
    }
    else if (opcode == 0b1100111) // JALR (I-type)
    {
        ex_mem_reg_.branch_taken = true;
        ex_mem_reg_.branch_target_pc = ex_mem_reg_.alu_result & ~1;
    }

    // Pass necessary data and signals to the next stage
    ex_mem_reg_.reg2_data = id_ex_reg_.reg2_data;
    ex_mem_reg_.rd = id_ex_reg_.rd;
    ex_mem_reg_.reg_write = id_ex_reg_.reg_write;
    ex_mem_reg_.mem_read = id_ex_reg_.mem_read;
    ex_mem_reg_.mem_write = id_ex_reg_.mem_write;
    ex_mem_reg_.mem_to_reg = id_ex_reg_.mem_to_reg;
}

void RV5StageVM_H_F::pipeline_memory()
{
    // PC Redirection for Jumps (JAL/JALR) - resolved by 1-cycle flush/bubble
    if (ex_mem_reg_.branch_taken) {
        uint32_t instruction_in_ex = id_ex_reg_.instruction; 
        uint8_t opcode = instruction_in_ex & 0b1111111;

        if (opcode == 0b1101111 || opcode == 0b1100111) { // JAL or JALR
            program_counter_ = ex_mem_reg_.branch_target_pc;
            if_id_reg_.reset(); // Kill the instruction currently in IF/ID (the bubble/stalled instruction)
        }
    }

    // Record memory changes for Undo/Redo
    std::vector<uint8_t> old_bytes_vec;
    std::vector<uint8_t> new_bytes_vec;
    uint64_t addr = ex_mem_reg_.alu_result;

    if (ex_mem_reg_.mem_read)
    { 
        mem_wb_reg_.memory_data = memory_controller_.ReadDoubleWord(addr);
    }
    else if (ex_mem_reg_.mem_write)
    { 
        // Store instruction
        for (size_t i = 0; i < 8; ++i)
            old_bytes_vec.push_back(memory_controller_.ReadByte_d(addr + i));

        memory_controller_.WriteDoubleWord(addr, ex_mem_reg_.reg2_data);

        for (size_t i = 0; i < 8; ++i)
            new_bytes_vec.push_back(memory_controller_.ReadByte_d(addr + i));

        if (old_bytes_vec != new_bytes_vec)
        {
            current_delta_.memory_changes.push_back({addr, old_bytes_vec, new_bytes_vec});
        }
    }

    // Pass necessary data to the final stage (MEM/WB register)
    mem_wb_reg_.alu_result = ex_mem_reg_.alu_result;
    mem_wb_reg_.rd = ex_mem_reg_.rd;

    // Pass control signals through
    mem_wb_reg_.reg_write = ex_mem_reg_.reg_write;
    mem_wb_reg_.mem_to_reg = ex_mem_reg_.mem_to_reg;
}

void RV5StageVM_H_F::pipeline_writeback()
{
    // PC Redirection for Conditional Branches (resolved by stall)
    if (ex_mem_reg_.branch_taken) {
        uint32_t instruction_in_mem = id_ex_reg_.instruction; 
        uint8_t opcode = instruction_in_mem & 0b1111111;

        if (opcode == 0b1100011) { // Conditional Branch (B-type)
            // Redirect the fetch PC
            program_counter_ = ex_mem_reg_.branch_target_pc;
            
            // Kill instructions still in the front-end to ensure the next fetch is clean
            id_ex_reg_.reset(); 
            if_id_reg_.reset();
            mem_wb_reg_.reset(); 
        }
    }
    

    // Write the final result back to the register file
    if (mem_wb_reg_.reg_write && mem_wb_reg_.rd != 0)
    {
        uint64_t write_data = mem_wb_reg_.mem_to_reg ? mem_wb_reg_.memory_data : mem_wb_reg_.alu_result;

        // Record state for Undo/Redo
        uint64_t old_value = registers_.ReadGpr(mem_wb_reg_.rd);
        if (old_value != write_data)
        {
            current_delta_.register_changes.push_back({mem_wb_reg_.rd,
                                                       0, // GPR type
                                                       old_value,
                                                       write_data});
        }

        registers_.WriteGpr(mem_wb_reg_.rd, write_data);
        instructions_retired_++;
    }
}

// --- High-Level Execution Loops and Undo/Redo ---

void RV5StageVM_H_F::Run()
{
    ClearStop();
    while (!stop_requested_ && (program_counter_ < program_size_ || id_ex_reg_.instruction != 0x13))
    {
        Step();
    }
}

void RV5StageVM_H_F::DebugRun()
{
    ClearStop();
    while (!stop_requested_ && (program_counter_ < program_size_ || id_ex_reg_.instruction != 0x13))
    {
        if (CheckBreakpoint(program_counter_))
        {
            std::cout << "VM_BREAKPOINT_HIT " << program_counter_ << std::endl;
            output_status_ = "VM_BREAKPOINT_HIT";
            break;
        }
        // print_pipeline_registers_debug();
        Step();
        std::cout << "Cycle: " << cycle_s_ << " | PC: 0x" << std::hex << program_counter_ << std::dec << std::endl;
        unsigned int delay_ms = vm_config::config.getRunStepDelay();
        std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));
    }
}

void RV5StageVM_H_F::Undo()
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

void RV5StageVM_H_F::Redo()
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

void RV5StageVM_H_F::print_pipeline_registers_debug()
{
    // Implementation for printing debug state (can be added later if needed)
}
