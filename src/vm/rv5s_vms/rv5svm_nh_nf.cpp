/**
 * @file rv5s_vm.cpp
 * @brief Implementation for the 5-stage pipelined VM (RV5S).
 * @author Atharva and Harshit
 */
#include "vm/rv5s_vms/rv5svm_nh_nf.h"
#include "common/instructions.h"
#include "config.h"

#include <iostream>
#include <thread>
#include <chrono>
#include <tuple>
#include <algorithm>

RV5StageVM_NH_NF::RV5StageVM_NH_NF() : RV5StageVM_Base()
{
    Reset();
}

void RV5StageVM_NH_NF::Reset()
{
    // Reset base class members
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

void RV5StageVM_NH_NF::Step()
{
    // A single clock cycle in a pipelined processor involves all stages
    // running concurrently on different instructions. We simulate this by
    // executing the stages in reverse order. This ensures that the output
    // of one stage in cycle N becomes the input of the next stage in cycle N+1
    // without overwriting the data prematurely.

    // Before starting the cycle, prepare a new delta for recording state changes.
    current_delta_ = StepDelta();
    current_delta_.old_pc = program_counter_;

    pipeline_writeback();
    pipeline_memory();
    pipeline_execute();
    pipeline_decode();
    pipeline_fetch();

    cycle_s_++; // One clock cycle has passed

    // After the cycle, finalize the delta and manage history stacks.
    current_delta_.new_pc = program_counter_;
    if (!current_delta_.register_changes.empty() || !current_delta_.memory_changes.empty())
    {
        undo_stack_.push(current_delta_);
        // A new action invalidates any previous "redo" history.
        while (!redo_stack_.empty())
        {
            redo_stack_.pop();
        }
    }
}

// --- High-Level Execution Loops ---

void RV5StageVM_NH_NF::Run()
{
    ClearStop();
    // The loop must continue until the PC is past the program AND the pipeline is empty.
    // A simple check is to see if a real instruction is still in the decode stage.
    while (!stop_requested_ && (program_counter_ < program_size_ || id_ex_reg_.instruction != 0x13))
    {
        Step();
    }
}

void RV5StageVM_NH_NF::DebugRun()
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
        print_pipeline_registers_debug();
        Step();
        std::cout << "Cycle: " << cycle_s_ << " | PC: 0x" << std::hex << program_counter_ << std::dec << std::endl;
        unsigned int delay_ms = vm_config::config.getRunStepDelay();
        std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));
    }
}

// --- Pipeline Stage Implementations (Option 1: No Hazard Detection) ---

void RV5StageVM_NH_NF::pipeline_fetch()
{
    if (program_counter_ < program_size_)
    {
        // Latch the instruction and PC for the next stage
        if_id_reg_.instruction = memory_controller_.ReadWord(program_counter_);
        if_id_reg_.pc = program_counter_;

        // Always predict "not taken" and fetch the next sequential instruction.
        program_counter_ += 4;
    }
    else
    {
        // Once past the end of the program, keep injecting NOPs.
        if_id_reg_.reset();
    }
}

void RV5StageVM_NH_NF::pipeline_decode()
{
    // Get instruction from the IF/ID register
    uint32_t instruction = if_id_reg_.instruction;

    // Let the control unit generate signals for this instruction
    control_unit_.SetControlSignals(instruction);

    // Prepare data for the next stage (ID/EX register) by latching it
    id_ex_reg_.pc = if_id_reg_.pc;
    id_ex_reg_.instruction = instruction; // Pass full instruction for decoding in EX
    id_ex_reg_.imm = ImmGenerator(instruction);

    // Extract register numbers
    id_ex_reg_.rs1 = (instruction >> 15) & 0x1F;
    id_ex_reg_.rs2 = (instruction >> 20) & 0x1F;
    id_ex_reg_.rd = (instruction >> 7) & 0x1F;

    // Read register data.
    // **NOTE for Option 1:** This is where data hazards will cause errors.
    // The code naively reads from the register file, which may hold stale data.
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

void RV5StageVM_NH_NF::pipeline_execute()
{
    // Select ALU inputs based on alu_src signal
    uint64_t alu_in1 = id_ex_reg_.reg1_data;
    uint64_t alu_in2 = id_ex_reg_.alu_src ? static_cast<uint64_t>(id_ex_reg_.imm) : id_ex_reg_.reg2_data;

    // Get the specific ALU operation from the ALU Control unit
    alu::AluOp alu_operation = control_unit_.GetAluSignal(id_ex_reg_.instruction, id_ex_reg_.alu_op > 0);

    // Execute the operation
    bool overflow; // Ignored for this simple model
    std::tie(ex_mem_reg_.alu_result, overflow) = alu_.execute(alu_operation, alu_in1, alu_in2);

    // --- Control Hazard Logic (Simple Flush on Taken Branch) ---
    ex_mem_reg_.branch_taken = false;
    if (id_ex_reg_.branch)
    {
        bool condition_met = false;
        uint8_t funct3 = (id_ex_reg_.instruction >> 12) & 0x7;

        // This logic is simplified for BEQ and BNE for this initial version
        if ((funct3 == 0b000 && ex_mem_reg_.alu_result == 0) || // BEQ
            (funct3 == 0b001 && ex_mem_reg_.alu_result != 0))   // BNE
        {
            condition_met = true;
        }
        // More branch conditions (BLT, BGE, etc.) would be added later.

        if (condition_met)
        {
            ex_mem_reg_.branch_taken = true;
            ex_mem_reg_.branch_target_pc = id_ex_reg_.pc + id_ex_reg_.imm;

            // On a taken branch, we must flush the two incorrect instructions
            // that were fetched. We do this by resetting the IF/ID and ID/EX registers.
            program_counter_ = ex_mem_reg_.branch_target_pc;
            if_id_reg_.reset();
            id_ex_reg_.reset();
        }
    }

    // Pass necessary data to the next stage
    ex_mem_reg_.reg2_data = id_ex_reg_.reg2_data; // Needed for Store instructions
    ex_mem_reg_.rd = id_ex_reg_.rd;

    // Pass control signals through
    ex_mem_reg_.reg_write = id_ex_reg_.reg_write;
    ex_mem_reg_.mem_read = id_ex_reg_.mem_read;
    ex_mem_reg_.mem_write = id_ex_reg_.mem_write;
    ex_mem_reg_.mem_to_reg = id_ex_reg_.mem_to_reg;
}

void RV5StageVM_NH_NF::pipeline_memory()
{
    // For Undo/Redo, we must record memory changes
    std::vector<uint8_t> old_bytes_vec;
    std::vector<uint8_t> new_bytes_vec;
    uint64_t addr = ex_mem_reg_.alu_result;

    if (ex_mem_reg_.mem_read)
    { // Load instruction
        mem_wb_reg_.memory_data = memory_controller_.ReadDoubleWord(addr);
    }
    else if (ex_mem_reg_.mem_write)
    { // Store instruction
        // Record state BEFORE the write for Undo
        for (size_t i = 0; i < 8; ++i)
            old_bytes_vec.push_back(memory_controller_.ReadByte_d(addr + i));

        memory_controller_.WriteDoubleWord(addr, ex_mem_reg_.reg2_data);

        // Record state AFTER the write for Redo
        for (size_t i = 0; i < 8; ++i)
            new_bytes_vec.push_back(memory_controller_.ReadByte_d(addr + i));

        if (old_bytes_vec != new_bytes_vec)
        {
            current_delta_.memory_changes.push_back({addr, old_bytes_vec, new_bytes_vec});
        }
    }

    // Pass necessary data to the final stage
    mem_wb_reg_.alu_result = ex_mem_reg_.alu_result;
    mem_wb_reg_.rd = ex_mem_reg_.rd;

    // Pass control signals through
    mem_wb_reg_.reg_write = ex_mem_reg_.reg_write;
    mem_wb_reg_.mem_to_reg = ex_mem_reg_.mem_to_reg;
}

void RV5StageVM_NH_NF::pipeline_writeback()
{
    if (mem_wb_reg_.reg_write && mem_wb_reg_.rd != 0)
    {
        uint64_t write_data = mem_wb_reg_.mem_to_reg ? mem_wb_reg_.memory_data : mem_wb_reg_.alu_result;

        // Record state for Undo/Redo
        uint64_t old_value = registers_.ReadGpr(mem_wb_reg_.rd);
        if (old_value != write_data)
        {
            current_delta_.register_changes.push_back({mem_wb_reg_.rd,
                                                       0, // GPR
                                                       old_value,
                                                       write_data});
        }

        registers_.WriteGpr(mem_wb_reg_.rd, write_data);
        instructions_retired_++;
    }
}

// --- Undo/Redo Implementations ---

void RV5StageVM_NH_NF::Undo()
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
    // Note: In a simple pipeline, undoing one instruction might require flushing the pipeline
    // to reflect the correct state, but for a basic undo, just resetting the PC is a start.
    // For full correctness, one might need to re-run the pipeline from the old_pc to repopulate it.
    // For now, we will reset the pipeline registers to NOPs.
    if_id_reg_.reset();
    id_ex_reg_.reset();
    ex_mem_reg_.reset();
    mem_wb_reg_.reset();

    redo_stack_.push(last);
    std::cout << "VM_UNDO_COMPLETED" << std::endl;
}

void RV5StageVM_NH_NF::Redo()
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

    // As with Undo, a fully correct Redo might involve complex state restoration.
    // For this implementation, we simply restore the architectural state and PC.
    program_counter_ = next.new_pc;
    if_id_reg_.reset();
    id_ex_reg_.reset();
    ex_mem_reg_.reset();
    mem_wb_reg_.reset();

    undo_stack_.push(next);
    std::cout << "VM_REDO_COMPLETED" << std::endl;
}

void RV5StageVM_NH_NF::print_pipeline_registers_debug()
{
    
}