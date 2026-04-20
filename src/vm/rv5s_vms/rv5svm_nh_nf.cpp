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
#include "debug_colors.h"

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

    // Reset components and history
    #ifndef DISABLE_GUI 
    circuit_scene_ = std::make_unique<Kites::RV5StageVM_NH_NF_CircuitScene>();
    connect(this, &VmBase::updateCircuitStateSignal,
            circuit_scene_.get(), &Kites::RV5StageVM_NH_NF_CircuitScene::updateCircuitState);
    #endif
    //connect(this, &VmBase::vmStateChangedSignal,
      //      circuit_scene_.get(), &Kites::RV5StageVM_NH_NF_CircuitScene::vmStateChangedSlot);
    Reset();
    active_wires_.append("PC_to_IM");
    active_wires_.append("PCMux_to_PC");
    active_wires_.append("IM_to_P1");
    active_wires_.append("P1_to_P2_PCcarry");
    active_wires_.append("P1_to_Control_RF_andall");
    active_wires_.append("RF_to_P2_UP");
    active_wires_.append("RFdown_to_P2");
    active_wires_.append("P2_to_ALUSRC");
    active_wires_.append("P2_to_ALUSCRMux");
    active_wires_.append("P2_to_ALUControl");
    active_wires_.append("ALUcontrol_to_ALU");
    active_wires_.append("ALUMux_to_ALU");


    active_wires_.append("P2_to_P3_MEMControl");
    active_wires_.append("P2_to_P3_WBcontrol");
    active_wires_.append("P2_to_ALUcontrol_Control");
    //active_wires_.append("P2_to_ALUMux");

    active_wires_.append("Control_to_P3_up");
    active_wires_.append("Control_to_P3_mid");
    active_wires_.append("Control_to_P3_down");
    
    active_wires_.append("P2_to_P3_WBcontrol");
    active_wires_.append("P2_to_P3_MEMControl");
    active_wires_.append("P2_to_ALUcontrol_Control");

    active_wires_.append("P2_to_ALU2");

    active_wires_.append("P3_to_P4_WBcontrol");

    active_wires_.append("rd_P2_to_P3");
    active_wires_.append("rd_P3_to_P4");

    always_active_wires_count_ = active_wires_.size();
}



void RV5StageVM_NH_NF::SetActiveWireNames() 
{
    // Clear and populate canonical wires for the NH_NF circuit (no forwarding)
    active_wires_.erase(active_wires_.begin() + always_active_wires_count_, active_wires_.end());

    // Backbone / always-visible wires (scene JSON uses these names)
    

    // Conditional wires based on current pipeline signals
    if (id_ex_reg_.alu_src)
    {
        active_wires_.append("Imm_to_P2");
        active_wires_.append("P2Imm_to_ALU2_down");
        active_wires_.append("P2_to_ALUMux");
    }

    
    // Memory stage activity (EX/MEM)
    // the intruction executed in this cycle was branch/jal/jalr
    if(ex_mem_reg_.instruction & 0b1111111 == 0b1100011
    ||ex_mem_reg_.instruction & 0b1111111 == 0b1100111
    ||ex_mem_reg_.instruction & 0b1111111 == 0b1101111)
    {
        active_wires_.append("ALU_zerores_to_P3");
        active_wires_.append("ALU2_to_P3");
    }
    else
    {
        active_wires_.append("ALU_to_P3");
    }

    if(mem_wb_reg_.instruction & 0b11111111 == 0b1100011
    ||mem_wb_reg_.instruction & 0b11111111 == 0b1100111
    ||mem_wb_reg_.instruction & 0b11111111 == 0b1101111)
    {
        active_wires_.append("ANDGate_lower_entry");
        active_wires_.append("P3_to_PCMux");
        //active_wires_.append("ANDGATE_to_PCMUX");
    }
    else
    {
        active_wires_.append("P3ALUres_to_DMup");
    }

    if(ex_mem_reg_.prev_branch_taken)
    {
        active_wires_.append("ANDGATE_to_PCMUX");
        active_wires_.append("P3_to_UpperEntryANDGATE");
    }
    else
    {
        active_wires_.append("ALU1_to_PCMuxUp");
    }

    if (ex_mem_reg_.prev_mem_read)
    {
        active_wires_.append("DM_to_P4");
        active_wires_.append("P3_to_DM_Memread");
    }
    if (ex_mem_reg_.prev_mem_write)
    {
        active_wires_.append("P3_TO_DM_control_memwrite");
        active_wires_.append("P3_rs2_to_DMdown");
        //active_wires_.append("P3ALUres_to_DMup");
    }

    if(mem_wb_reg_.prev_reg_write)
    {
      active_wires_.append("P4_wbcontrol_to_WBmux");
      active_wires_.append("P4_to_RF_regwritecontrol");
      active_wires_.append("WBMux_to_RF");
    }

    if(mem_wb_reg_.prev_mem_to_reg)
    {
        active_wires_.append("P4DM_to_lastMux");
        //active_wires_.append("WBMux_to_RF");
    }
    else if(mem_wb_reg_.prev_reg_write)
    {

        active_wires_.append("P4_ALUres_to_Mux");
        active_wires_.append("RdP4_to_RF");
    }
    // Branch / PC selection wires
    

    // The scene reads `active_wires_` when callers emit `updateCircuitStateSignal`.
}


void RV5StageVM_NH_NF::Reset()
{
    // Reset base class architectural state members
    program_counter_ = 0;
    instructions_retired_ = 0;
    cycle_s_ = 0;


    // Reset all hardware components
    registers_.Reset();
    memory_controller_.Reset();
    control_unit_.Reset();

    // Reset all pipeline registers to a known-safe (NOP) state
    if_id_reg_.reset();
    id_ex_reg_.reset();
    ex_mem_reg_.reset();
    mem_wb_reg_.reset();
    vm_state_.clear();
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

    pipeline_writeback();
    pipeline_memory(); 
    pipeline_execute(); 
    pipeline_decode();
    pipeline_fetch();

    uint64_t next_pc = program_counter_; 
    
    // If no redirect happened in EX or MEM, advance sequentially.
    if (next_pc == old_pc_before_redirect) {
        next_pc = old_pc_before_redirect + 4;
    }
    
    // Commit the new PC for the Fetch stage
    program_counter_ = next_pc;
    cycle_s_++; 

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
    
}


void RV5StageVM_NH_NF::pipeline_fetch()
{
    if (program_counter_ < program_size_)
    {
        // Latch the instruction and PC for the next stage (IF/ID register)
        if_id_reg_.instruction = memory_controller_.ReadWord_d(program_counter_);
        if_id_reg_.pc = program_counter_;
    }
    else
    {
        // Once past the end of the program, inject NOPs to drain the pipeline.
        if_id_reg_.reset();
    }
}

// void RV5StageVM_NH_NF::pipeline_decode()
// {
//     //Get instruction from the IF/ID register
//     uint32_t instruction = if_id_reg_.instruction;
//     if (instruction == NOP) {
//         // Pass through fields as needed
//         id_ex_reg_.pc = if_id_reg_.pc;
//         id_ex_reg_.instruction = instruction;
//         id_ex_reg_.imm = 0;
//         id_ex_reg_.rs1 = id_ex_reg_.rs2 = id_ex_reg_.rd = 0;
//         id_ex_reg_.reg1_data = 0;
//         id_ex_reg_.reg2_data = 0;

//         // Critically: zero *all* control signals so downstream stages are idle
//         id_ex_reg_.reg_write = false;
//         id_ex_reg_.branch    = false;
//         id_ex_reg_.alu_src   = false;
//         id_ex_reg_.mem_read  = false;
//         id_ex_reg_.mem_write = false;
//         id_ex_reg_.mem_to_reg= false;
//         id_ex_reg_.alu_op    = 0;
//         return;
//     }
//     // Control Unit: Generate signals based on the instruction
//     control_unit_.SetControlSignals(instruction);

//     // Latch data for the ID/EX register
//     id_ex_reg_.pc = if_id_reg_.pc;
//     id_ex_reg_.instruction = instruction;
//     id_ex_reg_.imm = ImmGenerator(instruction);

//     // Extract register numbers
//     id_ex_reg_.rs1 = (instruction >> 15) & 0x1F;
//     id_ex_reg_.rs2 = (instruction >> 20) & 0x1F;
//     id_ex_reg_.rd = (instruction >> 7) & 0x1F;

//     // Extract FP register indices for F/D instructions
//     if(instruction_set::isFInstruction(instruction) || instruction_set::isDInstruction(instruction))
//     {
//         id_ex_reg_.frs1 = (instruction >> 15) & 0x1F;
//         id_ex_reg_.frs2 = (instruction >> 20) & 0x1F;
//         id_ex_reg_.frs3 = (instruction >> 27) & 0x1F;
//         id_ex_reg_.frd  = (instruction >> 7) & 0x1F;

//         id_ex_reg_.freg1_data = registers_.ReadFpr(id_ex_reg_.frs1);
//         id_ex_reg_.freg2_data = registers_.ReadFpr(id_ex_reg_.frs2);
//         id_ex_reg_.freg3_data = registers_.ReadFpr(id_ex_reg_.frs3);
//         uint8_t opcode = instruction & 0b1111111;
//         uint8_t funct7 = (instruction >> 25) & 0b1111111;

//         // For load instructions, rs1 is a GPR base address.
//         // For integer-to-float conversions / moves, rs1 is also a GPR source.
//     }
//     else
//     {
//         id_ex_reg_.reg1_data = registers_.ReadGpr(id_ex_reg_.rs1);
//         id_ex_reg_.reg2_data = registers_.ReadGpr(id_ex_reg_.rs2);
//     }

//     // Pass all control signals to the next stage
//     id_ex_reg_.reg_write  = control_unit_.GetRegWrite();
//     id_ex_reg_.freg_write = instruction_set::isFInstruction(instruction) || instruction_set::isDInstruction(instruction);
//     id_ex_reg_.branch     = control_unit_.GetBranch();
//     id_ex_reg_.alu_src    = control_unit_.GetAluSrc();
//     id_ex_reg_.mem_read   = control_unit_.GetMemRead();
//     id_ex_reg_.mem_write  = control_unit_.GetMemWrite();
//     id_ex_reg_.mem_to_reg = control_unit_.GetMemToReg();
//     id_ex_reg_.alu_op     = control_unit_.GetAluOp();
// }

void RV5StageVM_NH_NF::pipeline_execute()
{
    uint32_t instruction = id_ex_reg_.instruction;
    uint8_t opcode = instruction & 0b1111111;
    uint8_t funct3 = (instruction >> 12) & 0x7;
    
    if (opcode == 0b1110011 && funct3 == 0b000)
	{
        handle_syscall();
		return;
	}

    if (opcode == 0b1110011)
	{
		execute_csr();
		return;
	}

    // Select ALU inputs
    uint64_t alu_in1 = id_ex_reg_.reg1_data;
    uint64_t alu_in2 = id_ex_reg_.alu_src ? static_cast<uint64_t>(id_ex_reg_.imm) : id_ex_reg_.reg2_data;

    // Get the specific ALU operation
    alu::AluOp alu_operation = control_unit_.GetAluSignal(id_ex_reg_.instruction, id_ex_reg_.alu_op > 0);

    // Execute the operation
    bool overflow; // Ignored for this simple model
    uint64_t alu_result = 0;
    const bool is_f_instruction = instruction_set::isFInstruction(instruction);
    const bool is_d_instruction = instruction_set::isDInstruction(instruction);
    if (is_f_instruction)
    {
        alu_result = execute_float();
    }
    else if (is_d_instruction)
    {
        alu_result = execute_double();
    }
    else
    {
        std::tie(alu_result, overflow) = alu::Alu::execute(alu_operation, alu_in1, alu_in2);
    }
    if((id_ex_reg_.instruction & 0b1111111) == 0b0110111) //lui
    {
        alu_result = static_cast<uint64_t>(id_ex_reg_.imm << 12);
    }

    // Latch data for EX/MEM Register
    ex_mem_reg_.pc                = id_ex_reg_.pc;
    ex_mem_reg_.instruction       = id_ex_reg_.instruction;
    ex_mem_reg_.alu_result        = alu_result;
    ex_mem_reg_.f_alu_result      = (is_f_instruction || is_d_instruction) ? alu_result : 0;
    ex_mem_reg_.rd                = id_ex_reg_.rd;
    ex_mem_reg_.frd               = id_ex_reg_.frd;
    ex_mem_reg_.reg2_data         = id_ex_reg_.reg2_data;
    ex_mem_reg_.freg2_data        = id_ex_reg_.freg2_data;
    ex_mem_reg_.reg_write         = id_ex_reg_.reg_write;
    ex_mem_reg_.mem_to_reg        = id_ex_reg_.mem_to_reg;
    ex_mem_reg_.prev_mem_read     = ex_mem_reg_.mem_read;
    ex_mem_reg_.prev_mem_write    = ex_mem_reg_.mem_write;
    ex_mem_reg_.mem_read          = id_ex_reg_.mem_read;
    ex_mem_reg_.mem_write         = id_ex_reg_.mem_write;
    ex_mem_reg_.prev_branch_taken = ex_mem_reg_.branch_taken;
    ex_mem_reg_.branch_taken      = false;
    ex_mem_reg_.branch_target_pc  = 0;
    
    

    // --- Conditional Branch Check (B-type: BLT, BGE, etc.) ---
    if (id_ex_reg_.branch && opcode == 0b1100011) 
    {
        bool condition_met = false;
        
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

// --- Undo/Redo Implementations ---



uint64_t RV5StageVM_NH_NF::execute_float() 
{
    uint32_t instruction = id_ex_reg_.instruction;
    uint8_t opcode = instruction & 0b1111111;
    uint8_t funct3 = (instruction >> 12) & 0b111;
    uint8_t funct7 = (instruction >> 25) & 0b1111111;
	uint8_t rm = funct3;

	uint8_t fcsr_status = 0;
    uint64_t alu_result = 0;

	if (rm == 0b111)
	{
		rm = registers_.ReadCsr(0x002);
	}

    uint64_t reg1_value = id_ex_reg_.freg1_data;
    uint64_t reg2_value = id_ex_reg_.freg2_data;
    uint64_t reg3_value = id_ex_reg_.freg3_data;

	if (funct7 == 0b1101000 || funct7 == 0b1111000 || opcode == 0b0000111 || opcode == 0b0100111)
	{
        reg1_value = registers_.ReadGpr(id_ex_reg_.rs1);
	}

    if (id_ex_reg_.alu_src)
	{
        std::cout << GREEN << "Is the alu src set correctly?" << RESET << std::endl;
        reg2_value = static_cast<uint64_t>(static_cast<int64_t>(id_ex_reg_.imm));
        std::cout << BLUE << "Immediate value used in ALU: " << reg2_value << RESET << std::endl;
	}

    alu::AluOp aluOperation = control_unit_.GetAluSignal(instruction, id_ex_reg_.alu_op > 0);
    std::cout << "The registers values:\n";
    std::cout << reg1_value << ' ' << reg2_value << ' ' << reg3_value << std::endl;
	std::tie(alu_result, fcsr_status) = alu::Alu::fpexecute(aluOperation, reg1_value, reg2_value, reg3_value, rm);

    std::cout << "The floating point result : \n";
    std::cout << alu_result << std::endl;

	registers_.WriteCsr(0x003, fcsr_status);
    return alu_result;
}

uint64_t RV5StageVM_NH_NF::execute_double()
{
    uint32_t instruction = id_ex_reg_.instruction;
    uint8_t opcode = instruction & 0b1111111;
    uint8_t funct3 = (instruction >> 12) & 0b111;
    uint8_t funct7 = (instruction >> 25) & 0b1111111;
    uint8_t rm = funct3;

    uint8_t fcsr_status = 0;
    uint64_t alu_result = 0;

    if (rm == 0b111)
    {
        rm = registers_.ReadCsr(0x002);
    }

    uint64_t reg1_value = id_ex_reg_.freg1_data;
    uint64_t reg2_value = id_ex_reg_.freg2_data;
    uint64_t reg3_value = id_ex_reg_.freg3_data;

    if (funct7 == 0b1101001 || funct7 == 0b1111001 || opcode == 0b0000111 || opcode == 0b0100111)
    {
        reg1_value = registers_.ReadGpr(id_ex_reg_.rs1);
    }

    if (id_ex_reg_.alu_src)
    {
        reg2_value = static_cast<uint64_t>(static_cast<int64_t>(id_ex_reg_.imm));
    }

    alu::AluOp alu_operation = control_unit_.GetAluSignal(instruction, id_ex_reg_.alu_op > 0);
    std::tie(alu_result, fcsr_status) = alu::Alu::dfpexecute(alu_operation, reg1_value, reg2_value, reg3_value, rm);
    
    registers_.WriteCsr(0x003, fcsr_status);
    return alu_result;
}


void RV5StageVM_NH_NF::handle_syscall() { 
    if ((id_ex_reg_.instruction & 0x7F) == 0b1110011 && ((id_ex_reg_.instruction >> 12) & 0x7) == 0b000) {
        RequestStop();
        output_status_ = "ECALL_EXIT";
        DumpState("vm_state.json");
    }
}

// --- FP Execute Handlers ---

void RV5StageVM_NH_NF::pipeline_execute_float()
{
    (void)execute_float();
}

void RV5StageVM_NH_NF::pipeline_execute_double()
{
    (void)execute_double();
}

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


// void RV5StageVM_NH_NF::memory_float()
// {
//     uint8_t rs2 = (current_instruction_ >> 20) & 0b11111;

// 	if (control_unit_.GetMemRead())
// 	{ // FLW
// 		mem_wb_reg_.memory_data = memory_controller_.ReadWord(mem_wb_reg_.alu_result);
// 	}

// 	uint64_t addr = 0;
// 	std::vector<uint8_t> old_bytes_vec;
// 	std::vector<uint8_t> new_bytes_vec;

// 	if (control_unit_.GetMemWrite())
// 	{ // FSW
// 		addr = mem_wb_reg_.alu_result;
// 		for (size_t i = 0; i < 4; ++i)
// 		{
// 			old_bytes_vec.push_back(memory_controller_.ReadByte(addr + i));
// 		}
// 		uint32_t val = registers_.ReadFpr(rs2) & 0xFFFFFFFF;
// 		memory_controller_.WriteWord(mem_wb_reg_.alu_result, val);
// 		// new_bytes_vec.push_back(memory_controller_.ReadByte(addr));
// 		for (size_t i = 0; i < 4; ++i)
// 		{
// 			new_bytes_vec.push_back(memory_controller_.ReadByte(addr + i));
// 		}
// 	}

// 	if (old_bytes_vec != new_bytes_vec)
// 	{
// 		current_delta_.memory_changes.push_back({addr, old_bytes_vec, new_bytes_vec});
// 	}
// }

// void RV5StageVM_NH_NF::writeback_float()
// {
//     uint8_t opcode = current_instruction_ & 0b1111111;
// 	uint8_t funct7 = (current_instruction_ >> 25) & 0b1111111;
// 	uint8_t rd = (current_instruction_ >> 7) & 0b11111;

// 	uint64_t old_reg = 0;
// 	unsigned int reg_index = rd;
// 	unsigned int reg_type = 2; // 0 for GPR, 1 for CSR, 2 for FPR
// 	uint64_t new_reg = 0;

// 	if (control_unit_.GetRegWrite())
// 	{
// 		// write to GPR
// 		if (funct7 == 0b1010000 || funct7 == 0b1100000 || funct7 == 0b1110000)
// 		{ // f(eq|lt|le).s, fcvt.(w|wu|l|lu).s
// 			old_reg = registers_.ReadGpr(rd);
// 			registers_.WriteGpr(rd, mem_wb_reg_.alu_result);
// 			new_reg = mem_wb_reg_.alu_result;
// 			reg_type = 0; // GPR
// 		}
// 		// write to FPR
// 		else if (opcode == 0b0000111)
// 		{
// 			old_reg = registers_.ReadFpr(rd);
// 			registers_.WriteFpr(rd, mem_wb_reg_.memory_data);
// 			new_reg = mem_wb_reg_.memory_data;
// 			reg_type = 2; // FPR
// 		}
// 		else
// 		{
// 			old_reg = registers_.ReadFpr(rd);
// 			registers_.WriteFpr(rd, mem_wb_reg_.alu_result);
// 			new_reg = mem_wb_reg_.alu_result;
// 			reg_type = 2; // FPR
// 		}
// 	}

// 	if (old_reg != new_reg)
// 	{
// 		current_delta_.register_changes.push_back({reg_index, reg_type, old_reg, new_reg});
// 	}
// }