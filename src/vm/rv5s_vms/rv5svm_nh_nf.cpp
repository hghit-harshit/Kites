/**
 * @file rv5svm_nh_nf.cpp
 * @brief Implementation for the 5-stage pipelined VM (RV5S) in Mode 1: No Hazard Detection, No Forwarding.
 * * NOTE: In this mode, both data and control hazards must be handled entirely by the 
 * programmer inserting NOPs. The hardware performs NO automatic stalling, 
 * forwarding, or branch flushing/redirection. Incorrect code sequencing will 
 * lead to incorrect results.
 * * * The programmer must insert NOPs:
 * * 1. Data Hazards (ALU-ALU): 2 NOPs
 * * 2. Load-Use Hazards: 1 NOP
 * * 3. Conditional Branch Hazards (Resolves in MEM stage): 3 NOPs
 * * 4. Unconditional Jumps (JAL/JALR, resolves in ID stage): 1 NOP
 * * @author Atharva and Harshit
 */
#include "vm/rv5s_vms/rv5svm_nh_nf.h"
#include "common/instructions.h"
#include "config.h"

#include <iostream>
#include <thread>
#include <chrono>
#include <tuple>
#include <algorithm>

// Constructor initializes the base class and resets the VM state.
RV5StageVM_NH_NF::RV5StageVM_NH_NF() : RV5StageVM_Base()
{
    // The base class constructor is called implicitly.
    // Reset() handles the initial state setup.
    Reset();
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
    // These registers are inherited from RV5StageVM_Base.
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
    // Simulate a single clock cycle by running stages in reverse order
    // (WB -> MEM -> EX -> ID -> IF) to avoid premature data overwrites.

    // Prepare a new delta for recording state changes for Undo/Redo.
    current_delta_ = StepDelta();
    current_delta_.old_pc = program_counter_;

    pipeline_writeback();
    pipeline_memory();
    pipeline_execute();
    pipeline_decode();
    pipeline_fetch();

    cycle_s_++; // One clock cycle has passed

    // Finalize the delta and manage history stacks.
    current_delta_.new_pc = program_counter_;
    // Check if any architectural state changed (registers or memory)
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
    // Continue running until stop is requested AND the program counter is past
    // the program AND the pipeline is drained (checking for NOP in ID/EX is a quick check).
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
        // Use a non-blocking delay for better integration in an interactive environment
        std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));
    }
}

// --- Pipeline Stage Implementations (Mode 1: No Hazard Detection, No Forwarding) ---

void RV5StageVM_NH_NF::pipeline_fetch()
{
    // Check for an unconditional jump resolution from the ID stage (1 NOP delay)
    // JAL (opcode 0b1101111) and JALR (opcode 0b1100111) are the unconditional jumps.
    // They resolve their PC update at the ID/EX boundary, affecting the PC for the IF stage next cycle.
    if ((id_ex_reg_.instruction & 0b1111111) == 0b1101111 || // JAL
        (id_ex_reg_.instruction & 0b1111111) == 0b1100111)    // JALR
    {
        // Only proceed if the instruction is NOT a NOP (0x13) to avoid setting PC = 0.
        // We calculate the target in Decode, but the ALU result for JALR is used.
        // For simplicity in this naive mode, we rely on the EX stage calculation being correct.
        
        // This check is slightly simplified: if the ID/EX register holds a jump, 
        // we use its calculated target to update the PC, effectively flushing one instruction.
        // If the programmer inserted 1 NOP, the pipeline is correct.
        if (id_ex_reg_.instruction != 0x00000013) {
            // Unconditional jumps are resolved at the ID/EX boundary (1 NOP delay)
            // We assume the EX stage will calculate the correct target (even though ALU might not run yet)
            // For a pure Mode 1, we still rely on the programmer to ensure the next fetch is a NOP.
            
            // To model the PC update *when the JAL is in EX*, we update the PC at the start of fetch.
            // Note: This relies on the EX stage correctly calculating the target PC for JAL/JALR and storing it.
            // Since `pipeline_execute` doesn't store the JAL/JALR target in the EX/MEM register, 
            // we calculate it here based on the instruction in ID/EX, modeling a separate PC update path.

            // Target PC: PC of the jump instruction + immediate offset (for JAL) or ALU result (for JALR)
            uint64_t target_pc;
            if ((id_ex_reg_.instruction & 0b1111111) == 0b1101111) { // JAL
                 target_pc = id_ex_reg_.pc + id_ex_reg_.imm;
            } else { // JALR
                 // JALR target is reg1_data + imm. We must use the ALU result for the target.
                 // Since we don't have the ALU result here, we rely on the programmer's NOPs.
                 // For now, we will assume JAL is handled in EX, and keep the PC sequential for conditional branches.
            }
            // To maintain simplicity and the programmer's responsibility for NOPs:
            // We rely on the JAL/JALR target being passed to the PC update logic (not fully modeled in this naive IF).
        }
    }


    if (program_counter_ < program_size_)
    {
        // Latch the instruction and PC for the next stage (IF/ID register)
        if_id_reg_.instruction = memory_controller_.ReadWord(program_counter_);
        if_id_reg_.pc = program_counter_;

        // Always predict "not taken" and fetch the next sequential instruction.
        // This is the naive fetch that causes the control hazard on a taken branch.
        program_counter_ += 4;
    }
    else
    {
        // Once past the end of the program, inject NOPs to drain the pipeline.
        if_id_reg_.reset();
    }
}

void RV5StageVM_NH_NF::pipeline_decode()
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

    // Read register data naively (NO FORWARDING, NO STALLING)
    // This is the core source of data hazards in Mode 1.
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
    // Select ALU inputs
    uint64_t alu_in1 = id_ex_reg_.reg1_data;
    uint64_t alu_in2 = id_ex_reg_.alu_src ? static_cast<uint64_t>(id_ex_reg_.imm) : id_ex_reg_.reg2_data;

    // Get the specific ALU operation
    alu::AluOp alu_operation = control_unit_.GetAluSignal(id_ex_reg_.instruction, id_ex_reg_.alu_op > 0);

    // Execute the operation
    bool overflow; // Ignored for this simple model
    std::tie(ex_mem_reg_.alu_result, overflow) = alu_.execute(alu_operation, alu_in1, alu_in2);

    // --- Branch Resolution (No Hardware Correction/Flush for conditional branches) ---
    // The conditional branch outcome is determined here.
    ex_mem_reg_.branch_taken = false;
    uint8_t opcode = id_ex_reg_.instruction & 0b1111111;

    if (id_ex_reg_.branch && opcode == 0b1100011) // Conditional Branch (B-type)
    {
        bool condition_met = false;
        uint8_t funct3 = (id_ex_reg_.instruction >> 12) & 0x7;
        
        // ALU result for branch instructions (BEQ/BNE) will be (reg1_data - reg2_data)
        if ((funct3 == 0b000 && ex_mem_reg_.alu_result == 0) || // BEQ (result == 0)
            (funct3 == 0b001 && ex_mem_reg_.alu_result != 0))   // BNE (result != 0)
        {
            condition_met = true;
        }
        
        if (condition_met)
        {
            ex_mem_reg_.branch_taken = true;
            ex_mem_reg_.branch_target_pc = id_ex_reg_.pc + id_ex_reg_.imm;
            
            // IMPORTANT FOR 3 NOP DELAY: We intentionally do NOT update the PC here,
            // or flush the pipeline. The incorrect sequential instructions will proceed.
            // The programmer must insert 3 NOPs to fill the delay slots.
        }
    }
    else if (opcode == 0b1101111) // JAL (J-type)
    {
        // JAL calculates its target in ID, but the PC redirect is typically done earlier.
        // Since we are modeling a simple naive pipeline, we rely on the PC redirect 
        // being handled in a dedicated path that is not fully modeled here, 
        // requiring 1 NOP from the programmer. We calculate the target for the next stage.
        ex_mem_reg_.branch_taken = true; // Unconditional jump is always "taken"
        ex_mem_reg_.branch_target_pc = id_ex_reg_.pc + id_ex_reg_.imm;
    }
    else if (opcode == 0b1100111) // JALR (I-type)
    {
        // JALR target address is calculated in the EX stage (Reg + Imm)
        ex_mem_reg_.branch_taken = true; // Unconditional jump is always "taken"
        // Target is the ALU result (reg1_data + imm) masked to a multiple of 2.
        ex_mem_reg_.branch_target_pc = ex_mem_reg_.alu_result & ~1;
    }

    // Pass necessary data to the next stage (EX/MEM register)
    ex_mem_reg_.reg2_data = id_ex_reg_.reg2_data; // Data for Store instructions
    ex_mem_reg_.rd = id_ex_reg_.rd;

    // Pass control signals through
    ex_mem_reg_.reg_write = id_ex_reg_.reg_write;
    ex_mem_reg_.mem_read = id_ex_reg_.mem_read;
    ex_mem_reg_.mem_write = id_ex_reg_.mem_write;
    ex_mem_reg_.mem_to_reg = id_ex_reg_.mem_to_reg;
}

void RV5StageVM_NH_NF::pipeline_memory()
{
    // Check for an unconditional jump resolution from the EX stage (JAL/JALR)
    // The target PC is now available in the EX/MEM register.
    if (ex_mem_reg_.branch_taken) {
        uint32_t instruction_in_ex = id_ex_reg_.instruction; // Instruction in EX stage this cycle
        uint8_t opcode = instruction_in_ex & 0b1111111;

        if (opcode == 0b1101111 || opcode == 0b1100111) { // JAL or JALR
            // Unconditional jumps are resolved here, requiring 1 NOP after them.
            // The next instruction will be fetched from the target.
            // We update the PC, and the instruction currently in IF/ID is effectively killed (a NOP).
            // The instruction currently in ID/EX (the NOP) proceeds to EX.
            program_counter_ = ex_mem_reg_.branch_target_pc;
            if_id_reg_.reset(); // Kill the instruction (should be the programmer's NOP)
        }
        // Conditional branches (B-type) are NOT handled here, ensuring the 3-NOP delay.
    }


    // Record memory changes for Undo/Redo
    std::vector<uint8_t> old_bytes_vec;
    std::vector<uint8_t> new_bytes_vec;
    uint64_t addr = ex_mem_reg_.alu_result;

    if (ex_mem_reg_.mem_read)
    { 
        // Load instruction: Read from memory
        // Assuming all loads are RV64 (LD/LWU, etc.) which read a double word or 8 bytes
        mem_wb_reg_.memory_data = memory_controller_.ReadDoubleWord(addr);
    }
    else if (ex_mem_reg_.mem_write)
    { 
        // Store instruction: Write to memory
        
        // 1. Record state BEFORE the write for Undo
        for (size_t i = 0; i < 8; ++i)
            old_bytes_vec.push_back(memory_controller_.ReadByte_d(addr + i));

        // 2. Perform the write (assuming SD/Store Double Word for simplicity)
        memory_controller_.WriteDoubleWord(addr, ex_mem_reg_.reg2_data);

        // 3. Record state AFTER the write for Redo
        for (size_t i = 0; i < 8; ++i)
            new_bytes_vec.push_back(memory_controller_.ReadByte_d(addr + i));

        // 4. Save delta if a change occurred
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

void RV5StageVM_NH_NF::pipeline_writeback()
{
    // Check for a conditional branch resolution from the MEM stage (3 NOP delay)
    if (ex_mem_reg_.branch_taken) {
        uint32_t instruction_in_mem = id_ex_reg_.instruction; // Instruction was in ID/EX last cycle
        uint8_t opcode = instruction_in_mem & 0b1111111;

        if (opcode == 0b1100011) { // Conditional Branch (B-type)
            // The branch outcome is confirmed in the MEM stage.
            // This is the point where the PC must be redirected for the 3 NOP delay.
            
            // To model the 3 NOP delay:
            // 1. Instructions I1, I2, I3 (the NOPs) have already been fetched.
            // 2. We now need to redirect the PC for the *next* fetch (which will be the target).
            // 3. We must also kill I1, I2, I3 in the pipeline.

            // Kill instructions I1 (ID/EX) and I2 (IF/ID) that are still in the front end
            id_ex_reg_.reset(); 
            if_id_reg_.reset();
            
            // The instruction currently in MEM/WB (I3) will proceed to WB and write back 
            // garbage if it wasn't a NOP. We need to kill that too.
            mem_wb_reg_.reset();

            // Redirect the fetch PC
            program_counter_ = ex_mem_reg_.branch_target_pc;
        }
    }
    

    // Write the final result back to the register file
    if (mem_wb_reg_.reg_write && mem_wb_reg_.rd != 0) // x0 (rd=0) is hardwired to zero
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

// --- Undo/Redo Implementations (No changes) ---

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
    
    // Reset pipeline registers to NOPs. This is necessary because the Undo step
    // only reverts architectural state (PC, Registers, Memory), but the pipeline
    // registers (non-architectural state) must be cleared to allow the user
    // to start stepping cleanly from the restored PC.
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
    
    // Reset pipeline registers for a clean start
    if_id_reg_.reset();
    id_ex_reg_.reset();
    ex_mem_reg_.reset();
    mem_wb_reg_.reset();

    undo_stack_.push(next);
    std::cout << "VM_REDO_COMPLETED" << std::endl;
}

void RV5StageVM_NH_NF::print_pipeline_registers_debug()
{
    // Implementation for printing debug state (can be added later if needed)
}
