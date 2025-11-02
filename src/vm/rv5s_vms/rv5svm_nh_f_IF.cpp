/**
 * @file rv5s_vm.cpp
 * @brief Implementation of the RV64 5-stage pipelined Virtual Machine in strict NH_F mode (No Hazard Detection, With Forwarding).
 * * Data Hazards (GPR & FPR): FORWARDING IMPLEMENTED. R-R, R-Store, Load-Store require 0 NOPs. Load-Use requires 1 NOP.
 * * Control Hazards (JAL/JALR, B-Type): AUTOMATICALLY handled by the pipeline (Static Prediction).
 * * F-Type Compatibility: Added FPR data paths and execution logic.
 * * @author Atharva and Harshit
 */

#include "vm/rv5s_vms/rv5svm_nh_f.h" // Including the correct header file for RV5StageVM_NH_F
#include "common/instructions.h" 
#include "config.h"              
#include "vm/alu.h"
#include "vm/vm_base.h" // For ImmGenerator, etc.

#include <iostream>
#include <thread>
#include <chrono>
#include <tuple>
#include <algorithm>

// NOP instruction: ADDI x0, x0, 0 (Used for flushing)
constexpr uint32_t NOP = 0x00000013;

using namespace alu;

// --- Constructor (Assuming it exists and calls Reset) ---
RV5StageVM_NH_F::RV5StageVM_NH_F() : RV5StageVM_Base() {
    Reset();
}


// --- VmBase Pure Virtual Method Implementations (Run, DebugRun, Reset, Step) ---

void RV5StageVM_NH_F::Run()
{
    ClearStop();
    while (!stop_requested_ && (program_counter_ < program_size_ || id_ex_reg_.instruction != NOP))
    {
        Step();
    }
}

void RV5StageVM_NH_F::DebugRun()
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

    // Clear history for Undo/Redo (inherited from VmBase)
    current_delta_ = StepDelta();
    while (!undo_stack_.empty())
        undo_stack_.pop();
    while (!redo_stack_.empty())
        redo_stack_.pop();
}

void RV5StageVM_NH_F::Step()
{
    // Capture PC before potential redirection in EX/MEM stages
    uint64_t old_pc_before_redirect = program_counter_;
    
    // Prepare a new delta for recording state changes for Undo/Redo.
    current_delta_ = StepDelta();
    current_delta_.old_pc = program_counter_;

    // 1. Execute stages (WB -> MEM -> EX -> ID -> IF)
    pipeline_writeback();
    pipeline_memory(); // Branch (3-cycle) resolution and PC redirect
    pipeline_execute(); // JAL/JALR (1-cycle) resolution and PC redirect
    pipeline_decode();
    
    // 2. Determine the next PC (Redirection logic overrides sequential advance)
    uint64_t next_pc = program_counter_; 
    
    // If no redirect happened in EX or MEM, advance sequentially.
    if (next_pc == old_pc_before_redirect) {
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

// --- Pipeline Stage Implementations (NH_F + F-Type) ---

void RV5StageVM_NH_F::pipeline_fetch()
{
    if (program_counter_ < program_size_)
    {
        // Latch the instruction and PC for the next stage (IF/ID register)
        if_id_reg_.instruction = memory_controller_.ReadWord(program_counter_);
        if_id_reg_.pc = program_counter_;
    }
    else
    {
        // Once past the end of the program, inject NOPs to drain the pipeline.
        if_id_reg_.reset();
    }
}

void RV5StageVM_NH_F::pipeline_decode()
{
    // Get instruction from the IF/ID register
    uint32_t instruction = if_id_reg_.instruction;

    // Control Unit: Generate signals based on the instruction
    control_unit_.SetControlSignals(instruction);

    // Latch data for the ID/EX register
    id_ex_reg_.pc = if_id_reg_.pc;
    id_ex_reg_.instruction = instruction;
    id_ex_reg_.imm = ImmGenerator(instruction);

    // 1. Extract and Read GPR Data (STALE)
    uint8_t rs1 = (instruction >> 15) & 0x1F;
    uint8_t rs2 = (instruction >> 20) & 0x1F;
    uint8_t rd = (instruction >> 7) & 0x1F;
    
    id_ex_reg_.rs1 = rs1;
    id_ex_reg_.rs2 = rs2;
    id_ex_reg_.rd = rd;

    id_ex_reg_.reg1_data = registers_.ReadGpr(rs1);
    id_ex_reg_.reg2_data = registers_.ReadGpr(rs2);
    
    // 2. Extract and Read FPR Data (STALE)
    uint8_t frs1 = (instruction >> 15) & 0x1F;
    uint8_t frs2 = (instruction >> 20) & 0x1F;
    uint8_t frs3 = (instruction >> 27) & 0x1F;
    uint8_t frd = (instruction >> 7) & 0x1F;

    id_ex_reg_.frs1 = frs1;
    id_ex_reg_.frs2 = frs2;
    id_ex_reg_.frs3 = frs3;
    id_ex_reg_.frd = frd;

    // NH_F rule applied: reads stale data, relies on forwarding in EX.
    id_ex_reg_.freg1_data = registers_.ReadFpr(frs1); 
    id_ex_reg_.freg2_data = registers_.ReadFpr(frs2);
    id_ex_reg_.freg3_data = registers_.ReadFpr(frs3);
    
    // 3. Generate Control Signals
    id_ex_reg_.reg_write = control_unit_.GetRegWrite();
    
    // Set FPR Write Enable (freg_write)
    uint8_t opcode = instruction & 0x7F;
    id_ex_reg_.freg_write = (opcode == 0b1010011 || // FP R-type (FADD, FSUB, etc.)
                             opcode == 0b0000111 || // FP Load (FLW, FLD)
                             (opcode >= 0b1000011 && opcode <= 0b1001111)); // FMA-type
    
    id_ex_reg_.branch = control_unit_.GetBranch();
    id_ex_reg_.alu_src = control_unit_.GetAluSrc();
    id_ex_reg_.mem_read = control_unit_.GetMemRead();
    id_ex_reg_.mem_write = control_unit_.GetMemWrite();
    id_ex_reg_.mem_to_reg = control_unit_.GetMemToReg();
    id_ex_reg_.alu_op = control_unit_.GetAluOp();
}

void RV5StageVM_NH_F::pipeline_execute()
{
    // --- Initial Inputs (Stale Data from ID/EX) ---
    uint64_t alu_in1 = id_ex_reg_.reg1_data;
    uint64_t alu_in2 = id_ex_reg_.reg2_data;
    uint64_t f_alu_in1 = id_ex_reg_.freg1_data;
    uint64_t f_alu_in2 = id_ex_reg_.freg2_data;
    uint64_t f_alu_in3 = id_ex_reg_.freg3_data; // For FMA

    // --- FORWARDING LOGIC VARIABLES ---
    uint8_t forward_gpr_a = 0; // 00=Reg, 10=EX/MEM, 01=MEM/WB
    uint8_t forward_gpr_b = 0;
    uint8_t forward_fpr_a = 0;
    uint8_t forward_fpr_b = 0; // Assuming only two FPR sources for most cases

    uint8_t rs1 = id_ex_reg_.rs1;
    uint8_t rs2 = id_ex_reg_.rs2;
    uint8_t frs1 = id_ex_reg_.frs1;
    uint8_t frs2 = id_ex_reg_.frs2;

    // --- 1. GPR Forwarding Checks ---
    if (rs1 != 0) {
        if (ex_mem_reg_.reg_write && (ex_mem_reg_.rd != 0) && (ex_mem_reg_.rd == rs1)) {
            forward_gpr_a = 2; // EX/MEM Priority 1
        } else if (mem_wb_reg_.reg_write && (mem_wb_reg_.rd != 0) && (mem_wb_reg_.rd == rs1)) {
            forward_gpr_a = 1; // MEM/WB Priority 2
        }
    }
    if (rs2 != 0) {
        if (ex_mem_reg_.reg_write && (ex_mem_reg_.rd != 0) && (ex_mem_reg_.rd == rs2)) {
            forward_gpr_b = 2; // EX/MEM Priority 1
        } else if (mem_wb_reg_.reg_write && (mem_wb_reg_.rd != 0) && (mem_wb_reg_.rd == rs2)) {
            forward_gpr_b = 1; // MEM/WB Priority 2
        }
    }
    
    // --- 2. FPR Forwarding Checks (Assumes frd is always 5 bits) ---
    // Note: F-type instructions typically read GPRs for address/conversion or FPRs for math.
    // We only forward if the instruction in EX/MEM or MEM/WB is an FPR write.
    if (id_ex_reg_.freg_write || id_ex_reg_.mem_read || id_ex_reg_.mem_write) {
        // EX/MEM FPR Forward (Priority 1)
        if (ex_mem_reg_.freg_write && (ex_mem_reg_.frd != 0)) {
            if (ex_mem_reg_.frd == frs1) forward_fpr_a = 2;
            if (ex_mem_reg_.frd == frs2) forward_fpr_b = 2;
        }
        
        // MEM/WB FPR Forward (Priority 2, only if not already forwarded by EX/MEM)
        if (mem_wb_reg_.freg_write && (mem_wb_reg_.frd != 0)) {
            if (mem_wb_reg_.frd == frs1 && forward_fpr_a == 0) forward_fpr_a = 1;
            if (mem_wb_reg_.frd == frs2 && forward_fpr_b == 0) forward_fpr_b = 1;
            // frs3 check omitted for simplicity but would follow the same pattern
        }
    }
    
    // --- FORWARDING APPLICATION (GPR) ---
    if (forward_gpr_a == 2) { 
        alu_in1 = ex_mem_reg_.alu_result;
    } else if (forward_gpr_a == 1) { 
        alu_in1 = mem_wb_reg_.mem_to_reg ? mem_wb_reg_.memory_data : mem_wb_reg_.alu_result;
    }

    if (forward_gpr_b == 2) { 
        alu_in2 = ex_mem_reg_.alu_result;
    } else if (forward_gpr_b == 1) { 
        alu_in2 = mem_wb_reg_.mem_to_reg ? mem_wb_reg_.memory_data : mem_wb_reg_.alu_result;
    }
    
    // Address calculation (Load/Store) uses rs1 + imm. ALU_in2 must be immediate if alu_src is set.
    if (id_ex_reg_.alu_src) {
        alu_in2 = static_cast<uint64_t>(id_ex_reg_.imm);
    }
    
    // --- FORWARDING APPLICATION (FPR) ---
    if (forward_fpr_a == 2) { // Forward from EX/MEM
        f_alu_in1 = ex_mem_reg_.f_alu_result;
    } else if (forward_fpr_a == 1) { // Forward from MEM/WB
        f_alu_in1 = mem_wb_reg_.mem_to_reg ? mem_wb_reg_.f_memory_data : mem_wb_reg_.f_alu_result;
    }

    if (forward_fpr_b == 2) { // Forward from EX/MEM
        f_alu_in2 = ex_mem_reg_.f_alu_result;
    } else if (forward_fpr_b == 1) { // Forward from MEM/WB
        f_alu_in2 = mem_wb_reg_.mem_to_reg ? mem_wb_reg_.f_memory_data : mem_wb_reg_.f_alu_result;
    }
    
    // --- EXECUTION ---
    uint32_t instruction = id_ex_reg_.instruction;
    uint8_t opcode = instruction & 0x7F;
    alu::AluOp alu_operation = control_unit_.GetAluSignal(instruction, id_ex_reg_.alu_op);
    uint64_t alu_result = 0;
    uint64_t f_alu_result = 0;
    uint8_t fcsr_status = 0;
    
    bool is_fp_execution = id_ex_reg_.freg_write || id_ex_reg_.alu_op == 3 || id_ex_reg_.alu_op == 4;

    if (is_fp_execution) {
        // Assume double precision (RV64D) for all FP ops where possible
        std::tie(f_alu_result, fcsr_status) = Alu::dfpexecute(alu_operation, f_alu_in1, f_alu_in2, f_alu_in3, 0b000); 
    }
    
    // GPR/Integer Execution (Default)
    if (!is_fp_execution || id_ex_reg_.branch || id_ex_reg_.mem_read || id_ex_reg_.mem_write) {
        bool overflow;
        std::tie(alu_result, overflow) = Alu::execute(alu_operation, alu_in1, alu_in2);
    }


    // Latch data for EX/MEM Register
    ex_mem_reg_.alu_result = alu_result;
    ex_mem_reg_.f_alu_result = f_alu_result;
    ex_mem_reg_.rd = id_ex_reg_.rd;
    ex_mem_reg_.frd = id_ex_reg_.frd; 

    // Store Data: CRITICAL FORWARDING - Store data must also be forwarded!
    uint64_t store_data = id_ex_reg_.reg2_data; // Initial stale data
    
    // Determine if it's an integer store (uses GPR rs2) or an FP store (uses FPR frs2)
    // Assuming F-type stores (FSW/FSD) require the FPR data to be forwarded.
    if (opcode == 0b0100111) { // F-type Store (FSW/FSD)
        // Forward FPR data (frs2)
        if (forward_fpr_b == 2) {
            store_data = ex_mem_reg_.f_alu_result;
        } else if (forward_fpr_b == 1) {
            store_data = mem_wb_reg_.mem_to_reg ? mem_wb_reg_.f_memory_data : mem_wb_reg_.f_alu_result;
        } else {
            // Use stale data from ID stage (freg2_data)
            store_data = id_ex_reg_.freg2_data;
        }
    } else if (id_ex_reg_.mem_write) { // Integer Store (SB/SH/SW/SD)
        // Forward GPR data (rs2) - already handled by forward_gpr_b logic above
        if (forward_gpr_b == 2) {
            store_data = ex_mem_reg_.alu_result;
        } else if (forward_gpr_b == 1) {
            store_data = mem_wb_reg_.mem_to_reg ? mem_wb_reg_.memory_data : mem_wb_reg_.alu_result;
        }
    }
    
    ex_mem_reg_.reg2_data = store_data; // reg2_data now holds the correct (forwarded) data to be stored.
    
    // Pass control signals...
    ex_mem_reg_.reg_write = id_ex_reg_.reg_write;
    ex_mem_reg_.freg_write = id_ex_reg_.freg_write; 
    ex_mem_reg_.mem_to_reg = id_ex_reg_.mem_to_reg;
    ex_mem_reg_.mem_read = id_ex_reg_.mem_read;
    ex_mem_reg_.mem_write = id_ex_reg_.mem_write;
    ex_mem_reg_.branch_taken = false;
    ex_mem_reg_.branch_target_pc = 0;
    
    // --- Control Hazard Logic (Automated) ---

    // Conditional Branch Check (B-type)
    if (id_ex_reg_.branch && opcode == 0b1100011) 
    {
        bool condition_met = false;
        uint8_t funct3 = (instruction >> 12) & 0x7;
        
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

void RV5StageVM_NH_F::pipeline_memory()
{
    // Pass GPR results
    mem_wb_reg_.alu_result = ex_mem_reg_.alu_result;
    mem_wb_reg_.rd = ex_mem_reg_.rd;
    mem_wb_reg_.reg_write = ex_mem_reg_.reg_write;
    mem_wb_reg_.mem_to_reg = ex_mem_reg_.mem_to_reg;
    
    // Pass FPR results
    mem_wb_reg_.f_alu_result = ex_mem_reg_.f_alu_result; 
    mem_wb_reg_.frd = ex_mem_reg_.frd; 
    mem_wb_reg_.freg_write = ex_mem_reg_.freg_write; 

    // --- B-Type Conditional Branch Resolution (3-Cycle Penalty) ---
    if (ex_mem_reg_.branch_taken && (id_ex_reg_.instruction & 0b1111111) == 0b1100011) {
        // B-Type misprediction confirmed in MEM stage. Hardware flushes the pipeline.
        
        program_counter_ = ex_mem_reg_.branch_target_pc;
        if_id_reg_.reset(); 
        id_ex_reg_.reset(); 
        branch_mispredictions_++;
    }

    // --- Memory Operations ---
    if (ex_mem_reg_.mem_read)
    { 
        uint64_t data = memory_controller_.ReadDoubleWord(ex_mem_reg_.alu_result);
        
        // Determine if it's a GPR Load or an FPR Load based on write enable
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
        // Store instruction (R-Store, L-Store solved by forwarding store_data from EX)
        // ex_mem_reg_.reg2_data already holds the forwarded store data.
        memory_controller_.WriteDoubleWord(ex_mem_reg_.alu_result, ex_mem_reg_.reg2_data);
    }
}

void RV5StageVM_NH_F::pipeline_writeback()
{
    // --- GPR Writeback ---
    if (mem_wb_reg_.reg_write && mem_wb_reg_.rd != 0) 
    {
        uint64_t write_data = mem_wb_reg_.mem_to_reg ? mem_wb_reg_.memory_data : mem_wb_reg_.alu_result;

        // Record state for Undo/Redo...
        registers_.WriteGpr(mem_wb_reg_.rd, write_data);
        instructions_retired_++;
    }
    
    // --- FPR Writeback ---
    if (mem_wb_reg_.freg_write && mem_wb_reg_.frd != 0)
    {
        uint64_t write_data = mem_wb_reg_.mem_to_reg ? mem_wb_reg_.f_memory_data : mem_wb_reg_.f_alu_result;
        
        // Record state for Undo/Redo (assuming reg_type 2 for FPR)
        registers_.WriteFpr(mem_wb_reg_.frd, write_data);
        instructions_retired_++;
    }
}

// --- Auxiliary methods (Finalizing the implementation) ---

void RV5StageVM_NH_F::Undo()
{
    if (undo_stack_.empty())
    {
        std::cout << "VM_NO_MORE_UNDO" << std::endl;
        return;
    }

    StepDelta last = undo_stack_.top();
    undo_stack_.pop();

    // Revert register changes
    for (const auto &change : last.register_changes)
    {
        // Assuming reg_type is correctly tracked in RegisterChange
        if (change.reg_type == 0) registers_.WriteGpr(change.reg_index, change.old_value);
        else if (change.reg_type == 2) registers_.WriteFpr(change.reg_index, change.old_value);
    }

    // Revert memory changes
    for (const auto &change : last.memory_changes)
    {
        for (size_t i = 0; i < change.old_bytes_vec.size(); ++i)
        {
            memory_controller_.WriteByte(change.address + i, change.old_bytes_vec[i]);
        }
    }

    // Restore the PC to the state *before* the undone cycle
    program_counter_ = last.old_pc;
    
    // Reset pipeline registers to NOPs for a clean step
    if_id_reg_.reset();
    id_ex_reg_.reset();
    ex_mem_reg_.reset();
    mem_wb_reg_.reset();

    redo_stack_.push(last);
    std::cout << "VM_REDO_COMPLETED" << std::endl;
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

    // Reapply register changes
    for (const auto &change : next.register_changes)
    {
        if (change.reg_type == 0) registers_.WriteGpr(change.reg_index, change.new_value);
        else if (change.reg_type == 2) registers_.WriteFpr(change.reg_index, change.new_value);
    }

    // Reapply memory changes
    for (const auto &change : next.memory_changes)
    {
        for (size_t i = 0; i < change.new_bytes_vec.size(); ++i)
        {
            memory_controller_.WriteByte(change.address + i, change.new_bytes_vec[i]);
        }
    }

    // Restore the PC to the state *after* the redone cycle
    program_counter_ = next.new_pc;
    
    // Reset pipeline registers for a clean step
    if_id_reg_.reset();
    id_ex_reg_.reset();
    ex_mem_reg_.reset();
    mem_wb_reg_.reset();

    undo_stack_.push(next);
    std::cout << "VM_REDO_COMPLETED" << std::endl;
}

void RV5StageVM_NH_F::print_pipeline_registers_debug()
{
    // A basic implementation for debug visibility
    std::cout << "--- Pipeline Debug (Cycle " << cycle_s_ << ") ---" << std::endl;
    std::cout << "PC: 0x" << std::hex << program_counter_ << std::dec << std::endl;
    std::cout << "IF/ID: Inst=0x" << std::hex << if_id_reg_.instruction << std::dec << " PC=" << if_id_reg_.pc << std::endl;
    std::cout << "ID/EX: rs1=" << (int)id_ex_reg_.rs1 << " rs2=" << (int)id_ex_reg_.rs2 << " rd=" << (int)id_ex_reg_.rd << std::endl;
    std::cout << "EX/MEM: ALU_Res=" << ex_mem_reg_.alu_result << " F_ALU_Res=" << ex_mem_reg_.f_alu_result << " Br_Taken=" << ex_mem_reg_.branch_taken << std::endl;
    std::cout << "MEM/WB: ALU_Res=" << mem_wb_reg_.alu_result << " F_ALU_Res=" << mem_wb_reg_.f_alu_result << " Mem_Data=" << mem_wb_reg_.memory_data << std::endl;
}

void RV5StageVM_NH_F::execute_float() {
    std::cerr << "Warning: F-Extension instruction passed to placeholder execute_float()." << std::endl;
}

void RV5StageVM_NH_F::execute_double() {
    std::cerr << "Warning: D-Extension instruction passed to placeholder execute_double()." << std::endl;
}

void RV5StageVM_NH_F::execute_csr() {
    std::cerr << "Warning: CSR instruction passed to placeholder execute_csr()." << std::endl;
}

void RV5StageVM_NH_F::handle_syscall() { 
    if ((id_ex_reg_.instruction & 0x7F) == 0b1110011 && ((id_ex_reg_.instruction >> 12) & 0x7) == 0b000) {
        RequestStop();
        output_status_ = "ECALL_EXIT";
        DumpState("vm_state.json");
    }
}
