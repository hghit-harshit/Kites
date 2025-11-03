/**
 * @file rv5svm_nh_f.cpp
 * @brief Implementation for the 5-stage pipelined VM (RV5S) in Mode 2: No Hazard Detection, With Forwarding.
 * * NOTE: This VM includes a Forwarding Unit to resolve ALU-to-ALU data hazards.
 * * The programmer must still insert NOPs for:
 * * 1. Load-Use Hazards: 1 NOP
 * * 2. Conditional Branch Hazards (Resolves in MEM stage): 3 NOPs
 * * 3. Unconditional Jumps (JAL/JALR, resolves in ID stage): 1 NOP
 * @author Atharva and Harshit
 */
#include "vm/rv5s_vms/rv5svm_nh_f.h"
#include "common/instructions.h"
#include "config.h"

#include <iostream>
#include <thread>
#include <chrono>
#include <tuple>
#include <algorithm>

// Constructor initializes the base class and resets the VM state.
RV5StageVM_NH_F::RV5StageVM_NH_F() : RV5StageVM_Base()
{
    Reset();
}

void RV5StageVM_NH_F::Reset()
{
    // Reset base class architectural state members
    program_counter_ = 0;
    instructions_retired_ = 0;
    cycle_s_ = 0;
    stop_requested_ = false;

    // Reset all hardware components
    registers_.Reset();
    memory_controller_.Reset();
    control_unit_.Reset();

    // Reset all pipeline registers to a known-safe (NOP) state
    if_id_reg_.reset();
    id_ex_reg_.reset();
    ex_mem_reg_.reset();
    mem_wb_reg_.reset();

    // Clear history for Undo/Redo
    current_delta_ = StepDelta();
    while (!undo_stack_.empty())
        undo_stack_.pop();
    while (!redo_stack_.empty())
        redo_stack_.pop();
}

void RV5StageVM_NH_F::Step()
{
    // Simulate a single clock cycle by running stages in reverse order
    // (WB -> MEM -> EX -> ID -> IF) to avoid premature data overwrites.

    current_delta_ = StepDelta();
    current_delta_.old_pc = program_counter_;

    pipeline_writeback();
    pipeline_memory();
    pipeline_execute();
    pipeline_decode();
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

// --- High-Level Execution Loops ---

void RV5StageVM_NH_F::Run()
{
    ClearStop();
    while (!stop_requested_ && (program_counter_ < program_size_ || id_ex_reg_.instruction != 0x13))
    {
        Step();
    }
}

void RV5StageVM_NH_F::DebugRun()
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
        // print_pipeline_registers_debug(); // Keep debug functions if implemented
        Step();
        std::cout << "Cycle: " << cycle_s_ << " | PC: 0x" << std::hex << program_counter_ << std::dec << std::endl;
        unsigned int delay_ms = vm_config::config.getRunStepDelay();
        std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));
    }
}

// --- Pipeline Stage Implementations (Mode 2: With Forwarding) ---

void RV5StageVM_NH_F::pipeline_fetch()
{
    // Jumps (JAL/JALR) are handled in MEM stage (1 NOP delay)
    // Conditional Branches are handled in WB stage (3 NOP delay)
    // PC update logic remains the same as Mode 1, as only the data hazard logic changes.

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

void RV5StageVM_NH_F::pipeline_decode()
{
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

    // Read register data naively (NO STALLING for Load-Use, NO FORWARDING yet)
    // The actual forwarding selection happens in the Execute stage.
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

void RV5StageVM_NH_F::pipeline_execute()
{
    // Get the register read indices from the ID/EX register
    uint8_t rs1_id_ex = id_ex_reg_.rs1;
    uint8_t rs2_id_ex = id_ex_reg_.rs2;
    uint64_t alu_in1 = id_ex_reg_.reg1_data;
    uint64_t alu_in2 = id_ex_reg_.alu_src ? static_cast<uint64_t>(id_ex_reg_.imm) : id_ex_reg_.reg2_data;

    // --- FORWARDING UNIT LOGIC (Mode 2) ---
    
    // 1. Check EX/MEM Register for Forwarding (One stage back)
    // Forwarding A (rs1)
    if (ex_mem_reg_.reg_write && ex_mem_reg_.rd != 0 && ex_mem_reg_.rd == rs1_id_ex) {
        alu_in1 = ex_mem_reg_.alu_result; // Forward ALU result
    } 
    // Forwarding B (rs2)
    if (ex_mem_reg_.reg_write && ex_mem_reg_.rd != 0 && ex_mem_reg_.rd == rs2_id_ex) {
        alu_in2 = ex_mem_reg_.alu_result; // Forward ALU result
    }

    // 2. Check MEM/WB Register for Forwarding (Two stages back)
    // Note: The MEM/WB check overrides the EX/MEM check if both match, which is correct 
    // for a Load instruction that skipped the EX/MEM path, or to handle R-Type/I-Type 
    // results that pass through memory.
    
    // Forwarding A (rs1)
    if (mem_wb_reg_.reg_write && mem_wb_reg_.rd != 0 && mem_wb_reg_.rd == rs1_id_ex) {
        // Source is Load result (memory_data) or ALU result (alu_result)
        alu_in1 = mem_wb_reg_.mem_to_reg ? mem_wb_reg_.memory_data : mem_wb_reg_.alu_result;
    }
    // Forwarding B (rs2)
    if (mem_wb_reg_.reg_write && mem_wb_reg_.rd != 0 && mem_wb_reg_.rd == rs2_id_ex) {
        alu_in2 = mem_wb_reg_.mem_to_reg ? mem_wb_reg_.memory_data : mem_wb_reg_.alu_result;
    }

    // IMPORTANT: LOAD-USE HAZARDS ARE NOT HANDLED.
    // If the instruction in EX/MEM is a Load (`ex_mem_reg_.mem_read` is true), 
    // and the current instruction (in ID/EX) uses its result, the forwarded data 
    // will be stale (or from the ALU calculation). The programmer must insert 1 NOP.
    
    // --- ALU Execution ---
    alu::AluOp alu_operation = control_unit_.GetAluSignal(id_ex_reg_.instruction, id_ex_reg_.alu_op > 0);
    bool overflow;
    std::tie(ex_mem_reg_.alu_result, overflow) = alu_.execute(alu_operation, alu_in1, alu_in2);

    // --- Control Hazard Logic (Same as Mode 1: Programmer must insert NOPs) ---
    ex_mem_reg_.branch_taken = false;
    uint8_t opcode = id_ex_reg_.instruction & 0b1111111;

    if (id_ex_reg_.branch && opcode == 0b1100011) // Conditional Branch (B-type)
    {
        bool condition_met = false;
        uint8_t funct3 = (id_ex_reg_.instruction >> 12) & 0x7;
        
        if ((funct3 == 0b000 && ex_mem_reg_.alu_result == 0) || // BEQ (result == 0)
            (funct3 == 0b001 && ex_mem_reg_.alu_result != 0))   // BNE (result != 0)
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

    // Pass necessary data to the next stage
    ex_mem_reg_.reg2_data = id_ex_reg_.reg2_data; // Data for Store instructions
    ex_mem_reg_.rd = id_ex_reg_.rd;

    // Pass control signals through
    ex_mem_reg_.reg_write = id_ex_reg_.reg_write;
    ex_mem_reg_.mem_read = id_ex_reg_.mem_read;
    ex_mem_reg_.mem_write = id_ex_reg_.mem_write;
    ex_mem_reg_.mem_to_reg = id_ex_reg_.mem_to_reg;
}

void RV5StageVM_NH_F::pipeline_memory()
{
    // PC Redirection for Unconditional Jumps (1 NOP delay)
    if (ex_mem_reg_.branch_taken) {
        uint32_t instruction_in_ex = id_ex_reg_.instruction; 
        uint8_t opcode = instruction_in_ex & 0b1111111;

        if (opcode == 0b1101111 || opcode == 0b1100111) { // JAL or JALR
            program_counter_ = ex_mem_reg_.branch_target_pc;
            if_id_reg_.reset(); // Kill the instruction (should be the programmer's NOP)
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

void RV5StageVM_NH_F::pipeline_writeback()
{
    // PC Redirection for Conditional Branches (3 NOP delay)
    if (ex_mem_reg_.branch_taken) {
        uint32_t instruction_in_mem = id_ex_reg_.instruction; 
        uint8_t opcode = instruction_in_mem & 0b1111111;

        if (opcode == 0b1100011) { // Conditional Branch (B-type)
            // Redirect the fetch PC
            program_counter_ = ex_mem_reg_.branch_target_pc;
            
            // Kill instructions I1, I2, I3
            id_ex_reg_.reset(); 
            if_id_reg_.reset();
            mem_wb_reg_.reset(); // Kill I3 (in MEM/WB)
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

// --- Undo/Redo Implementations (No changes from base) ---

void RV5StageVM_NH_F::Undo()
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

void RV5StageVM_NH_F::Redo()
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

void RV5StageVM_NH_F::print_pipeline_registers_debug()
{
    // Implementation for printing debug state (can be added later if needed)
}
