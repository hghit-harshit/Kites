#include "common/debug_colors.h"
#include "common/instructions.h"
#include "processor/rv5s/rv5s_processor_base.h"
#include <thread>

namespace Kites
{
void RV5StageVM_Base::Run()
{
    ClearStop();
    while (!stop_requested_ && (program_counter_ < program_size_ || !is_pipeline_drained()))
    {
        if (last_breakpoint_pc_ && program_counter_ != *last_breakpoint_pc_)
        {
            last_breakpoint_pc_.reset();
        }

        // if we hit a break point (and it's a new PC), we pause execution
        // This prevents firing the same breakpoint multiple times during stalls
        if (std::find(breakpoints_.begin(), breakpoints_.end(), program_counter_) !=
                breakpoints_.end() && program_counter_ != last_breakpoint_pc_)
        {
            pause_requested_ = true;
            last_breakpoint_pc_ = program_counter_;
            emit processorPausedAtBreakpointSignal();
        }

        {
            QMutexLocker locker(&pause_mutex_);
            // using while because thie wait can be interrupted by spurious wakeups
            while (pause_requested_ && !stop_requested_)
            {
                pause_wait_condition_.wait(&pause_mutex_);
            }

            if (stop_requested_) // if we were requested to stop while paused
                break;
        }
        Step();
        SetActiveWireNames();
        cpi_ = instructions_retired_
                   ? static_cast<float>(cycle_s_) / static_cast<float>(instructions_retired_)
                   : 0.0f;
        ipc_ = cycle_s_ ? static_cast<float>(instructions_retired_) / static_cast<float>(cycle_s_)
                        : 0.0f;
        
        setProcessorState();
        emit updateCircuitStateSignal(active_wires_);
        emit processorClockedSignal(processor_state_);

        // handling the delay
        {
            QMutexLocker locker(&pause_mutex_);
            if (stop_requested_ || pause_requested_)
                continue;
            pause_wait_condition_.wait(&pause_mutex_, step_delay_);
            // we wait for step_delay_ milliseconds or until notified to wake up
        }
    }
    if (stop_requested_)
    {
        emit processorClockedSignal(processor_state_);
        // we emit the vm state changed signal once more to update the ui
        // this will get rid of any highlights as they pause when we stop
        // before execution is complete
    }
}

void RV5StageVM_Base::DebugRun()
{
    ClearStop();
    while (!stop_requested_ && (program_counter_ < program_size_ || !is_pipeline_drained()))
    {
#ifdef VM_DEBUG_PRINTS
        print_pipeline_registers_debug();
#endif
        Step();
        std::cout << "Cycle: " << cycle_s_ << " | PC: 0x" << std::hex << program_counter_
                  << std::dec << std::endl;
    }
#ifdef VM_DEBUG_PRINTS
    print_pipeline_registers_debug();
#endif
}

void RV5StageVM_Base::Reset()
{
    program_counter_      = 0;
    instructions_retired_ = 0;
    cycle_s_              = 0;
    stall_cycles_         = 0;
    last_breakpoint_pc_.reset(); // Clear breakpoint tracking on reset

    registers_.Reset();
    memory_controller_.reset();
    control_unit_.Reset();

    if_id_reg_.reset();
    id_ex_reg_.reset();
    ex_mem_reg_.reset();
    mem_wb_reg_.reset();
    current_delta_ = RV5StageStepDelta{};
    undo_stack_ = std::stack<RV5StageStepDelta>();
    redo_stack_ = std::stack<RV5StageStepDelta>();
    processorState.reset();
}

void RV5StageVM_Base::begin_step_delta()
{
    current_delta_                           = RV5StageStepDelta{};
    current_delta_.old_pc                    = program_counter_;
    current_delta_.old_cycle                 = cycle_s_;
    current_delta_.old_stall_cycles          = stall_cycles_;
    current_delta_.old_instructions_retired  = instructions_retired_;
    current_delta_.old_branch_mispredictions = branch_mispredictions_;

    current_delta_.pipeline_register_change.old_if_id_reg  = if_id_reg_;
    current_delta_.pipeline_register_change.old_id_ex_reg  = id_ex_reg_;
    current_delta_.pipeline_register_change.old_ex_mem_reg = ex_mem_reg_;
    current_delta_.pipeline_register_change.old_mem_wb_reg = mem_wb_reg_;
}

void RV5StageVM_Base::finalize_step_delta()
{
    current_delta_.new_pc                    = program_counter_;
    current_delta_.new_cycle                 = cycle_s_;
    current_delta_.new_stall_cycles          = stall_cycles_;
    current_delta_.new_instructions_retired  = instructions_retired_;
    current_delta_.new_branch_mispredictions = branch_mispredictions_;

    current_delta_.pipeline_register_change.new_if_id_reg  = if_id_reg_;
    current_delta_.pipeline_register_change.new_id_ex_reg  = id_ex_reg_;
    current_delta_.pipeline_register_change.new_ex_mem_reg = ex_mem_reg_;
    current_delta_.pipeline_register_change.new_mem_wb_reg = mem_wb_reg_;

    undo_stack_.push(current_delta_);
    while (!redo_stack_.empty())
    {
        redo_stack_.pop();
    }
}

void RV5StageVM_Base::setProcessorState()
{
    processorState.reset();
}

void RV5StageVM_Base::pipeline_decode()
{
    uint32_t instruction = if_id_reg_.instruction;
    if (instruction == NOP)
    {
        // Pass through fields as needed
        id_ex_reg_.pc          = if_id_reg_.pc;
        id_ex_reg_.instruction = instruction;
        id_ex_reg_.imm         = 0;
        id_ex_reg_.rs1         = id_ex_reg_.rs2 = id_ex_reg_.rd = 0;
        id_ex_reg_.reg1_data   = 0;
        id_ex_reg_.reg2_data   = 0;

        // Clear FPR fields so stale values don't poison forwarding
        id_ex_reg_.frs1 = id_ex_reg_.frs2 = id_ex_reg_.frs3 = id_ex_reg_.frd = 0;
        id_ex_reg_.freg1_data = 0;
        id_ex_reg_.freg2_data = 0;
        id_ex_reg_.freg3_data = 0;

        // Critically: zero *all* control signals so downstream stages are idle
        id_ex_reg_.reg_write  = false;
        id_ex_reg_.freg_write = false;
        id_ex_reg_.branch     = false;
        id_ex_reg_.alu_src    = false;
        id_ex_reg_.mem_read   = false;
        id_ex_reg_.mem_write  = false;
        id_ex_reg_.mem_to_reg = false;
        id_ex_reg_.alu_op     = 0;
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

    // Extract FP register indices for F/D instructions
    if (instruction_set::isFInstruction(instruction) ||
        instruction_set::isDInstruction(instruction))
    {
        id_ex_reg_.frs1 = (instruction >> 15) & 0x1F;
        id_ex_reg_.frs2 = (instruction >> 20) & 0x1F;
        id_ex_reg_.frs3 = (instruction >> 27) & 0x1F;
        id_ex_reg_.frd = (instruction >> 7) & 0x1F;

        id_ex_reg_.freg1_data = registers_.ReadFpr(id_ex_reg_.frs1);
        id_ex_reg_.freg2_data = registers_.ReadFpr(id_ex_reg_.frs2);
        id_ex_reg_.freg3_data = registers_.ReadFpr(id_ex_reg_.frs3);


        // For load instructions, rs1 is a GPR base address.
        // For integer-to-float conversions / moves, rs1 is also a GPR source.
    }
    else
    {
        id_ex_reg_.reg1_data = registers_.ReadGpr(id_ex_reg_.rs1);
        id_ex_reg_.reg2_data = registers_.ReadGpr(id_ex_reg_.rs2);
    }

    // Pass all control signals to the next stage
    id_ex_reg_.reg_write  = control_unit_.GetRegWrite();
    id_ex_reg_.freg_write = instruction_set::isFInstruction(instruction) ||
                            instruction_set::isDInstruction(instruction);
    id_ex_reg_.branch     = control_unit_.GetBranch();
    id_ex_reg_.alu_src    = control_unit_.GetAluSrc();
    id_ex_reg_.mem_read   = control_unit_.GetMemRead();
    id_ex_reg_.mem_write  = control_unit_.GetMemWrite();
    id_ex_reg_.mem_to_reg = control_unit_.GetMemToReg();
    id_ex_reg_.alu_op     = control_unit_.GetAluOp();
}

void RV5StageVM_Base::memory_writeback()
{
    auto read_bytes = [&](uint64_t addr, size_t byte_count)
    {
        std::vector<uint8_t> bytes;
        bytes.reserve(byte_count);
        for (size_t i = 0; i < byte_count; ++i)
        {
            bytes.push_back(memory_controller_.readByte(addr + i));
        }
        return bytes;
    };

    switch ((mem_wb_reg_.instruction >> 12) & 0b111)
    {
    case 0b000:
    { // SB
        auto old_bytes_vec = read_bytes(ex_mem_reg_.alu_result, 1);
        memory_controller_.writeByte(ex_mem_reg_.alu_result, ex_mem_reg_.reg2_data & 0xFF);
        auto new_bytes_vec = read_bytes(ex_mem_reg_.alu_result, 1);
        if (old_bytes_vec != new_bytes_vec)
        {
            current_delta_.memory_changes.push_back(
                {ex_mem_reg_.alu_result, old_bytes_vec, new_bytes_vec});
        }
        break;
    }
    case 0b001:
    { // SH
        auto old_bytes_vec = read_bytes(ex_mem_reg_.alu_result, 2);
        memory_controller_.writeHalfWord(ex_mem_reg_.alu_result, ex_mem_reg_.reg2_data & 0xFFFF);
        auto new_bytes_vec = read_bytes(ex_mem_reg_.alu_result, 2);
        if (old_bytes_vec != new_bytes_vec)
        {
            current_delta_.memory_changes.push_back(
                {ex_mem_reg_.alu_result, old_bytes_vec, new_bytes_vec});
        }
        break;
    }
    case 0b010:
    { // SW
        auto old_bytes_vec = read_bytes(ex_mem_reg_.alu_result, 4);
        memory_controller_.writeWord(ex_mem_reg_.alu_result, ex_mem_reg_.reg2_data & 0xFFFFFFFF);
        auto new_bytes_vec = read_bytes(ex_mem_reg_.alu_result, 4);
        if (old_bytes_vec != new_bytes_vec)
        {
            current_delta_.memory_changes.push_back(
                {ex_mem_reg_.alu_result, old_bytes_vec, new_bytes_vec});
        }
        break;
    }
    case 0b011:
    { // SD
        auto old_bytes_vec = read_bytes(ex_mem_reg_.alu_result, 8);
        memory_controller_.writeDoubleWord(ex_mem_reg_.alu_result,
                                           ex_mem_reg_.reg2_data & 0xFFFFFFFFFFFFFFFF);
        auto new_bytes_vec = read_bytes(ex_mem_reg_.alu_result, 8);
        if (old_bytes_vec != new_bytes_vec)
        {
            current_delta_.memory_changes.push_back(
                {ex_mem_reg_.alu_result, old_bytes_vec, new_bytes_vec});
        }
        break;
    }
    }
}

/** Ignore the next two fucntion they i Ill refactor thosee i promise :'( */

void RV5StageVM_Base::memory_read()
{
    switch ((mem_wb_reg_.instruction >> 12) & 0b111)
    {
    case 0b000:
    { // LB
        mem_wb_reg_.memory_data =
            static_cast<int8_t>(memory_controller_.readByte(ex_mem_reg_.alu_result));
        break;
    }
    case 0b001:
    { // LH
        mem_wb_reg_.memory_data =
            static_cast<int16_t>(memory_controller_.readHalfWord(ex_mem_reg_.alu_result));
        break;
    }
    case 0b010:
    { // LW
        mem_wb_reg_.memory_data =
            static_cast<int32_t>(memory_controller_.readWord(ex_mem_reg_.alu_result));
        break;
    }
    case 0b011:
    { // LD
        mem_wb_reg_.memory_data = memory_controller_.readDoubleWord(ex_mem_reg_.alu_result);
        break;
    }
    case 0b100:
    { // LBU
        mem_wb_reg_.memory_data =
            static_cast<uint8_t>(memory_controller_.readByte(ex_mem_reg_.alu_result));
        break;
    }
    case 0b101:
    { // LHU
        mem_wb_reg_.memory_data =
            static_cast<uint16_t>(memory_controller_.readHalfWord(ex_mem_reg_.alu_result));
        break;
    }
    case 0b110:
    { // LWU
        mem_wb_reg_.memory_data =
            static_cast<uint32_t>(memory_controller_.readWord(ex_mem_reg_.alu_result));
        break;
    }
    }
}

void RV5StageVM_Base::register_write_back(const uint64_t &write_data)
{
    switch (mem_wb_reg_.instruction & 0b1111111)
    {
    case 0b0110011: // R-Type
    case 0b0010011: // I-Type
    case 0b0010111:
    { // AUIPC
        registers_.WriteGpr(mem_wb_reg_.rd, write_data);
        break;
    }
    case 0b0000011:
    { // Load
        registers_.WriteGpr(mem_wb_reg_.rd, write_data);
        break;
    }
    case 0b1100111: // JALR
    case 0b1101111:
    { // JAL
        registers_.WriteGpr(mem_wb_reg_.rd, mem_wb_reg_.pc + 4);
        break;
    }
    case 0b0110111:
    { // LUI
        registers_.WriteGpr(mem_wb_reg_.rd, write_data << 12);
        break;
    }
    default:
        break;
    }
}

//////////////////////

void RV5StageVM_Base::pipeline_memory()
{

    // --- B-Type Conditional Branch Resolution (3-Cycle Penalty) ---
    if (ex_mem_reg_.branch_taken && (ex_mem_reg_.instruction & 0b1111111) == 0b1100011)
    {
        // B-Type misprediction confirmed in MEM stage. Hardware flushes the pipeline.

        program_counter_ = ex_mem_reg_.branch_target_pc;
        if_id_reg_.reset();
        id_ex_reg_.reset();
        branch_mispredictions_++;
    }

    // since we are running cycle backward
    // by the time execture check this register forwarding previous results are gone
    // so we store these seperately
    mem_wb_reg_.prev_rd          = mem_wb_reg_.rd;
    mem_wb_reg_.prev_alu_result  = mem_wb_reg_.alu_result;
    mem_wb_reg_.prev_mem_to_reg  = mem_wb_reg_.mem_to_reg;
    mem_wb_reg_.prev_reg_write   = mem_wb_reg_.reg_write;
    mem_wb_reg_.prev_memory_data = mem_wb_reg_.memory_data;
    // Save FPR prev fields for forwarding (mirrors GPR prev fields above)
    mem_wb_reg_.prev_frd           = mem_wb_reg_.frd;
    mem_wb_reg_.prev_f_alu_result  = mem_wb_reg_.f_alu_result;
    mem_wb_reg_.prev_f_memory_data = mem_wb_reg_.f_memory_data;
    mem_wb_reg_.prev_freg_write    = mem_wb_reg_.freg_write;
    // --- Standard MEM Operations ---
    mem_wb_reg_.pc          = ex_mem_reg_.pc;
    mem_wb_reg_.instruction = ex_mem_reg_.instruction;
    mem_wb_reg_.alu_result  = ex_mem_reg_.alu_result;
    mem_wb_reg_.rd          = ex_mem_reg_.rd;
    mem_wb_reg_.reg_write   = ex_mem_reg_.reg_write;
    mem_wb_reg_.mem_to_reg  = ex_mem_reg_.mem_to_reg;
    // Latch FPR fields from EX/MEM
    mem_wb_reg_.frd          = ex_mem_reg_.frd;
    mem_wb_reg_.f_alu_result = ex_mem_reg_.f_alu_result;
    mem_wb_reg_.freg_write   = ex_mem_reg_.freg_write;

    bool is_F_Instruction = instruction_set::isFInstruction(mem_wb_reg_.instruction);
    bool is_D_Instruction = instruction_set::isDInstruction(mem_wb_reg_.instruction);

    // Memory Access
    if (ex_mem_reg_.mem_read)
    {
        // Load instruction: Result available at end of this stage (Load-Use still needs 1 NOP
        // stall)
        if (is_F_Instruction)
        { // FLW

            mem_wb_reg_.memory_data =
                static_cast<int32_t>(memory_controller_.readWord(ex_mem_reg_.alu_result));
            mem_wb_reg_.f_memory_data = mem_wb_reg_.memory_data;
        }
        else if (is_D_Instruction)
        { // FLD
            mem_wb_reg_.memory_data = memory_controller_.readDoubleWord(ex_mem_reg_.alu_result);
            mem_wb_reg_.f_memory_data = mem_wb_reg_.memory_data;
        }
        else
        {
            switch ((mem_wb_reg_.instruction >> 12) & 0b111)
            {
            case 0b000:
            { // LB
                mem_wb_reg_.memory_data =
                    static_cast<int8_t>(memory_controller_.readByte(ex_mem_reg_.alu_result));
                break;
            }
            case 0b001:
            { // LH
                mem_wb_reg_.memory_data =
                    static_cast<int16_t>(memory_controller_.readHalfWord(ex_mem_reg_.alu_result));
                break;
            }
            case 0b010:
            { // LW
                mem_wb_reg_.memory_data =
                    static_cast<int32_t>(memory_controller_.readWord(ex_mem_reg_.alu_result));
                break;
            }
            case 0b011:
            { // LD
                mem_wb_reg_.memory_data = memory_controller_.readDoubleWord(ex_mem_reg_.alu_result);
                break;
            }
            case 0b100:
            { // LBU
                mem_wb_reg_.memory_data =
                    static_cast<uint8_t>(memory_controller_.readByte(ex_mem_reg_.alu_result));
                break;
            }
            case 0b101:
            { // LHU
                mem_wb_reg_.memory_data =
                    static_cast<uint16_t>(memory_controller_.readHalfWord(ex_mem_reg_.alu_result));
                break;
            }
            case 0b110:
            { // LWU
                mem_wb_reg_.memory_data =
                    static_cast<uint32_t>(memory_controller_.readWord(ex_mem_reg_.alu_result));
                break;
            }
            }
        }
        std::cout << "Data read:" << (int)mem_wb_reg_.memory_data << std::endl;
    }
    else if (ex_mem_reg_.mem_write)
    {
        auto read_bytes = [&](uint64_t addr, size_t byte_count)
        {
            std::vector<uint8_t> bytes;
            bytes.reserve(byte_count);
            for (size_t i = 0; i < byte_count; ++i)
            {
                bytes.push_back(memory_controller_.readByte(addr + i));
            }
            return bytes;
        };

        std::cout << (int)ex_mem_reg_.alu_result << ' ' << (int)ex_mem_reg_.reg2_data << std::endl;
        if (is_F_Instruction)
        { // FSW
            auto old_bytes_vec = read_bytes(ex_mem_reg_.alu_result, 4);
            std::cout << debug_color::red << "For float store we are writing:" << (int)ex_mem_reg_.freg2_data
                      << std::endl
                      << "To address:" << (int)ex_mem_reg_.alu_result << std::endl
                      << debug_color::reset;
            memory_controller_.writeWord(ex_mem_reg_.alu_result,
                                         ex_mem_reg_.freg2_data & 0xFFFFFFFF);
            auto new_bytes_vec = read_bytes(ex_mem_reg_.alu_result, 4);
            if (old_bytes_vec != new_bytes_vec)
            {
                current_delta_.memory_changes.push_back(
                    {ex_mem_reg_.alu_result, old_bytes_vec, new_bytes_vec});
            }
        }
        else if (is_D_Instruction)
        { // FSD
            auto old_bytes_vec = read_bytes(ex_mem_reg_.alu_result, 8);
            memory_controller_.writeDoubleWord(ex_mem_reg_.alu_result,
                                               ex_mem_reg_.freg2_data & 0xFFFFFFFFFFFFFFFF);
            auto new_bytes_vec = read_bytes(ex_mem_reg_.alu_result, 8);
            if (old_bytes_vec != new_bytes_vec)
            {
                current_delta_.memory_changes.push_back(
                    {ex_mem_reg_.alu_result, old_bytes_vec, new_bytes_vec});
            }
        }
        else
        {
            memory_writeback();
        }
    }
}

void RV5StageVM_Base::pipeline_writeback()
{
    // --- FP Writeback Delegation ---

    if (mem_wb_reg_.reg_write && mem_wb_reg_.rd != 0)
    {
        // debug stuff
        std::cout << "Writing back to register x" << (int)mem_wb_reg_.rd << std::endl;
        std::cout << "Value:"
                  << (int)(mem_wb_reg_.mem_to_reg ? mem_wb_reg_.memory_data
                                                  : mem_wb_reg_.alu_result)
                  << std::endl;
        // debgug stuff end

        uint64_t write_data =
            mem_wb_reg_.mem_to_reg ? mem_wb_reg_.memory_data : mem_wb_reg_.alu_result;

        // Record state for Undo/Redo
        uint64_t old_value = registers_.ReadGpr(mem_wb_reg_.rd);
        if (old_value != write_data)
        {
            current_delta_.register_changes.push_back({mem_wb_reg_.rd,
                                                       0, // GPR type
                                                       old_value, write_data});
        }

        /**TODO i think this is counting wrong so look into this */
        instructions_retired_++; // Instruction successfully retired

        if (instruction_set::isFInstruction(mem_wb_reg_.instruction))
        {
            pipeline_writeback_float();
            return;
        }
        else if (instruction_set::isDInstruction(mem_wb_reg_.instruction))
        {
            pipeline_writeback_double();
            return;
        }

        switch (mem_wb_reg_.instruction & 0b1111111)
        {
        case 0b0110011: // R-Type
        case 0b0010011: // I-Type
        case 0b0010111:
        { // AUIPC
            registers_.WriteGpr(mem_wb_reg_.rd, write_data);
            break;
        }
        case 0b0000011:
        { // Load
            registers_.WriteGpr(mem_wb_reg_.rd, write_data);
            break;
        }
        case 0b1100111: // JALR
        case 0b1101111:
        { // JAL
            registers_.WriteGpr(mem_wb_reg_.rd, mem_wb_reg_.pc + 4);
            break;
        }
        case 0b0110111:
        { // LUI
            registers_.WriteGpr(mem_wb_reg_.rd, write_data);
            break;
        }
        default:
            break;
        }
    }
}

void RV5StageVM_Base::pipeline_writeback_float()
{
    // yes i just copy pasted this from rvss
    uint64_t current_instruction_ = mem_wb_reg_.instruction;
    uint8_t opcode = current_instruction_ & 0b1111111;
    uint8_t funct7 = (current_instruction_ >> 25) & 0b1111111;
    uint8_t rd = (current_instruction_ >> 7) & 0b11111;

    uint64_t old_reg = 0;
    unsigned int reg_index = rd;
    unsigned int reg_type = 2; // 0 for GPR, 1 for CSR, 2 for FPR
    uint64_t new_reg = 0;

    // write to GPR
    if (funct7 == 0b1010000 || funct7 == 0b1100000 || funct7 == 0b1110000)
    { // f(eq|lt|le).s, fcvt.(w|wu|l|lu).s
        old_reg = registers_.ReadGpr(rd);
        registers_.WriteGpr(rd, mem_wb_reg_.alu_result);
        new_reg = mem_wb_reg_.alu_result;
        reg_type = 0; // GPR
    }
    // write to FPR
    else if (opcode == 0b0000111)
    {
        old_reg = registers_.ReadFpr(rd);
        registers_.WriteFpr(rd, mem_wb_reg_.memory_data);
        new_reg = mem_wb_reg_.memory_data;
        reg_type = 2; // FPR
    }
    else
    {
        std::cout << "Writing to fpr : " << rd << " value:" << mem_wb_reg_.alu_result;
        old_reg = registers_.ReadFpr(rd);
        registers_.WriteFpr(rd, mem_wb_reg_.alu_result);
        new_reg = mem_wb_reg_.alu_result;
        reg_type = 2; // FPR
    }

    if (old_reg != new_reg)
    {
        current_delta_.register_changes.push_back({reg_index, reg_type, old_reg, new_reg});
    }
}

void RV5StageVM_Base::pipeline_writeback_double()
{
    uint64_t current_instruction_ = mem_wb_reg_.instruction;
    uint8_t opcode = current_instruction_ & 0b1111111;
    uint8_t funct7 = (current_instruction_ >> 25) & 0b1111111;
    uint8_t rd = (current_instruction_ >> 7) & 0b11111;

    uint64_t old_reg = 0;
    unsigned int reg_index = rd;
    unsigned int reg_type = 2; // 0 for GPR, 1 for CSR, 2 for FPR
    uint64_t new_reg = 0;

    // write to GPR
    if (funct7 == 0b1010001 || funct7 == 0b1100001 || funct7 == 0b1110001)
    { // f(eq|lt|le).d, fcvt.(w|wu|l|lu).d
        old_reg = registers_.ReadGpr(rd);
        registers_.WriteGpr(rd, mem_wb_reg_.alu_result);
        new_reg = mem_wb_reg_.alu_result;
        reg_type = 0; // GPR
    }
    // write to FPR
    else if (opcode == 0b0000111)
    {
        old_reg = registers_.ReadFpr(rd);
        registers_.WriteFpr(rd, mem_wb_reg_.memory_data);
        new_reg = mem_wb_reg_.memory_data;
        reg_type = 2; // FPR
    }
    else
    {
        old_reg = registers_.ReadFpr(rd);
        registers_.WriteFpr(rd, mem_wb_reg_.alu_result);
        new_reg = mem_wb_reg_.alu_result;
        reg_type = 2; // FPR
    }

    if (old_reg != new_reg)
    {
        current_delta_.register_changes.push_back({reg_index, reg_type, old_reg, new_reg});
    }
}

uint64_t RV5StageVM_Base::execute_float()
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
        // std::cout << GREEN << "Is the alu src set correctly?" << RESET << std::endl;
        reg2_value = static_cast<uint64_t>(static_cast<int64_t>(id_ex_reg_.imm));
        // std::cout << BLUE << "Immediate value used in ALU: " << reg2_value << RESET << std::endl;
    }

    alu::AluOp aluOperation = control_unit_.GetAluSignal(instruction, id_ex_reg_.alu_op > 0);
    std::cout << "The registers values:\n";
    std::cout << reg1_value << ' ' << reg2_value << ' ' << reg3_value << std::endl;
    std::tie(alu_result, fcsr_status) =
        alu::Alu::fpexecute(aluOperation, reg1_value, reg2_value, reg3_value, rm);

    std::cout << "The floating point result : \n";
    std::cout << alu_result << std::endl;

    registers_.WriteCsr(0x003, fcsr_status);
    return alu_result;
}

uint64_t RV5StageVM_Base::execute_double()
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
    std::tie(alu_result, fcsr_status) =
        alu::Alu::dfpexecute(alu_operation, reg1_value, reg2_value, reg3_value, rm);

    registers_.WriteCsr(0x003, fcsr_status);
    return alu_result;
}

bool RV5StageVM_Base::is_pipeline_drained() const
{
    // IF/ID and ID/EX registers directly store the instruction word.
    if (if_id_reg_.instruction != NOP)
        return false;
    if (id_ex_reg_.instruction != NOP)
        return false;
    if (ex_mem_reg_.instruction != NOP)
        return false;
    if (mem_wb_reg_.instruction != NOP)
        return false;
    // EX/MEM: Check for architectural side effects (Reg Write, Mem Read, Mem Write).
    // A NOP will have all these control signals disabled (false).
    if (ex_mem_reg_.reg_write || ex_mem_reg_.mem_read || ex_mem_reg_.mem_write)
        return false;

    // MEM/WB: Check for final architectural side effect (Reg Write).
    if (mem_wb_reg_.reg_write)
        return false;

    return true;
}

void RV5StageVM_Base::Undo()
{
    if (undo_stack_.empty())
    {
        return;
    }

    RV5StageStepDelta last = undo_stack_.top();
    undo_stack_.pop();

    for (auto it = last.register_changes.rbegin(); it != last.register_changes.rend(); ++it)
    {
        const auto &change = *it;
        switch (change.reg_type)
        {
        case 0:
            registers_.WriteGpr(change.reg_index, change.old_value);
            break;
        case 1:
            registers_.WriteCsr(change.reg_index, change.old_value);
            break;
        case 2:
            registers_.WriteFpr(change.reg_index, change.old_value);
            break;
        default:
            break;
        }
    }

    for (auto it = last.memory_changes.rbegin(); it != last.memory_changes.rend(); ++it)
    {
        const auto &change = *it;
        for (size_t i = 0; i < change.old_bytes_vec.size(); ++i)
        {
            memory_controller_.writeByte_d(change.address + i, change.old_bytes_vec[i]);
        }
    }

    program_counter_ = last.old_pc;
    cycle_s_ = last.old_cycle;
    instructions_retired_ = last.old_instructions_retired;
    stall_cycles_ = last.old_stall_cycles;
    branch_mispredictions_ = last.old_branch_mispredictions;

    if_id_reg_ = last.pipeline_register_change.old_if_id_reg;
    id_ex_reg_ = last.pipeline_register_change.old_id_ex_reg;
    ex_mem_reg_ = last.pipeline_register_change.old_ex_mem_reg;
    mem_wb_reg_ = last.pipeline_register_change.old_mem_wb_reg;

    redo_stack_.push(last);

    cpi_ = instructions_retired_
               ? static_cast<float>(cycle_s_) / static_cast<float>(instructions_retired_)
               : 0.0f;
    ipc_ =
        cycle_s_ ? static_cast<float>(instructions_retired_) / static_cast<float>(cycle_s_) : 0.0f;
    setProcessorState();
    SetActiveWireNames();
    emit updateCircuitStateSignal(active_wires_);
    emit processorClockedSignal(processor_state_);
}

void RV5StageVM_Base::Redo()
{
    if (redo_stack_.empty())
    {
        return;
    }

    RV5StageStepDelta next = redo_stack_.top();
    redo_stack_.pop();

    for (const auto &change : next.register_changes)
    {
        switch (change.reg_type)
        {
        case 0:
            registers_.WriteGpr(change.reg_index, change.new_value);
            break;
        case 1:
            registers_.WriteCsr(change.reg_index, change.new_value);
            break;
        case 2:
            registers_.WriteFpr(change.reg_index, change.new_value);
            break;
        default:
            break;
        }
    }

    for (const auto &change : next.memory_changes)
    {
        for (size_t i = 0; i < change.new_bytes_vec.size(); ++i)
        {
            memory_controller_.writeByte(change.address + i, change.new_bytes_vec[i]);
        }
    }

    program_counter_ = next.new_pc;
    cycle_s_ = next.new_cycle;
    instructions_retired_ = next.new_instructions_retired;
    stall_cycles_ = next.new_stall_cycles;
    branch_mispredictions_ = next.new_branch_mispredictions;

    if_id_reg_ = next.pipeline_register_change.new_if_id_reg;
    id_ex_reg_ = next.pipeline_register_change.new_id_ex_reg;
    ex_mem_reg_ = next.pipeline_register_change.new_ex_mem_reg;
    mem_wb_reg_ = next.pipeline_register_change.new_mem_wb_reg;

    undo_stack_.push(next);

    cpi_ = instructions_retired_
               ? static_cast<float>(cycle_s_) / static_cast<float>(instructions_retired_)
               : 0.0f;
    ipc_ =
        cycle_s_ ? static_cast<float>(instructions_retired_) / static_cast<float>(cycle_s_) : 0.0f;
    setProcessorState();
    SetActiveWireNames();
    emit updateCircuitStateSignal(active_wires_);
    emit processorClockedSignal(processor_state_);
}

void RV5StageVM_Base::print_pipeline_registers_debug()
{
    std::cout << "\xEF\xBB\xBF"; // UTF-8 BOM (optional but helps old terminals)

    std::cout << "\n┌──────────────────────────────────────────────────────────┐\n";
    std::cout << "│ Pipeline Debug (Cycle " << cycle_s_ << ")"
              << std::string(34 - std::to_string(cycle_s_).size(), ' ') << "│\n";
    std::cout << "├──────────────────────────────────────────────────────────┤\n";

    std::cout << "│ PC: 0x" << std::hex << std::setw(8) << std::setfill('0') << program_counter_
              << std::dec << "                                           │\n";

    auto inst_to_mnemonic = [](uint32_t inst) -> const char *
    {
        if (inst == NOP)
            return "NOP";
        uint8_t opc = inst & 0x7F;
        switch (opc)
        {
        case 0b1101111:
            return "JAL";
        case 0b1100111:
            return "JALR";
        case 0b1100011:
            return "BR";
        case 0b0000011:
            return "LOAD";
        case 0b0100011:
            return "STORE";
        case 0b0010011:
            return "ALU_IMM";
        case 0b0110011:
            return "ALU_REG";
        case 0b1110011:
            return "SYSTEM";
        default:
            return "OTHER";
        }
    };

    auto print_row = [&](const char *stage, uint32_t inst)
    {
        std::stringstream ss;
        ss << "0x" << std::hex << std::setw(8) << std::setfill('0') << inst << std::dec;

        std::cout << "│ " << std::left << std::setw(6) << stage << "│ " << std::setw(12) << ss.str()
                  << "│ " << std::setw(10) << inst_to_mnemonic(inst) << "│\n";
    };

    std::cout << "├──────────────────────────────────────────────────────────┤\n";
    std::cout << "│ Stage  │ Instruction  │ Mnemonic                         │\n";
    std::cout << "├──────────────────────────────────────────────────────────┤\n";

    print_row("IF/ID", if_id_reg_.instruction);
    print_row("ID/EX", id_ex_reg_.instruction);
    print_row("EX/MEM", ex_mem_reg_.instruction);
    print_row("MEM/WB", mem_wb_reg_.instruction);

    // You had not printed MEM/WB, so I didn’t add it
    // Same logic preserved exactly

    std::cout << "└──────────────────────────────────────────────────────────┘\n";
}
}//namespace Kites
