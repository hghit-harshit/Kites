/**
 * @file rv5svm_nh_nf.cpp
 * @brief Implementation for the 5-stage pipelined VM (RV5S) in Mode 1: No Hazard Detection, No Forwarding (NH_NF).
 * * Data Hazards (R-R, L-R, R-Store, L-Store) require 2 NOPs (Programmer Responsibility).
 * * Control Hazards (JAL/JALR, B-Type) are AUTOMATICALLY handled by the pipeline.
 * * Control Penalties: JAL/JALR = 1 bubble (EX stage); B-Type = 2 bubbles (MEM stage, 3-cycle penalty).
 * * @author Atharva and Harshit
 */
#include "vm/rv5s_vms/rv5svm_nh_nf.h"
#include "common/instructions.h" 
#include "config.h"              
#include "vm/alu.h"
#include "vm/vm_base.h" // For ImmGenerator, etc.
#include "ui/processor_designs/rv5svm_nh_nf_circuit_scene.h"

#include <iostream>
#include <thread>
#include <chrono>
#include <tuple>
#include <algorithm>
#include <thread>
#include <chrono>

// NOP instruction: ADDI x0, x0, 0 
constexpr uint32_t NOP = 0x00000013;

// --- VmBase Pure Virtual Method Implementations (Run, DebugRun, Reset, Step) ---
RV5StageVM_NH_NF::RV5StageVM_NH_NF() : RV5StageVM_Base()
{
    // Initialize VmBase members
    // program_counter_ = 0;
    // instructions_retired_ = 0;
    // cycle_s_ = 0;
    // stall_cycles_ = 0; 
    
    // // Initialize local members
    // stall_fetch_and_decode_ = false;

    // Reset components and history
    circuit_scene_ = std::make_unique<Kites::RV5StageVM_NH_NF_CircuitScene>();
    connect(this, &VmBase::updateCircuitStateSignal,
            circuit_scene_.get(), &Kites::RV5StageVM_NH_NF_CircuitScene::updateCircuitState);
    Reset();
}


bool RV5StageVM_NH_NF::is_pipeline_drained() const
{
    // IF/ID and ID/EX registers directly store the instruction word.
    if (if_id_reg_.instruction != NOP) return false;
    if (id_ex_reg_.instruction != NOP) return false;
    
    // EX/MEM: Check for architectural side effects (Reg Write, Mem Read, Mem Write).
    // A NOP will have all these control signals disabled (false).
    if (ex_mem_reg_.reg_write || ex_mem_reg_.mem_read || ex_mem_reg_.mem_write) return false;

    // MEM/WB: Check for final architectural side effect (Reg Write).
    if (mem_wb_reg_.reg_write) return false;
    
    return true;
}

void RV5StageVM_NH_NF::Run()
{
    ClearStop();
    // Continue running until stop is requested OR the pipeline has drained.
    // while (!stop_requested_ && (program_counter_ < program_size_ || id_ex_reg_.instruction != NOP))
    // {
    //     Step();
    // }

    while (!stop_requested_ && (program_counter_ < program_size_ || !is_pipeline_drained()))
    {
        Step();
    }
}

void RV5StageVM_NH_NF::DebugRun()
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
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
    print_pipeline_registers_debug();
}

void RV5StageVM_NH_NF::Reset()
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

void RV5StageVM_NH_NF::Step()
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
    // Fetch the instruction at the committed PC address.
    pipeline_fetch();
    // 2. Determine the next PC (Redirection logic overrides sequential advance)
    uint64_t next_pc = program_counter_; 
    
    // If no redirect happened in EX or MEM, advance sequentially.
    if (next_pc == old_pc_before_redirect) {
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

// --- Pipeline Stage Implementations (Full Proof Control) ---

void RV5StageVM_NH_NF::pipeline_fetch()
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
        //if_id_reg_.reset();
        if_id_reg_.insertNop();
    }
}

void RV5StageVM_NH_NF::pipeline_decode()
{
    // Get instruction from the IF/ID register
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

    // Read register data naively (NH_NF CORE LOGIC: READS STALE DATA)
    // This enforces the 2 NOP requirement for data hazards.
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
    // if (if_id_reg_.pc >= program_size_) {
    //     // Turn everything off — this acts as a bubble
    //     id_ex_reg_.instruction = NOP;   // purely cosmetic, no effect
    //     id_ex_reg_.pc = if_id_reg_.pc;
    //     id_ex_reg_.imm = 0;
    //     id_ex_reg_.rs1 = id_ex_reg_.rs2 = id_ex_reg_.rd = 0;
    //     id_ex_reg_.reg1_data = id_ex_reg_.reg2_data = 0;
    //     id_ex_reg_.reg_write = false;
    //     id_ex_reg_.branch = false;
    //     id_ex_reg_.alu_src = false;
    //     id_ex_reg_.mem_read = false;
    //     id_ex_reg_.mem_write = false;
    //     id_ex_reg_.mem_to_reg = false;
    //     id_ex_reg_.alu_op = 0;

    //     //ex_mem_reg_.alu_result = alu_result;
    //     ex_mem_reg_.rd = id_ex_reg_.rd;
    //     ex_mem_reg_.reg2_data = id_ex_reg_.reg2_data;
    //     ex_mem_reg_.reg_write = id_ex_reg_.reg_write;
    //     ex_mem_reg_.mem_to_reg = id_ex_reg_.mem_to_reg;
    //     ex_mem_reg_.mem_read = id_ex_reg_.mem_read;
    //     ex_mem_reg_.mem_write = id_ex_reg_.mem_write;
    //     ex_mem_reg_.branch_taken = false;
    //     ex_mem_reg_.branch_target_pc = 0;

    //     return;
    // }
    //  uint32_t instruction = if_id_reg_.instruction;
    // // --- Bubble for injected NOPs (end-of-program or flushed) ---
    

    // Select ALU inputs
    uint64_t alu_in1 = id_ex_reg_.reg1_data;
    uint64_t alu_in2 = id_ex_reg_.alu_src ? static_cast<uint64_t>(id_ex_reg_.imm) : id_ex_reg_.reg2_data;

    // Get the specific ALU operation
    alu::AluOp alu_operation = control_unit_.GetAluSignal(id_ex_reg_.instruction, id_ex_reg_.alu_op > 0);

    // Execute the operation
    bool overflow; // Ignored for this simple model
    uint64_t alu_result;
    // ALU operations for integer, F, and D extensions should be called here based on instruction
    // For simplicity, only integer execute is shown, as per the B-type logic dependency.
    std::tie(alu_result, overflow) = alu::Alu::execute(alu_operation, alu_in1, alu_in2);

    // Latch data for EX/MEM Register
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

    // --- Conditional Branch Check (B-type: BLT, BGE, etc.) ---
    if (id_ex_reg_.branch && opcode == 0b1100011) 
    {
        bool condition_met = false;
        uint8_t funct3 = (instruction >> 12) & 0x7;
        
        // This fully implements all six branch conditions using the ALU subtraction/comparison result.
        switch (funct3) {
            case 0b000: condition_met = (alu_result == 0); break; // BEQ (Result of Subtraction is Zero)
            case 0b001: condition_met = (alu_result != 0); break; // BNE (Result of Subtraction is Non-Zero)
            case 0b100: condition_met = (alu_result == 1); break; // BLT (Result of kSlt is 1)
            case 0b101: condition_met = (alu_result == 0); break; // BGE (Result of kSlt is 0 - Not Less Than)
            case 0b110: condition_met = (alu_result == 1); break; // BLTU (Result of kSltu is 1)
            case 0b111: condition_met = (alu_result == 0); break; // BGEU (Result of kSltu is 0 - Not Less Than Unsigned)
        }

        if (condition_met)
        {
            // Misprediction detected. Flag MEM stage to handle the 3-cycle flush.
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

        // 1. Redirect PC (Auto-Advance)
        program_counter_ = jump_target;

        // 2. Kill the next instruction (IF/ID register) to incur the 1-bubble penalty.
        if_id_reg_.reset(); 
        
        // 3. Mark as taken (for link register write)
        ex_mem_reg_.branch_taken = true; 
    }
}

void RV5StageVM_NH_NF::pipeline_memory()
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
    mem_wb_reg_.instruction = ex_mem_reg_.instruction;
    mem_wb_reg_.alu_result = ex_mem_reg_.alu_result;
    mem_wb_reg_.rd = ex_mem_reg_.rd;
    mem_wb_reg_.reg_write = ex_mem_reg_.reg_write;
    mem_wb_reg_.mem_to_reg = ex_mem_reg_.mem_to_reg;
    
    if (ex_mem_reg_.mem_read)
    { 
        mem_wb_reg_.memory_data = memory_controller_.ReadDoubleWord(ex_mem_reg_.alu_result);
    }
    else if (ex_mem_reg_.mem_write)
    { 
        // Store instruction (Uses stale data from ID)
        memory_controller_.WriteDoubleWord(ex_mem_reg_.alu_result, ex_mem_reg_.reg2_data);
    }
}

void RV5StageVM_NH_NF::pipeline_writeback()
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
        std::cout << "Wrote " << write_data << " to x" << mem_wb_reg_.rd << std::endl; // for debugging
        instructions_retired_++; // Instruction successfully retired
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

// --- Debug and Specialized Handlers ---

void RV5StageVM_NH_NF::print_pipeline_registers_debug()
{
    // A basic implementation for debug visibility
    std::cout << "--- Pipeline Debug (Cycle " << cycle_s_ << ") ---" << std::endl;
    std::cout << "PC: 0x" << std::hex << program_counter_ << std::dec << std::endl;

    auto inst_to_mnemonic = [](uint32_t inst) -> const char* {
        if (inst == NOP) return "NOP";
        uint8_t opc = inst & 0x7F;
        switch (opc) {
            case 0b1101111: return "JAL";
            case 0b1100111: return "JALR";
            case 0b1100011: return "BR";
            case 0b0000011: return "LOAD";
            case 0b0100011: return "STORE";
            case 0b0010011: return "ALU_IMM";
            case 0b0110011: return "ALU_REG";
            case 0b1110011: return "SYSTEM";
            default: return "OTHER";
        }
    };

    // Pretty-print pipeline registers in an ASCII box
    // Compact table: Stage | Instruction (hex) | Mnemonic
    auto print_row = [&](const char* stage, uint32_t inst){
        std::cout << "| " << stage << " | 0x" << std::hex << inst << std::dec
                  << " | " << inst_to_mnemonic(inst) << " |\n";
    };

    std::cout << "+-----------------------------------------------+\n";
    std::cout << "| Stage   | Instruction (hex) | Mnemonic         |\n";
    std::cout << "+-----------------------------------------------+\n";
    print_row("IF/ID",  if_id_reg_.instruction);
    print_row("ID/EX",  id_ex_reg_.instruction);
    print_row("EX/MEM", ex_mem_reg_.instruction);
    print_row("MEM/WB", mem_wb_reg_.instruction);
    std::cout << "+-----------------------------------------------+\n";
}

// void RV5StageVM_NH_NF::execute_float() {
//     // Placeholder: In a full implementation, this calls alu::Alu::fpexecute
//     std::cerr << "Warning: F-Extension instruction passed to placeholder execute_float()." << std::endl;
// }

// void RV5StageVM_NH_NF::execute_double() {
//     // Placeholder: In a full implementation, this calls alu::Alu::dfpexecute
//     std::cerr << "Warning: D-Extension instruction passed to placeholder execute_double()." << std::endl;
// }

// void RV5StageVM_NH_NF::execute_csr() {
//     // Placeholder: In a full implementation, this handles Control and Status Register instructions.
//     std::cerr << "Warning: CSR instruction passed to placeholder execute_csr()." << std::endl;
// }

void RV5StageVM_NH_NF::handle_syscall() { 
    if ((id_ex_reg_.instruction & 0x7F) == 0b1110011 && ((id_ex_reg_.instruction >> 12) & 0x7) == 0b000) {
        RequestStop();
        output_status_ = "ECALL_EXIT";
        DumpState("vm_state.json");
    }
}
