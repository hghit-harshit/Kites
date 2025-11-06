/**
 * @file rv5svm_nh_nf.cpp
 * @brief Implementation for the 5-stage pipelined VM (RV5S) with Forwarding.
 * * Data Hazards: Now solved by forwarding, except for Load-Use.
 * * Load-Use Hazard: Requires 1 NOP manually (Programmer Responsibility).
 * * Control Hazards: AUTOMATICALLY handled (JAL/JALR = 1 bubble, B-Type = 2 bubbles).
 * * @author Atharva and Harshit
 */
#include "vm/rv5s_vms/rv5svm_nh_f.h"
#include "common/instructions.h" 
#include "config.h"              
#include "vm/alu.h"
#include "vm/vm_base.h" // For ImmGenerator, etc.

#include <iostream>
#include <thread>
#include <chrono>
#include <tuple>
#include <algorithm>

// NOP instruction: ADDI x0, x0, 0 
constexpr uint32_t NOP = 0x00000013;

// --- VmBase Pure Virtual Method Implementations (Run, DebugRun, Reset, Step) ---

void RV5StageVM_NH_F::Run()
{
    ClearStop();
    // Continue running until stop is requested OR the pipeline has drained.
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

    // Clear history for Undo/Redo
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

// --- Pipeline Stage Implementations (Full Proof Control + Forwarding) ---

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

    // Extract register numbers
    id_ex_reg_.rs1 = (instruction >> 15) & 0x1F;
    id_ex_reg_.rs2 = (instruction >> 20) & 0x1F;
    id_ex_reg_.rd = (instruction >> 7) & 0x1F;

    // Read register data naively (Data will be overwritten by forwarding logic in EX)
    // The base read is still performed, even if it is stale.
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
    // Initial ALU inputs are the stale values read from the register file (ID_EX)
    uint64_t alu_in1 = id_ex_reg_.reg1_data;
    uint64_t alu_in2 = id_ex_reg_.reg2_data;

    // If AluSrc is true (I-type ALU, Load, Store), override reg2_data with the immediate
    if (id_ex_reg_.alu_src) {
        alu_in2 = static_cast<uint64_t>(id_ex_reg_.imm);
    }
    
    // Forwarding values (Source = 00 means no forwarding, use ID_EX value)
    uint8_t forward_a = 0; // 10 = EX/MEM, 01 = MEM/WB
    uint8_t forward_b = 0;

    // --- FORWARDING LOGIC (R-R, R-Store, L-Store resolved here) ---

    // **Priority 1: EX/MEM (Data available from current instruction in MEM)**
    // R-R, R-Store, Load-Store dependencies (I_n-1) -> I_n
    if (ex_mem_reg_.reg_write && (ex_mem_reg_.rd != 0)) {
        if (ex_mem_reg_.rd == id_ex_reg_.rs1) {
            forward_a = 2; // Forward A from EX/MEM ALU result
        }
        if (ex_mem_reg_.rd == id_ex_reg_.rs2) {
            forward_b = 2; // Forward B from EX/MEM ALU result
        }
    }

    // **Priority 2: MEM/WB (Data available from current instruction in WB)**
    // R-R, R-Store, Load-Store dependencies (I_n-2) -> I_n
    if (mem_wb_reg_.reg_write && (mem_wb_reg_.rd != 0)) {
        // Forward A from MEM/WB unless EX/MEM is forwarding to the same register (Priority)
        if (mem_wb_reg_.rd == id_ex_reg_.rs1 && forward_a != 2) {
            forward_a = 1; // Forward A from MEM/WB result (ALU or Memory)
        }
        // Forward B from MEM/WB unless EX/MEM is forwarding to the same register (Priority)
        if (mem_wb_reg_.rd == id_ex_reg_.rs2 && forward_b != 2) {
            forward_b = 1; // Forward B from MEM/WB result (ALU or Memory)
        }
    }
    
    // --- FORWARDING APPLICATION ---

    // Forwarding Source A
    if (forward_a == 2) { // Forward from EX/MEM
        alu_in1 = ex_mem_reg_.alu_result;
    } else if (forward_a == 1) { // Forward from MEM/WB
        alu_in1 = mem_wb_reg_.mem_to_reg ? mem_wb_reg_.memory_data : mem_wb_reg_.alu_result;
    }

    // Forwarding Source B
    // NOTE: Store instructions use rs2's value (`reg2_data`) which is passed through EX/MEM.
    if (forward_b == 2) { // Forward from EX/MEM
        alu_in2 = ex_mem_reg_.alu_result;
    } else if (forward_b == 1) { // Forward from MEM/WB
        alu_in2 = mem_wb_reg_.mem_to_reg ? mem_wb_reg_.memory_data : mem_wb_reg_.alu_result;
    }

    // Re-apply immediate value check after forwarding for ALU_B if needed
    if (id_ex_reg_.alu_src && id_ex_reg_.alu_op != 0) { // If it's an ALU immediate, not Load/Store address calc (ALUOp=0)
        alu_in2 = static_cast<uint64_t>(id_ex_reg_.imm); 
    } else if (id_ex_reg_.alu_src && (id_ex_reg_.mem_read || id_ex_reg_.mem_write)) { 
        // If it's a Load/Store address calculation (ALUOp=0), use the forwarded alu_in2 if it's rs2's data (for Store) 
        // or the immediate if it's the address calculation. Since Load/Store address is rs1 + imm, only rs1 is forwarded.
        // We only override alu_in2 if it was an immediate *and* it wasn't forwarded to (which is correct, forwarding is for register data).
        alu_in2 = id_ex_reg_.alu_src ? static_cast<uint64_t>(id_ex_reg_.imm) : alu_in2;
    }


    // --- EXECUTION (Same as before) ---
    uint32_t instruction = id_ex_reg_.instruction;
    alu::AluOp alu_operation = control_unit_.GetAluSignal(instruction, id_ex_reg_.alu_op > 0);
    bool overflow;
    uint64_t alu_result;
    std::tie(alu_result, overflow) = alu::Alu::execute(alu_operation, alu_in1, alu_in2);

    // Latch data for EX/MEM Register
    ex_mem_reg_.alu_result = alu_result;
    ex_mem_reg_.rd = id_ex_reg_.rd;
    // CRITICAL: The data to be stored (reg2_data for Store) must ALSO be forwarded!
    uint64_t store_data = id_ex_reg_.reg2_data;
    if (forward_b == 2) { // Forward from EX/MEM
        store_data = ex_mem_reg_.alu_result;
    } else if (forward_b == 1) { // Forward from MEM/WB
        store_data = mem_wb_reg_.mem_to_reg ? mem_wb_reg_.memory_data : mem_wb_reg_.alu_result;
    }
    ex_mem_reg_.reg2_data = store_data;
    // Pass control signals... (omitted for brevity, they are all correctly copied)
    ex_mem_reg_.reg_write = id_ex_reg_.reg_write;
    ex_mem_reg_.mem_to_reg = id_ex_reg_.mem_to_reg;
    ex_mem_reg_.mem_read = id_ex_reg_.mem_read;
    ex_mem_reg_.mem_write = id_ex_reg_.mem_write;
    ex_mem_reg_.branch_taken = false;
    ex_mem_reg_.branch_target_pc = 0;
    
    // ... (Control Hazard Logic remains the same, omitted for brevity) ...

    uint8_t opcode = instruction & 0b1111111;

    // --- Conditional Branch Check (B-type: BLT, BGE, etc.) ---
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
    // --- Unconditional Jump Check (JAL/JALR: 1-cycle penalty) ---
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
    // --- B-Type Conditional Branch Resolution (3-Cycle Penalty) ---
    if (ex_mem_reg_.branch_taken && (id_ex_reg_.instruction & 0b1111111) == 0b1100011) {
        // B-Type misprediction confirmed in MEM stage. Hardware flushes the pipeline.
        
        // 1. Redirect the fetch PC
        program_counter_ = ex_mem_reg_.branch_target_pc;

        // 2. Kill the two instructions in the front end (IF/ID and ID/EX) to incur the 2-bubble penalty.
        if_id_reg_.reset(); 
        id_ex_reg_.reset(); 
        
        branch_mispredictions_++;
    }

    // --- Standard MEM Operations ---
    mem_wb_reg_.alu_result = ex_mem_reg_.alu_result;
    mem_wb_reg_.rd = ex_mem_reg_.rd;
    mem_wb_reg_.reg_write = ex_mem_reg_.reg_write;
    mem_wb_reg_.mem_to_reg = ex_mem_reg_.mem_to_reg;
    
    if (ex_mem_reg_.mem_read)
    { 
        // Load instruction: Result available at end of this stage (Load-Use still needs 1 NOP)
        mem_wb_reg_.memory_data = memory_controller_.ReadDoubleWord(ex_mem_reg_.alu_result);
    }
    else if (ex_mem_reg_.mem_write)
    { 
        // Store instruction (R-Store, L-Store solved by forwarding store_data from EX)
        memory_controller_.WriteDoubleWord(ex_mem_reg_.alu_result, ex_mem_reg_.reg2_data);
    }
}

void RV5StageVM_NH_F::pipeline_writeback()
{
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
        instructions_retired_++; // Instruction successfully retired
    }
}

// --- Auxiliary methods (omitted for brevity) ---
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
        registers_.WriteGpr(change.reg_index, change.old_value);
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
        registers_.WriteGpr(change.reg_index, change.new_value);
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
    std::cout << "EX/MEM: ALU_Res=" << ex_mem_reg_.alu_result << " Br_Taken=" << ex_mem_reg_.branch_taken << std::endl;
    std::cout << "MEM/WB: ALU_Res=" << mem_wb_reg_.alu_result << " Mem_Data=" << mem_wb_reg_.memory_data << std::endl;
}

// void RV5StageVM_NH_F::execute_float() {
//     // Placeholder: In a full implementation, this calls alu::Alu::fpexecute
//     std::cerr << "Warning: F-Extension instruction passed to placeholder execute_float()." << std::endl;
// }

// void RV5StageVM_NH_F::execute_double() {
//     // Placeholder: In a full implementation, this calls alu::Alu::dfpexecute
//     std::cerr << "Warning: D-Extension instruction passed to placeholder execute_double()." << std::endl;
// }

// void RV5StageVM_NH_F::execute_csr() {
//     // Placeholder: In a full implementation, this handles Control and Status Register instructions.
//     std::cerr << "Warning: CSR instruction passed to placeholder execute_csr()." << std::endl;
// }

void RV5StageVM_NH_F::handle_syscall() { 
    if ((id_ex_reg_.instruction & 0x7F) == 0b1110011 && ((id_ex_reg_.instruction >> 12) & 0x7) == 0b000) {
        RequestStop();
        output_status_ = "ECALL_EXIT";
        DumpState("vm_state.json");
    }
}
