/**
 * @file rv5svm_h_f.cpp
 * @brief Implementation for the 5-stage pipelined VM (RV5S) in Mode 4: Hazard Detection, With Forwarding.
 * * NOTE: This is a PURE INTEGER (GPR) implementation.
 * * Stall Rule: Only Load-Use GPR dependencies require a 1-cycle stall. All others are solved by forwarding.
 * * Control Hazards (JAL/JALR, B-Type) are AUTOMATICALLY handled by flush/redirect.
 * @author Atharva and Harshit
 */
#include "vm/rv5s_vms/rv5svm_h_f.h" // Assuming this header now defines RV5StageVM_H_F
#include "common/instructions.h" 
#include "config.h"              
#include "vm/alu.h"
#include "vm/vm_base.h" 
#include "vm/pipeline_registers.h" 
#include "ui/processor_designs/rv5svm_h_f_circuit_scene.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <tuple>
#include <algorithm>

// NOP instruction: ADDI x0, x0, 0 (Used for flushing)
constexpr uint32_t NOP = 0x00000013;

using namespace alu;

// Define stall count constants (used internally, defined in rv5s_hdu.h)
//constexpr int STALL_NONE = 0;
//constexpr int STALL_ONE_CYCLE = 1; 

// --- RV5StageVM_H_F Class Implementation ---

RV5StageVM_H_F::RV5StageVM_H_F() : RV5StageVM_Base()
{
    // Initialize VmBase members
    // program_counter_ = 0;
    // instructions_retired_ = 0;
    // cycle_s_ = 0;
    // stall_cycles_ = 0; 
    
    // // Initialize local members
    // stall_fetch_and_decode_ = false;

    // Reset components and history
    circuit_scene_ = std::make_unique<Kites::RV5StageVM_H_F_CircuitScene>();
    connect(this, &VmBase::updateCircuitStateSignal,
            circuit_scene_.get(), &Kites::RV5StageVM_H_F_CircuitScene::updateCircuitState);
    Reset();
}



void RV5StageVM_H_F::SetActiveWireNames(){}

void RV5StageVM_H_F::Reset()
{
    program_counter_ = 0;
    instructions_retired_ = 0;
    cycle_s_ = 0;
    //stop_requested_ = false;
    stall_fetch_and_decode_ = false;
    stall_cycles_ = 0; // Initialize stall counter

    registers_.Reset();
    memory_controller_.Reset();
    control_unit_.Reset();

    if_id_reg_.reset();
    id_ex_reg_.reset();
    ex_mem_reg_.reset();
    mem_wb_reg_.reset();
    vm_state_.clear();

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
        stalls_needed = check_data_hazard(if_id_reg_, id_ex_reg_,ex_mem_reg_, true /* is_forwarding_enabled */);
        
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
    
    
    // Fetch the instruction at the committed PC address.
    pipeline_fetch();
    // 3. PC Update and Fetch
    uint64_t next_pc = program_counter_; 
    
    // Only advance the PC if we were not stalling this cycle (and thus fetched an instruction).
    // Note: Control hazards (JAL/Branch) already override program_counter_ in EX/MEM.
    if (!stall_fetch_and_decode_ && next_pc == old_pc_before_redirect) {
        next_pc = old_pc_before_redirect + 4;
    }
    
    // Commit the new PC for the Fetch stage
    program_counter_ = next_pc; 
    
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
    // if (program_counter_ >= program_size_ && id_ex_reg_.instruction == NOP) {
    //     RequestStop();
    // }
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

    // Latch data for the ID/EX register
    id_ex_reg_.pc = if_id_reg_.pc;
    id_ex_reg_.instruction = instruction;
    id_ex_reg_.imm = ImmGenerator(instruction);

    // Extract GPR register numbers
    id_ex_reg_.rs1 = (instruction >> 15) & 0x1F;
    id_ex_reg_.rs2 = (instruction >> 20) & 0x1F;
    id_ex_reg_.rd = (instruction >> 7) & 0x1F;
    
    // Read GPR register data naively (Data will be overwritten by forwarding logic)
    id_ex_reg_.reg1_data = registers_.ReadGpr(id_ex_reg_.rs1);
    id_ex_reg_.reg2_data = registers_.ReadGpr(id_ex_reg_.rs2);

    // Pass all control signals to the next stage
    id_ex_reg_.reg_write = control_unit_.GetRegWrite();
    // FPR control signals are omitted for pure integer
    
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

    if(id_ex_reg_.mem_write)
    {
        std::cout << "we have store instruction\n";
        std::cout << (int)id_ex_reg_.rs1 << " " << (int)id_ex_reg_.rs2 << std::endl;
        std::cout << (int)ex_mem_reg_.rd << " " << (int)mem_wb_reg_.prev_rd << std::endl;
        std::cout << (int)alu_in1 << " " << (int)alu_in2 << std::endl;

        std::cout << (ex_mem_reg_.reg_write ? "reg_write" : "nononon") << std::endl;
        std::cout << "result" << (int)ex_mem_reg_.alu_result << std::endl;
    }
    // --- GPR FORWARDING LOGIC ---
    uint8_t forward_a = 0; // 10 = EX/MEM, 01 = MEM/WB
    uint8_t forward_b = 0;

    // **Priority 1: EX/MEM Forward** (R-R, R-Store hazard solved)
    if (ex_mem_reg_.reg_write && (ex_mem_reg_.rd != 0)) {
        if (ex_mem_reg_.rd == id_ex_reg_.rs1) forward_a = 2;
        if (ex_mem_reg_.rd == id_ex_reg_.rs2) forward_b = 2;
    }

    // **Priority 2: MEM/WB Forward** (R-R, L-Store hazard solved)
    if (mem_wb_reg_.prev_reg_write && (mem_wb_reg_.prev_rd != 0)) {
        // Forward A from MEM/WB unless EX/MEM is already forwarding
        if (mem_wb_reg_.prev_rd == id_ex_reg_.rs1 && forward_a != 2) forward_a = 1;
        // Forward B from MEM/WB unless EX/MEM is already forwarding
        if (mem_wb_reg_.prev_rd == id_ex_reg_.rs2 && forward_b != 2) forward_b = 1;
    }
    
    // --- FORWARDING APPLICATION ---

    if (forward_a == 2) { 
        alu_in1 = ex_mem_reg_.alu_result;
    } else if (forward_a == 1) { 
        alu_in1 = mem_wb_reg_.prev_mem_to_reg ? mem_wb_reg_.prev_memory_data : mem_wb_reg_.prev_alu_result;
    }

    if (forward_b == 2) { 
        alu_in2 = ex_mem_reg_.alu_result;
    } else if (forward_b == 1) { 
        alu_in2 = mem_wb_reg_.prev_mem_to_reg ? mem_wb_reg_.prev_memory_data : mem_wb_reg_.prev_alu_result;
    }

    // Re-apply immediate value check (must happen AFTER forwarding application)
    if (id_ex_reg_.alu_src) 
    {
        // if(id_ex_reg_.mem_write)
        // {
        //     std::cout << "This is the culprit\n";
        //     alu_in2 += static_cast<uint64_t>(id_ex_reg_.imm);
        // }
        // else
        // { alu_in2 = static_cast<uint64_t>(id_ex_reg_.imm); }

        alu_in2 = static_cast<uint64_t>(id_ex_reg_.imm);
    }
    
    if(id_ex_reg_.mem_write)
    {
        std::cout << "we have store instruction\n";
        std::cout << (int)alu_in1 << " " << (int)alu_in2 << std::endl;
    }

    // --- EXECUTION ---
    uint32_t instruction = id_ex_reg_.instruction;
    alu::AluOp alu_operation = control_unit_.GetAluSignal(instruction, id_ex_reg_.alu_op > 0);
    bool overflow;
    uint64_t alu_result;
    std::tie(alu_result, overflow) = alu::Alu::execute(alu_operation, alu_in1, alu_in2);

    if(id_ex_reg_.rd == 3)
    {
        std::cout << "ALU result:" << (int)alu_result << std::endl;
    }
    // Latch data for EX/MEM Register
    

    // Store Data: CRITICAL FORWARDING - Store data must also be forwarded!
    uint64_t store_data = id_ex_reg_.reg2_data;
    if (forward_b == 2) { store_data = ex_mem_reg_.alu_result; } 
    else if (forward_b == 1) { store_data = mem_wb_reg_.mem_to_reg ? mem_wb_reg_.prev_memory_data : mem_wb_reg_.prev_alu_result; }

    if(id_ex_reg_.mem_write)
    {
        std::cout << "Store data:" << (int)store_data << std::endl;
    }
    ex_mem_reg_.reg2_data = store_data;
    ex_mem_reg_.alu_result = alu_result;
    ex_mem_reg_.rd = id_ex_reg_.rd; 
    // Pass control signals...
    ex_mem_reg_.pc = id_ex_reg_.pc;
    ex_mem_reg_.instruction = instruction;
    ex_mem_reg_.reg_write = id_ex_reg_.reg_write;
    ex_mem_reg_.mem_to_reg = id_ex_reg_.mem_to_reg;
    ex_mem_reg_.mem_read = id_ex_reg_.mem_read;
    ex_mem_reg_.mem_write = id_ex_reg_.mem_write;
    ex_mem_reg_.branch_taken = false;
    ex_mem_reg_.branch_target_pc = 0;
    
    uint8_t opcode = instruction & 0b1111111;

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
            //program_counter_ = ex_mem_reg_.branch_target_pc;
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

void RV5StageVM_H_F::pipeline_memory()
{
    // --- B-Type Conditional Branch Resolution (3-Cycle Penalty) ---
    if (ex_mem_reg_.branch_taken && (ex_mem_reg_.instruction & 0b1111111) == 0b1100011) {
        // B-Type misprediction confirmed in MEM stage. Hardware flushes the pipeline.
        
        program_counter_ = ex_mem_reg_.branch_target_pc;
        if_id_reg_.reset(); 
        id_ex_reg_.reset(); 
        branch_mispredictions_++;
    }

    // since we are running cycle backward
    // by the time execture check this register forwarding previous results are gone
    // so we store these seperately
    mem_wb_reg_.prev_rd = mem_wb_reg_.rd;
    mem_wb_reg_.prev_alu_result = mem_wb_reg_.alu_result;
    mem_wb_reg_.prev_mem_to_reg = mem_wb_reg_.mem_to_reg;
    mem_wb_reg_.prev_reg_write = mem_wb_reg_.reg_write;
    mem_wb_reg_.prev_memory_data = mem_wb_reg_.memory_data;
    // --- Standard MEM Operations ---
    mem_wb_reg_.pc = ex_mem_reg_.pc;
    mem_wb_reg_.instruction = ex_mem_reg_.instruction;
    mem_wb_reg_.alu_result = ex_mem_reg_.alu_result;
    mem_wb_reg_.rd = ex_mem_reg_.rd;
    mem_wb_reg_.reg_write = ex_mem_reg_.reg_write;
    mem_wb_reg_.mem_to_reg = ex_mem_reg_.mem_to_reg;
    
    // Memory Access
    if (ex_mem_reg_.mem_read)
    { 
        // Load instruction: Result available at end of this stage (Load-Use still needs 1 NOP stall)
        std::cout << "Reading memory at address:" << (int)ex_mem_reg_.alu_result << std::endl;
        mem_wb_reg_.memory_data = memory_controller_.ReadDoubleWord(ex_mem_reg_.alu_result);
        std::cout << "Data read:" << (int)mem_wb_reg_.memory_data << std::endl;
    }
    else if (ex_mem_reg_.mem_write)
    { 
        // Store instruction (Solved by forwarding store_data from EX)
        std::cout << "AHHAHAHAHAHAHAHAAH\n";
        std::cout << (int)ex_mem_reg_.alu_result << ' ' << (int)ex_mem_reg_.reg2_data << std::endl;
        memory_controller_.WriteDoubleWord(ex_mem_reg_.alu_result, ex_mem_reg_.reg2_data);
    }
}

void RV5StageVM_H_F::pipeline_writeback()
{
    // Write the final result back to the register file
    if (mem_wb_reg_.reg_write && mem_wb_reg_.rd != 0) 
    {
        std::cout << "Writing back to register x" << (int)mem_wb_reg_.rd << std::endl;
        std::cout << "Value:" << (int)(mem_wb_reg_.mem_to_reg ? mem_wb_reg_.memory_data : mem_wb_reg_.alu_result) << std::endl;
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
        instructions_retired_++; // Instruction successfully retired
    }
}

// --- Auxiliary methods (Full implementation) ---

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
        if (change.reg_type == 0) registers_.WriteGpr(change.reg_index, change.old_value);
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
        if (change.reg_type == 0) registers_.WriteGpr(change.reg_index, change.new_value);
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

// void RV5StageVM_H_F::print_pipeline_registers_debug()
// {
//     // A basic implementation for debug visibility
//     std::cout << "--- Pipeline Debug (Cycle " << cycle_s_ << ") ---" << std::endl;
//     std::cout << "PC: 0x" << std::hex << program_counter_ << std::dec << std::endl;
//     std::cout << "IF/ID: Inst=0x" << std::hex << if_id_reg_.instruction << std::dec << " PC=" << if_id_reg_.pc << std::endl;
//     std::cout << "ID/EX: rs1=" << (int)id_ex_reg_.rs1 << " rs2=" << (int)id_ex_reg_.rs2 << " rd=" << (int)id_ex_reg_.rd << std::endl;
//     std::cout << "EX/MEM: ALU_Res=" << ex_mem_reg_.alu_result << " Br_Taken=" << ex_mem_reg_.branch_taken << std::endl;
//     std::cout << "MEM/WB: ALU_Res=" << mem_wb_reg_.alu_result << " Mem_Data=" << mem_wb_reg_.memory_data << std::endl;
// }

// void RV5StageVM_H_F::execute_float() {
//     std::cerr << "Warning: F-Extension instruction passed to placeholder execute_float()." << std::endl;
// }

// void RV5StageVM_H_F::execute_double() {
//     std::cerr << "Warning: D-Extension instruction passed to placeholder execute_double()." << std::endl;
// }

// void RV5StageVM_H_F::execute_csr() {
//     std::cerr << "Warning: CSR instruction passed to placeholder execute_csr()." << std::endl;
// }

void RV5StageVM_H_F::handle_syscall() 
{ 
    if ((id_ex_reg_.instruction & 0x7F) == 0b1110011 && ((id_ex_reg_.instruction >> 12) & 0x7) == 0b000) {
        RequestStop();
        output_status_ = "ECALL_EXIT";
        // DumpState("vm_state.json");
    }
}
