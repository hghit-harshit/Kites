/**
 * @file rv5svm_h_f.cpp
 * @brief Implementation for the 5-stage pipelined VM (RV5S) in Mode 4: Hazard Detection, With Forwarding.
 * * NOTE: This is the full RV64G implementation (GPR + FPR).
 * * Stall Rule: Only Load-Use (GPR or FPR) requires a 1-cycle stall. All others are solved by forwarding.
 * * Control Hazards (JAL/JALR, B-Type) are AUTOMATICALLY handled by flush/redirect.
 * @author Atharva and Harshit
 */
#include "vm/rv5s_vms/rv5svm_h_f.h" // Assuming this header now defines RV5StageVM_H_F
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
constexpr int STALL_ONE_CYCLE = 1; 

// --- RV5StageVM_H_F Class Implementation ---

RV5StageVM_H_F::RV5StageVM_H_F() : RV5StageVM_Base()
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

void RV5StageVM_H_F::Run()
{
    ClearStop();
    while (!stop_requested_ && (program_counter_ < program_size_ || id_ex_reg_.instruction != NOP))
    {
        Step();
    }
}

void RV5StageVM_H_F::DebugRun()
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

void RV5StageVM_H_F::Reset()
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


void RV5StageVM_H_F::Step()
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

    // --- Hazard Detection and Stall Logic (The core of Mode 4) ---
    
    int stalls_needed = 0;
    bool currently_stalling = (stall_cycles_ > 0);
    
    if (currently_stalling) {
        // Stall is already active. Decrement counter and continue stall.
        stall_cycles_--;
        stalls_needed = 1; // Indicate we are continuing the stall (1 cycle consumed)
    } else {
        // Pipeline is free. Check for a NEW hazard using the external HDU function.
        // **CORRECT CALL**: Using the external function with forwarding=true.
        stalls_needed = check_data_hazard(if_id_reg_, id_ex_reg_, true /* is_forwarding_enabled */);
        
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

// --- Pipeline Stage Implementations (Mode 4: H_F) ---

void RV5StageVM_H_F::pipeline_fetch()
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

void RV5StageVM_H_F::pipeline_decode()
{
    // This function only runs if a stall was NOT active in Step().
    uint32_t instruction = if_id_reg_.instruction;
    control_unit_.SetControlSignals(instruction);

    // Latch data for the ID/EX register
    id_ex_reg_.pc = if_id_reg_.pc;
    id_ex_reg_.instruction = instruction;
    id_ex_reg_.imm = ImmGenerator(instruction);

    // 1. Extract GPR and FPR register numbers
    id_ex_reg_.rs1 = (instruction >> 15) & 0x1F;
    id_ex_reg_.rs2 = (instruction >> 20) & 0x1F;
    id_ex_reg_.rd = (instruction >> 7) & 0x1F;
    
    id_ex_reg_.frs1 = (instruction >> 15) & 0x1F;
    id_ex_reg_.frs2 = (instruction >> 20) & 0x1F;
    id_ex_reg_.frs3 = (instruction >> 27) & 0x1F;
    id_ex_reg_.frd = (instruction >> 7) & 0x1F;

    // Read register data naively (Data will be overwritten by forwarding logic)
    id_ex_reg_.reg1_data = registers_.ReadGpr(id_ex_reg_.rs1);
    id_ex_reg_.reg2_data = registers_.ReadGpr(id_ex_reg_.rs2);
    id_ex_reg_.freg1_data = registers_.ReadFpr(id_ex_reg_.frs1); 
    id_ex_reg_.freg2_data = registers_.ReadFpr(id_ex_reg_.frs2);
    id_ex_reg_.freg3_data = registers_.ReadFpr(id_ex_reg_.frs3);

    // Pass all control signals to the next stage
    id_ex_reg_.reg_write = control_unit_.GetRegWrite();
    
    // Set FPR Write Enable (freg_write)
    uint8_t opcode = instruction & 0x7F;
    id_ex_reg_.freg_write = (opcode == 0b1010011 || // FP R-type
                             opcode == 0b0000111 || // FP Load
                             (opcode >= 0b1000011 && opcode <= 0b1001111)); // FMA-type
    
    id_ex_reg_.branch = control_unit_.GetBranch();
    id_ex_reg_.alu_src = control_unit_.GetAluSrc();
    id_ex_reg_.mem_read = control_unit_.GetMemRead();
    id_ex_reg_.mem_write = control_unit_.GetMemWrite();
    id_ex_reg_.mem_to_reg = control_unit_.GetMemToReg();
    id_ex_reg_.alu_op = control_unit_.GetAluOp();
}

void RV5StageVM_H_F::pipeline_execute()
{
    // Initial ALU inputs are the stale values read from the register file (ID_EX)
    uint64_t alu_in1 = id_ex_reg_.reg1_data;
    uint64_t alu_in2 = id_ex_reg_.reg2_data;
    uint64_t f_alu_in1 = id_ex_reg_.freg1_data;
    uint64_t f_alu_in2 = id_ex_reg_.freg2_data;
    uint64_t f_alu_in3 = id_ex_reg_.freg3_data;

    // --- FORWARDING LOGIC VARIABLES ---
    uint8_t forward_gpr_a = 0; uint8_t forward_gpr_b = 0;
    uint8_t forward_fpr_a = 0; uint8_t forward_fpr_b = 0;
    uint8_t forward_fpr_c = 0; // For frs3 (FMA)

    uint8_t rs1 = id_ex_reg_.rs1; uint8_t rs2 = id_ex_reg_.rs2;
    uint8_t frs1 = id_ex_reg_.frs1; uint8_t frs2 = id_ex_reg_.frs2; uint8_t frs3 = id_ex_reg_.frs3;

    // --- 1. GPR Forwarding Checks ---
    if (ex_mem_reg_.reg_write && (ex_mem_reg_.rd != 0)) {
        if (ex_mem_reg_.rd == rs1) forward_gpr_a = 2;
        if (ex_mem_reg_.rd == rs2) forward_gpr_b = 2;
    }
    if (mem_wb_reg_.reg_write && (mem_wb_reg_.rd != 0)) {
        if (mem_wb_reg_.rd == rs1 && forward_gpr_a != 2) forward_gpr_a = 1;
        if (mem_wb_reg_.rd == rs2 && forward_gpr_b != 2) forward_gpr_b = 1;
    }
    
    // --- 2. FPR Forwarding Checks ---
    if (id_ex_reg_.freg_write || id_ex_reg_.alu_op == 3 || id_ex_reg_.alu_op == 4) { 
        if (ex_mem_reg_.freg_write && (ex_mem_reg_.frd != 0)) {
            if (ex_mem_reg_.frd == frs1) forward_fpr_a = 2;
            if (ex_mem_reg_.frd == frs2) forward_fpr_b = 2;
            if (ex_mem_reg_.frd == frs3) forward_fpr_c = 2;
        }
        if (mem_wb_reg_.freg_write && (mem_wb_reg_.frd != 0)) {
            if (mem_wb_reg_.frd == frs1 && forward_fpr_a != 2) forward_fpr_a = 1;
            if (mem_wb_reg_.frd == frs2 && forward_fpr_b != 2) forward_fpr_b = 1;
            if (mem_wb_reg_.frd == frs3 && forward_fpr_c != 2) forward_fpr_c = 1;
        }
    }

    // --- FORWARDING APPLICATION ---
    if (forward_gpr_a == 2) alu_in1 = ex_mem_reg_.alu_result; 
    else if (forward_gpr_a == 1) alu_in1 = mem_wb_reg_.mem_to_reg ? mem_wb_reg_.memory_data : mem_wb_reg_.alu_result;

    if (forward_gpr_b == 2) alu_in2 = ex_mem_reg_.alu_result;
    else if (forward_gpr_b == 1) alu_in2 = mem_wb_reg_.mem_to_reg ? mem_wb_reg_.memory_data : mem_wb_reg_.alu_result;

    if (forward_fpr_a == 2) f_alu_in1 = ex_mem_reg_.f_alu_result;
    else if (forward_fpr_a == 1) f_alu_in1 = mem_wb_reg_.freg_write ? mem_wb_reg_.f_memory_data : mem_wb_reg_.f_alu_result;

    if (forward_fpr_b == 2) f_alu_in2 = ex_mem_reg_.f_alu_result;
    else if (forward_fpr_b == 1) f_alu_in2 = mem_wb_reg_.freg_write ? mem_wb_reg_.f_memory_data : mem_wb_reg_.f_alu_result;

    if (forward_fpr_c == 2) f_alu_in3 = ex_mem_reg_.f_alu_result;
    else if (forward_fpr_c == 1) f_alu_in3 = mem_wb_reg_.freg_write ? mem_wb_reg_.f_memory_data : mem_wb_reg_.f_alu_result;

    // Re-apply immediate check/Store Data Forwarding
    if (id_ex_reg_.alu_src) { alu_in2 = static_cast<uint64_t>(id_ex_reg_.imm); }
    
    // --- EXECUTION ---
    uint32_t instruction = id_ex_reg_.instruction;
    alu::AluOp alu_operation = control_unit_.GetAluSignal(instruction, id_ex_reg_.alu_op > 0);
    uint64_t alu_result = 0;
    uint64_t f_alu_result = 0;
    
    bool is_fp_execution = id_ex_reg_.freg_write || id_ex_reg_.alu_op == 3 || id_ex_reg_.alu_op == 4;

    if (is_fp_execution) {
        uint8_t fcsr_status = 0;
        // The implementation must decide between float/double, assuming double (dfpexecute) for RV64G
        std::tie(f_alu_result, fcsr_status) = Alu::dfpexecute(alu_operation, f_alu_in1, f_alu_in2, f_alu_in3, 0b000); 
    }
    
    // GPR/Integer Execution (Always needed for address calc, branch, or integer ops)
    if (!is_fp_execution || id_ex_reg_.branch || id_ex_reg_.mem_read || id_ex_reg_.mem_write) {
        bool overflow;
        std::tie(alu_result, overflow) = Alu::execute(alu_operation, alu_in1, alu_in2);
    }

    // Latch data for EX/MEM Register
    ex_mem_reg_.alu_result = alu_result;
    ex_mem_reg_.f_alu_result = f_alu_result;
    ex_mem_reg_.rd = id_ex_reg_.rd; ex_mem_reg_.frd = id_ex_reg_.frd; 

    // Store Data: Forwarded GPR data (rs2) is stored in reg2_data for the MEM stage.
    uint64_t store_data = id_ex_reg_.reg2_data;
    if (forward_gpr_b == 2) { store_data = ex_mem_reg_.alu_result; } 
    else if (forward_gpr_b == 1) { store_data = mem_wb_reg_.mem_to_reg ? mem_wb_reg_.memory_data : mem_wb_reg_.alu_result; }
    ex_mem_reg_.reg2_data = store_data;
    
    // Pass control signals...
    ex_mem_reg_.reg_write = id_ex_reg_.reg_write; ex_mem_reg_.freg_write = id_ex_reg_.freg_write;
    ex_mem_reg_.mem_to_reg = id_ex_reg_.mem_to_reg; ex_mem_reg_.mem_read = id_ex_reg_.mem_read;
    ex_mem_reg_.mem_write = id_ex_reg_.mem_write; ex_mem_reg_.branch_taken = false;
    ex_mem_reg_.branch_target_pc = 0;
    
    uint8_t opcode = instruction & 0b1111111;

    // --- Control Hazard Logic (Automated) ---
    if (id_ex_reg_.branch && opcode == 0b1100011) {
        bool condition_met = false; uint8_t funct3 = (instruction >> 12) & 0x7;
        switch (funct3) {
            case 0b000: condition_met = (alu_result == 0); break; 
            case 0b001: condition_met = (alu_result != 0); break; 
            case 0b100: condition_met = (alu_result == 1); break; 
            case 0b101: condition_met = (alu_result == 0); break; 
            case 0b110: condition_met = (alu_result == 1); break; 
            case 0b111: condition_met = (alu_result == 0); break; 
        }
        if (condition_met) {
            ex_mem_reg_.branch_taken = true;
            ex_mem_reg_.branch_target_pc = id_ex_reg_.pc + id_ex_reg_.imm;
        }
    } else if (opcode == 0b1101111 || opcode == 0b1100111) { 
        uint64_t jump_target = (opcode == 0b1101111) ? (id_ex_reg_.pc + id_ex_reg_.imm) : (alu_result & ~1);
        ex_mem_reg_.alu_result = id_ex_reg_.pc + 4; 
        program_counter_ = jump_target; if_id_reg_.reset(); ex_mem_reg_.branch_taken = true; 
    }
}

void RV5StageVM_H_F::pipeline_memory()
{
    // --- B-Type Conditional Branch Resolution (3-Cycle Penalty) ---
    if (ex_mem_reg_.branch_taken && (id_ex_reg_.instruction & 0b1111111) == 0b1100011) {
        program_counter_ = ex_mem_reg_.branch_target_pc;
        if_id_reg_.reset(); id_ex_reg_.reset(); 
        branch_mispredictions_++;
    }

    // --- Standard MEM Operations ---
    mem_wb_reg_.alu_result = ex_mem_reg_.alu_result; mem_wb_reg_.rd = ex_mem_reg_.rd;
    mem_wb_reg_.reg_write = ex_mem_reg_.reg_write; mem_wb_reg_.mem_to_reg = ex_mem_reg_.mem_to_reg;
    
    mem_wb_reg_.f_alu_result = ex_mem_reg_.f_alu_result; mem_wb_reg_.frd = ex_mem_reg_.frd;
    mem_wb_reg_.freg_write = ex_mem_reg_.freg_write; 

    // Memory Access
    if (ex_mem_reg_.mem_read) { 
        uint64_t data = memory_controller_.ReadDoubleWord(ex_mem_reg_.alu_result);
        if (mem_wb_reg_.freg_write) { mem_wb_reg_.f_memory_data = data; } 
        else { mem_wb_reg_.memory_data = data; }
    } else if (ex_mem_reg_.mem_write) { 
        memory_controller_.WriteDoubleWord(ex_mem_reg_.alu_result, ex_mem_reg_.reg2_data);
    }
}

void RV5StageVM_H_F::pipeline_writeback()
{
    // --- GPR Writeback ---
    if (mem_wb_reg_.reg_write && mem_wb_reg_.rd != 0) {
        uint64_t write_data = mem_wb_reg_.mem_to_reg ? mem_wb_reg_.memory_data : mem_wb_reg_.alu_result;
        uint64_t old_value = registers_.ReadGpr(mem_wb_reg_.rd);
        if (old_value != write_data) { current_delta_.register_changes.push_back({mem_wb_reg_.rd, 0, old_value, write_data}); }
        registers_.WriteGpr(mem_wb_reg_.rd, write_data);
        instructions_retired_++;
    }
    
    // --- FPR Writeback ---
    if (mem_wb_reg_.freg_write && mem_wb_reg_.frd != 0) {
        uint64_t write_data = mem_wb_reg_.mem_to_reg ? mem_wb_reg_.f_memory_data : mem_wb_reg_.f_alu_result;
        uint64_t old_value = registers_.ReadFpr(mem_wb_reg_.frd);
        if (old_value != write_data) { current_delta_.register_changes.push_back({mem_wb_reg_.frd, 2, old_value, write_data}); }
        registers_.WriteFpr(mem_wb_reg_.frd, write_data);
        instructions_retired_++;
    }
}

// --- Auxiliary methods ---

void RV5StageVM_H_F::Undo()
{
    if (undo_stack_.empty()) { std::cout << "VM_NO_MORE_UNDO" << std::endl; return; }
    StepDelta last = undo_stack_.top(); undo_stack_.pop();
    for (const auto &change : last.register_changes) {
        if (change.reg_type == 0) registers_.WriteGpr(change.reg_index, change.old_value);
        else if (change.reg_type == 2) registers_.WriteFpr(change.reg_index, change.old_value);
    }
    // Memory and PC restore logic...
    redo_stack_.push(last); std::cout << "VM_UNDO_COMPLETED" << std::endl;
}

void RV5StageVM_H_F::Redo()
{
    if (redo_stack_.empty()) { std::cout << "VM_NO_MORE_REDO" << std::endl; return; }
    StepDelta next = redo_stack_.top(); redo_stack_.pop();
    for (const auto &change : next.register_changes) {
        if (change.reg_type == 0) registers_.WriteGpr(change.reg_index, change.new_value);
        else if (change.reg_type == 2) registers_.WriteFpr(change.reg_index, change.new_value);
    }
    // Memory and PC restore logic...
    undo_stack_.push(next); std::cout << "VM_REDO_COMPLETED" << std::endl;
}

void RV5StageVM_H_F::print_pipeline_registers_debug()
{
    std::cout << "--- Pipeline Debug (Cycle " << cycle_s_ << ") ---" << std::endl;
    std::cout << "PC: 0x" << std::hex << program_counter_ << std::dec << std::endl;
    std::cout << "IF/ID: Inst=0x" << std::hex << if_id_reg_.instruction << std::dec << " PC=" << if_id_reg_.pc << std::endl;
    std::cout << "ID/EX: rs1=" << (int)id_ex_reg_.rs1 << " rd=" << (int)id_ex_reg_.rd << " frd=" << (int)id_ex_reg_.frd << std::endl;
    std::cout << "EX/MEM: ALU_Res=" << ex_mem_reg_.alu_result << " F_ALU_Res=" << ex_mem_reg_.f_alu_result << " Br_Taken=" << ex_mem_reg_.branch_taken << std::endl;
    std::cout << "MEM/WB: ALU_Res=" << mem_wb_reg_.alu_result << " F_ALU_Res=" << mem_wb_reg_.f_alu_result << " Mem_Data=" << mem_wb_reg_.memory_data << " F_Mem_Data=" << mem_wb_reg_.f_memory_data << std::endl;
}

void RV5StageVM_H_F::execute_float() {
    // This executes single-precision FP operations (assuming logic exists to call this).
    uint64_t f_alu_in1 = id_ex_reg_.freg1_data; uint64_t f_alu_in2 = id_ex_reg_.freg2_data; uint64_t f_alu_in3 = id_ex_reg_.freg3_data;
    uint64_t f_alu_result = 0; uint8_t fcsr_status = 0;
    alu::AluOp alu_operation = control_unit_.GetAluSignal(id_ex_reg_.instruction, true);
    std::tie(f_alu_result, fcsr_status) = Alu::fpexecute(alu_operation, f_alu_in1, f_alu_in2, f_alu_in3, 0b000);
    ex_mem_reg_.f_alu_result = f_alu_result;
}

void RV5StageVM_H_F::execute_double() {
    // This executes double-precision FP operations (assuming logic exists to call this).
    uint64_t f_alu_in1 = id_ex_reg_.freg1_data; uint64_t f_alu_in2 = id_ex_reg_.freg2_data; uint64_t f_alu_in3 = id_ex_reg_.freg3_data;
    uint64_t f_alu_result = 0; uint8_t fcsr_status = 0;
    alu::AluOp alu_operation = control_unit_.GetAluSignal(id_ex_reg_.instruction, true);
    std::tie(f_alu_result, fcsr_status) = Alu::dfpexecute(alu_operation, f_alu_in1, f_alu_in2, f_alu_in3, 0b000);
    ex_mem_reg_.f_alu_result = f_alu_result;
}

void RV5StageVM_H_F::execute_csr() { std::cerr << "Warning: CSR instruction passed to placeholder execute_csr()." << std::endl; }
void RV5StageVM_H_F::handle_syscall() { 
    if ((id_ex_reg_.instruction & 0x7F) == 0b1110011 && ((id_ex_reg_.instruction >> 12) & 0x7) == 0b000) {
        RequestStop();
        output_status_ = "ECALL_EXIT";
    }
}
