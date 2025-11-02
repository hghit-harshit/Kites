/**
 * @file rv5svm_h_nf.cpp
 * @brief Implementation for the 5-stage pipelined VM (RV5S) in Mode 3: Hazard Detection, No Forwarding.
 * * NOTE: This implementation is for RV64G (GPR + FPR).
 * * Stall Rule: Any GPR or FPR dependency results in a 2-cycle stall (2 bubbles).
 * * Control Hazards (JAL/JALR, B-Type) are AUTOMATICALLY handled by flush/redirect.
 * @author Atharva and Harshit
 */
#include "vm/rv5s_vms/rv5svm_h_nf.h" // Assuming this header defines RV5StageVM_H_NF
#include "common/instructions.h" 
#include "config.h"              
#include "vm/alu.h"
#include "vm/vm_base.h" 
#include "vm/pipeline_registers.h" 

// *** INCLUDE THE EXTERNAL HDU HEADER FOR FUNCTION DECLARATION ***
#include "rv5s_hdu.h" 

#include <iostream>
#include <thread>
#include <chrono>
#include <tuple>
#include <algorithm>

// NOP instruction: ADDI x0, x0, 0 (Used for flushing)
constexpr uint32_t NOP = 0x00000013;

using namespace alu;

// Define stall count constants (used internally, defined in rv5s_hdu.h)
constexpr int STALL_NONE = 0;
constexpr int STALL_TWO_CYCLES = 2; // Used for H_NF mode

// --- RV5StageVM_H_NF Class Implementation ---

RV5StageVM_H_NF::RV5StageVM_H_NF() : RV5StageVM_Base()
{
    // Initialize VmBase members
    program_counter_ = 0;
    instructions_retired_ = 0;
    cycle_s_ = 0;
    stall_cycles_ = 0; 
    
    // Initialize local members
    stall_fetch_and_decode_ = false;

    // Reset components and history
    Reset();
}

void RV5StageVM_H_NF::Run()
{
    ClearStop();
    while (!stop_requested_ && (program_counter_ < program_size_ || id_ex_reg_.instruction != NOP))
    {
        Step();
    }
}

void RV5StageVM_H_NF::DebugRun()
{
    ClearStop();
    while (!stop_requested_ && (program_counter_ < program_size_ || id_ex_reg_.instruction != NOP))
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
    }
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
    // Capture PC before potential redirection in EX/MEM stages
    uint64_t old_pc_before_redirect = program_counter_;
    
    // Prepare a new delta for recording state changes for Undo/Redo.
    current_delta_ = StepDelta();
    current_delta_.old_pc = program_counter_;

    // 1. Execute back stages (WB -> MEM -> EX)
    pipeline_writeback();
    pipeline_memory();
    pipeline_execute();

    // --- Hazard Detection and Stall Logic (The core of Mode 3) ---
    
    int stalls_needed = 0;
    bool currently_stalling = (stall_cycles_ > 0);
    
    if (currently_stalling) {
        // Stall is already active. Decrement counter and continue stall.
        stall_cycles_--;
        stalls_needed = 1; // Indicate we are continuing the stall (1 cycle consumed)
    } else {
        // Pipeline is free. Check for a NEW hazard using the external HDU function.
        // **CORRECT CALL**: Using the external function with forwarding=false.
        stalls_needed = check_data_hazard(if_id_reg_, id_ex_reg_, false /* is_forwarding_enabled */);
        
        if (stalls_needed > 0) {
            // Start the stall: stalls_needed cycles total. 1 cycle is handled now.
            stall_cycles_ = stalls_needed - 1; 
        }
    }

    // 2. Process Decode/Fetch based on stall status
    if (stalls_needed > 0) {
        // STALL: Freeze IF/ID (by preventing its update) and inject NOP into ID/EX
        id_ex_reg_.reset();
        // Skip pipeline_decode() - The instruction in IF/ID stays put.
        stall_fetch_and_decode_ = true;
    } else {
        // NO STALL: Advance pipeline normally
        pipeline_decode();
        stall_fetch_and_decode_ = false;
    }
    
    // 3. PC Update and Fetch
    uint64_t next_pc = program_counter_; 
    
    // Only advance the PC if we were not stalling this cycle (and thus fetched an instruction).
    // Note: Control hazards (JAL/Branch) already override program_counter_ in EX/MEM.
    if (!stall_fetch_and_decode_ && next_pc == old_pc_before_redirect) {
        next_pc = old_pc_before_redirect + 4;
    }
    
    // Commit the new PC for the Fetch stage
    program_counter_ = next_pc; 
    
    // Fetch the instruction at the committed PC address.
    pipeline_fetch();

    cycle_s_++; // One clock cycle has passed

    // Finalize the delta and manage history stacks.
    current_delta_.new_pc = program_counter_; 

    // Check if any architectural state changed (registers or memory)
    if (!current_delta_.register_changes.empty() || !current_delta_.memory_changes.empty())
    {
        undo_stack_.push(current_delta_);
        while (!redo_stack_.empty())
        {
            redo_stack_.pop();
        }
    }
    
    // Draining check: if PC is past the program AND ID/EX is a NOP (implying pipeline drain)
    if (program_counter_ >= program_size_ && id_ex_reg_.instruction == NOP) {
        RequestStop();
    }
}

// --- Pipeline Stage Implementations (Mode 3: H_NF) ---

void RV5StageVM_H_NF::pipeline_fetch()
{
    // If a stall is active, we prevent IF/ID from updating.
    if (stall_fetch_and_decode_) {
        // IF/ID register is intentionally NOT updated, holding the stalled instruction.
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
    // This function only runs if a stall was NOT active in Step().
    uint32_t instruction = if_id_reg_.instruction;
    control_unit_.SetControlSignals(instruction);

    // Latch data for the ID/EX register
    id_ex_reg_.pc = if_id_reg_.pc;
    id_ex_reg_.instruction = instruction;
    id_ex_reg_.imm = ImmGenerator(instruction);

    // 1. Extract GPR register numbers
    id_ex_reg_.rs1 = (instruction >> 15) & 0x1F;
    id_ex_reg_.rs2 = (instruction >> 20) & 0x1F;
    id_ex_reg_.rd = (instruction >> 7) & 0x1F;
    
    // Read GPR register data naively (NO FORWARDING)
    id_ex_reg_.reg1_data = registers_.ReadGpr(id_ex_reg_.rs1);
    id_ex_reg_.reg2_data = registers_.ReadGpr(id_ex_reg_.rs2);
    
    // 2. Extract and Read FPR Data (STALE)
    id_ex_reg_.frs1 = (instruction >> 15) & 0x1F;
    id_ex_reg_.frs2 = (instruction >> 20) & 0x1F;
    id_ex_reg_.frs3 = (instruction >> 27) & 0x1F;
    id_ex_reg_.frd = (instruction >> 7) & 0x1F;

    // Read FPR data naively (NO FORWARDING)
    id_ex_reg_.freg1_data = registers_.ReadFpr(id_ex_reg_.frs1); 
    id_ex_reg_.freg2_data = registers_.ReadFpr(id_ex_reg_.frs2);
    id_ex_reg_.freg3_data = registers_.ReadFpr(id_ex_reg_.frs3);

    // Pass all control signals to the next stage
    id_ex_reg_.reg_write = control_unit_.GetRegWrite();
    // FPR write enable is determined by control unit for F-type instructions
    uint8_t opcode = instruction & 0x7F;
    id_ex_reg_.freg_write = (opcode == 0b1010011 || opcode == 0b0000111 || (opcode >= 0b1000011 && opcode <= 0b1001111));
    
    id_ex_reg_.branch = control_unit_.GetBranch();
    id_ex_reg_.alu_src = control_unit_.GetAluSrc();
    id_ex_reg_.mem_read = control_unit_.GetMemRead();
    id_ex_reg_.mem_write = control_unit_.GetMemWrite();
    id_ex_reg_.mem_to_reg = control_unit_.GetMemToReg();
    id_ex_reg_.alu_op = control_unit_.GetAluOp();
}

void RV5StageVM_H_NF::pipeline_execute()
{
    // ALU inputs are the values passed from ID/EX. They are guaranteed correct 
    // because the HDU stalled the pipeline until the data reached WB.
    uint64_t alu_in1 = id_ex_reg_.reg1_data;
    uint64_t alu_in2 = id_ex_reg_.alu_src ? static_cast<uint64_t>(id_ex_reg_.imm) : id_ex_reg_.reg2_data;
    
    uint64_t f_alu_in1 = id_ex_reg_.freg1_data;
    uint64_t f_alu_in2 = id_ex_reg_.freg2_data;
    uint64_t f_alu_in3 = id_ex_reg_.freg3_data;

    alu::AluOp alu_operation = control_unit_.GetAluSignal(id_ex_reg_.instruction, id_ex_reg_.alu_op > 0);
    uint64_t alu_result = 0;
    uint64_t f_alu_result = 0;

    // Determine if execution involves FP unit
    bool is_fp_execution = id_ex_reg_.freg_write || id_ex_reg_.alu_op == 3 || id_ex_reg_.alu_op == 4;

    if (is_fp_execution) {
        // Execute Floating Point Operation (assuming double precision for generic RV64G)
        uint8_t fcsr_status = 0;
        // Placeholder for rounding mode (0b000)
        std::tie(f_alu_result, fcsr_status) = alu::Alu::dfpexecute(alu_operation, f_alu_in1, f_alu_in2, f_alu_in3, 0b000); 
    }
    
    // GPR/Integer Execution (Always needed for address calc, branch, or integer ops)
    if (!is_fp_execution || id_ex_reg_.branch || id_ex_reg_.mem_read || id_ex_reg_.mem_write) {
        bool overflow;
        std::tie(alu_result, overflow) = alu::Alu::execute(alu_operation, alu_in1, alu_in2);
    }

    // Latch data for EX/MEM Register
    ex_mem_reg_.alu_result = alu_result;
    ex_mem_reg_.f_alu_result = f_alu_result;
    ex_mem_reg_.rd = id_ex_reg_.rd; 
    ex_mem_reg_.frd = id_ex_reg_.frd; 

    // Store Data: FPR data or GPR data to be written to memory. Uses stale data from ID.
    ex_mem_reg_.reg2_data = id_ex_reg_.reg2_data; 
    
    // Pass control signals...
    ex_mem_reg_.reg_write = id_ex_reg_.reg_write;
    ex_mem_reg_.freg_write = id_ex_reg_.freg_write;
    ex_mem_reg_.mem_to_reg = id_ex_reg_.mem_to_reg;
    ex_mem_reg_.mem_read = id_ex_reg_.mem_read;
    ex_mem_reg_.mem_write = id_ex_reg_.mem_write;
    ex_mem_reg_.branch_taken = false;
    ex_mem_reg_.branch_target_pc = 0;
    
    uint32_t instruction = id_ex_reg_.instruction;
    uint8_t opcode = instruction & 0b1111111;

    // --- Control Hazard Logic (Automated) ---

    // Conditional Branch Check (B-type)
    if (id_ex_reg_.branch && opcode == 0b1100011) 
    {
        bool condition_met = false;
        uint8_t funct3 = (instruction >> 12) & 0x7;
        
        // This fully implements all six branch conditions using the ALU subtraction/comparison result.
        switch (funct3) {
            case 0b000: condition_met = (alu_result == 0); break; // BEQ
            case 0b001: condition_met = (alu_result != 0); break; // BNE
            case 0b100: condition_met = (alu_result == 1); break; // BLT
            case 0b101: condition_met = (alu_result == 0); break; // BGE
            case 0b110: condition_met = (alu_result == 1); break; // BLTU
            case 0b111: condition_met = (alu_result == 0); break; // BGEU
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
        if (opcode == 0b1101111) {
            jump_target = id_ex_reg_.pc + id_ex_reg_.imm; // JAL
        } else {
            jump_target = alu_result & ~1; // JALR (ALU result is Reg + Imm)
            ex_mem_reg_.alu_result = id_ex_reg_.pc + 4; // Set link address (PC+4)
        }

        program_counter_ = jump_target;
        if_id_reg_.reset(); 
        ex_mem_reg_.branch_taken = true; 
    }
}

void RV5StageVM_H_NF::pipeline_memory()
{
    // --- B-Type Conditional Branch Resolution (3-Cycle Penalty) ---
    if (ex_mem_reg_.branch_taken && (id_ex_reg_.instruction & 0b1111111) == 0b1100011) {
        // B-Type misprediction confirmed in MEM stage. Hardware flushes the pipeline.
        
        program_counter_ = ex_mem_reg_.branch_target_pc;
        if_id_reg_.reset(); 
        id_ex_reg_.reset(); 
        branch_mispredictions_++;
    }

    // --- Standard MEM Operations ---
    mem_wb_reg_.alu_result = ex_mem_reg_.alu_result;
    mem_wb_reg_.rd = ex_mem_reg_.rd;
    mem_wb_reg_.reg_write = ex_mem_reg_.reg_write;
    mem_wb_reg_.mem_to_reg = ex_mem_reg_.mem_to_reg;
    
    mem_wb_reg_.f_alu_result = ex_mem_reg_.f_alu_result;
    mem_wb_reg_.frd = ex_mem_reg_.frd;
    mem_wb_reg_.freg_write = ex_mem_reg_.freg_write;
    
    // Memory Access
    if (ex_mem_reg_.mem_read)
    { 
        uint64_t data = memory_controller_.ReadDoubleWord(ex_mem_reg_.alu_result);
        
        // Determine if it's a GPR Load or an FPR Load
        if (mem_wb_reg_.freg_write) {
            // FPR Load (FLW/FLD)
            mem_wb_reg_.f_memory_data = data;
        } else {
            // GPR Load (LW/LD)
            mem_wb_reg_.memory_data = data;
        }
    }
    else if (ex_mem_reg_.mem_write)
    { 
        // Store instruction (Uses data from ID/EX)
        memory_controller_.WriteDoubleWord(ex_mem_reg_.alu_result, ex_mem_reg_.reg2_data);
    }
}

void RV5StageVM_H_NF::pipeline_writeback()
{
    // --- GPR Writeback ---
    if (mem_wb_reg_.reg_write && mem_wb_reg_.rd != 0) 
    {
        uint64_t write_data = mem_wb_reg_.mem_to_reg ? mem_wb_reg_.memory_data : mem_wb_reg_.alu_result;

        // Record state for Undo/Redo
        uint64_t old_value = registers_.ReadGpr(mem_wb_reg_.rd);
        if (old_value != write_data)
        {
            current_delta_.register_changes.push_back({mem_wb_reg_.rd, 0, old_value, write_data});
        }

        registers_.WriteGpr(mem_wb_reg_.rd, write_data);
        instructions_retired_++;
    }
    
    // --- FPR Writeback ---
    if (mem_wb_reg_.freg_write && mem_wb_reg_.frd != 0) 
    {
        uint64_t write_data = mem_wb_reg_.mem_to_reg ? mem_wb_reg_.f_memory_data : mem_wb_reg_.f_alu_result;

        // Record state for Undo/Redo
        uint64_t old_value = registers_.ReadFpr(mem_wb_reg_.frd);
        if (old_value != write_data)
        {
            current_delta_.register_changes.push_back({mem_wb_reg_.frd, 2, old_value, write_data}); // reg_type 2 for FPR
        }

        registers_.WriteFpr(mem_wb_reg_.frd, write_data);
        instructions_retired_++;
    }
}

// --- Auxiliary methods (Full implementation) ---

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
        if (change.reg_type == 0) registers_.WriteGpr(change.reg_index, change.old_value);
        else if (change.reg_type == 2) registers_.WriteFpr(change.reg_index, change.old_value);
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
        if (change.reg_type == 0) registers_.WriteGpr(change.reg_index, change.new_value);
        else if (change.reg_type == 2) registers_.WriteFpr(change.reg_index, change.new_value);
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

void RV5StageVM_H_NF::print_pipeline_registers_debug()
{
    // A basic implementation for debug visibility
    std::cout << "--- Pipeline Debug (Cycle " << cycle_s_ << ") ---" << std::endl;
    std::cout << "PC: 0x" << std::hex << program_counter_ << std::dec << std::endl;
    std::cout << "IF/ID: Inst=0x" << std::hex << if_id_reg_.instruction << std::dec << " PC=" << if_id_reg_.pc << std::endl;
    std::cout << "ID/EX: rs1=" << (int)id_ex_reg_.rs1 << " rs2=" << (int)id_ex_reg_.rs2 << " rd=" << (int)id_ex_reg_.rd << " frd=" << (int)id_ex_reg_.frd << std::endl;
    std::cout << "EX/MEM: ALU_Res=" << ex_mem_reg_.alu_result << " F_ALU_Res=" << ex_mem_reg_.f_alu_result << " Br_Taken=" << ex_mem_reg_.branch_taken << std::endl;
    std::cout << "MEM/WB: ALU_Res=" << mem_wb_reg_.alu_result << " F_ALU_Res=" << mem_wb_reg_.f_alu_result << " Mem_Data=" << mem_wb_reg_.memory_data << " F_Mem_Data=" << mem_wb_reg_.f_memory_data << std::endl;
}

void RV5StageVM_H_NF::execute_float() {
    // This executes single-precision FP operations, using existing ALU data fields.
    // It is called if the instruction is FP R-type (ALUOp=3)
    uint64_t f_alu_in1 = id_ex_reg_.freg1_data;
    uint64_t f_alu_in2 = id_ex_reg_.freg2_data;
    uint64_t f_alu_in3 = id_ex_reg_.freg3_data; // For FMA
    
    uint64_t f_alu_result = 0;
    uint8_t fcsr_status = 0;
    
    alu::AluOp alu_operation = control_unit_.GetAluSignal(id_ex_reg_.instruction, true);
    // Placeholder for rounding mode (0b000)
    std::tie(f_alu_result, fcsr_status) = alu::Alu::fpexecute(alu_operation, f_alu_in1, f_alu_in2, f_alu_in3, 0b000);

    ex_mem_reg_.f_alu_result = f_alu_result;
}

void RV5StageVM_H_NF::execute_double() {
    // This executes double-precision FP operations, using existing ALU data fields.
    uint64_t f_alu_in1 = id_ex_reg_.freg1_data;
    uint64_t f_alu_in2 = id_ex_reg_.freg2_data;
    uint64_t f_alu_in3 = id_ex_reg_.freg3_data; // For FMA
    
    uint64_t f_alu_result = 0;
    uint8_t fcsr_status = 0;
    
    alu::AluOp alu_operation = control_unit_.GetAluSignal(id_ex_reg_.instruction, true);
    // Placeholder for rounding mode (0b000)
    std::tie(f_alu_result, fcsr_status) = alu::Alu::dfpexecute(alu_operation, f_alu_in1, f_alu_in2, f_alu_in3, 0b000);

    ex_mem_reg_.f_alu_result = f_alu_result;
}

void RV5StageVM_H_NF::execute_csr() {
    // Placeholder implementation for CSR operations
    std::cerr << "Warning: CSR instruction passed to placeholder execute_csr()." << std::endl;
}

void RV5StageVM_H_NF::handle_syscall() { 
    if ((id_ex_reg_.instruction & 0x7F) == 0b1110011 && ((id_ex_reg_.instruction >> 12) & 0x7) == 0b000) {
        RequestStop();
        output_status_ = "ECALL_EXIT";
        // DumpState("vm_state.json");
    }
}
