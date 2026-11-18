#include "common/debug_colors.h"
#include "common/instructions.h"
#include "processor/rv5s/rv5s_processor_base.h"
#include "processor/rv5s/rv5s_hdu.h"
#include <tuple>
#include <thread>

namespace Kites
{
RV5StageVM_Base::RV5StageVM_Base(bool hazardDetection, bool forwarding)
    : m_hazardDetection(hazardDetection), m_forwarding(forwarding)
{
}

void RV5StageVM_Base::Step()
{
    begin_step_delta();

    // Back stages first (WB -> MEM -> EX) so their results are visible to ID and IF.
    pipeline_writeback();
    pipeline_memory();
    pipeline_execute();

    if (m_hazardDetection && computeStall())
    {
        // Freeze IF/ID and the PC, and inject a bubble into ID/EX.
        id_ex_reg_.reset();
        stall_fetch_and_decode_ = true;
    }
    else
    {
        pipeline_decode();
        stall_fetch_and_decode_ = false;
    }

    fetch_and_advance_pc(stall_fetch_and_decode_);
    cycle_s_++;

    finalize_step_delta();
}

bool RV5StageVM_Base::computeStall()
{
    // stall_cycles_ doubles as the countdown of an ongoing multi-cycle stall.
    if (stall_cycles_ > 0)
    {
        stall_cycles_--;
        return true;
    }

    const int stallsNeeded = check_data_hazard(if_id_reg_, id_ex_reg_, ex_mem_reg_, m_forwarding);
    if (stallsNeeded > 0)
    {
        stall_cycles_ = stallsNeeded - 1; // this cycle is the first stall
        return true;
    }
    return false;
}

void RV5StageVM_Base::pipeline_fetch()
{
    if (stall_fetch_and_decode_)
    {
        return; // IF/ID keeps the stalled instruction
    }

    if (program_counter_ < program_size_)
    {
        if_id_reg_.instruction = memory_controller_.readInstruction(program_counter_);
        if_id_reg_.pc = program_counter_;
    }
    else
    {
        // Past the end of the program, inject NOPs to drain the pipeline.
        if_id_reg_.reset();
    }
}

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
        StepAndNotify();

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
        // we emit the processor clocked signal once more to update the ui
        // this will get rid of any highlights as they pause when we stop
        // before execution is complete
    }
}

bool RV5StageVM_Base::StepAndNotify()
{
    QMutexLocker locker(&step_mutex_);
    if (program_counter_ >= program_size_ && is_pipeline_drained())
    {
        return false;
    }
    last_executed_pc_ = id_ex_reg_.pc; // this pc will be executed in this step,
    //so we set it as the last executed pc before we step
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
    return program_counter_ < program_size_ || !is_pipeline_drained();
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
    pc_redirected_        = false;
    stall_fetch_and_decode_ = false;
    last_breakpoint_pc_.reset(); // Clear breakpoint tracking on reset

    registers_.Reset();
    memory_controller_.reset();
    control_unit_.Reset();

    if_id_reg_.reset();
    id_ex_reg_.reset();
    ex_mem_reg_.reset();
    mem_wb_reg_.reset();
    current_delta_ = RV5StageStepDelta{};
    m_undoBuffer.clear();
    m_undoBuffer.push(RV5StageStepDelta{});
    processor_state_.reset();
}

void RV5StageVM_Base::begin_step_delta()
{
    memory_controller_.beginStep();
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

    m_undoBuffer.push(current_delta_); // also drops any redo history
    memory_controller_.commitStep();
}

void RV5StageVM_Base::setProcessorState()
{
    // processor_state_.programCounters[toIndex(PipelineStage::IF)]  = program_counter_;
    // processor_state_.programCounters[toIndex(PipelineStage::ID)]  = if_id_reg_.pc;
    // processor_state_.programCounters[toIndex(PipelineStage::EX)]  = id_ex_reg_.pc;
    // processor_state_.programCounters[toIndex(PipelineStage::MEM)] = ex_mem_reg_.pc;
    // processor_state_.programCounters[toIndex(PipelineStage::WB)]  = mem_wb_reg_.pc;
    processor_state_.programCounters = 
    {program_counter_, if_id_reg_.pc, id_ex_reg_.pc, ex_mem_reg_.pc, mem_wb_reg_.pc};
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
    const bool is_fp = instruction_set::isFInstruction(instruction) ||
                       instruction_set::isDInstruction(instruction);
    if (is_fp)
    {
        id_ex_reg_.frs1 = (instruction >> 15) & 0x1F;
        id_ex_reg_.frs2 = (instruction >> 20) & 0x1F;
        id_ex_reg_.frs3 = (instruction >> 27) & 0x1F;
        id_ex_reg_.frd = (instruction >> 7) & 0x1F;

        id_ex_reg_.freg1_data = registers_.ReadFpr(id_ex_reg_.frs1);
        id_ex_reg_.freg2_data = registers_.ReadFpr(id_ex_reg_.frs2);
        id_ex_reg_.freg3_data = registers_.ReadFpr(id_ex_reg_.frs3);

        // Loads/stores (base address), fcvt-from-int and fmv.*.x read a GPR via rs1.
        // Latch it here so EX can forward it like any integer operand. rs2 is an FPR.
        id_ex_reg_.reg1_data = registers_.ReadGpr(id_ex_reg_.rs1);
        id_ex_reg_.reg2_data = 0;
    }
    else
    {
        id_ex_reg_.frs1 = id_ex_reg_.frs2 = id_ex_reg_.frs3 = id_ex_reg_.frd = 0;
        id_ex_reg_.reg1_data = registers_.ReadGpr(id_ex_reg_.rs1);
        id_ex_reg_.reg2_data = registers_.ReadGpr(id_ex_reg_.rs2);
    }

    // Pass all control signals to the next stage.
    // reg_write = writes a GPR; freg_write = writes an FPR. They are mutually exclusive, so
    // GPR forwarding/hazard logic never acts on an FP destination and vice versa.
    id_ex_reg_.reg_write  = is_fp ? instruction_set::fpWritesGpr(instruction)
                                  : control_unit_.GetRegWrite();
    id_ex_reg_.freg_write = instruction_set::fpWritesFpr(instruction);
    id_ex_reg_.branch     = control_unit_.GetBranch();
    id_ex_reg_.alu_src    = control_unit_.GetAluSrc();
    id_ex_reg_.mem_read   = control_unit_.GetMemRead();
    id_ex_reg_.mem_write  = control_unit_.GetMemWrite();
    id_ex_reg_.mem_to_reg = control_unit_.GetMemToReg();
    id_ex_reg_.alu_op     = control_unit_.GetAluOp();
}

// Undo of stores is handled by the memory controller's per-step history.
void RV5StageVM_Base::memory_writeback()
{
    switch ((mem_wb_reg_.instruction >> 12) & 0b111)
    {
    case 0b000: // SB
        memory_controller_.writeByte(ex_mem_reg_.alu_result, ex_mem_reg_.reg2_data & 0xFF);
        break;
    case 0b001: // SH
        memory_controller_.writeHalfWord(ex_mem_reg_.alu_result, ex_mem_reg_.reg2_data & 0xFFFF);
        break;
    case 0b010: // SW
        memory_controller_.writeWord(ex_mem_reg_.alu_result, ex_mem_reg_.reg2_data & 0xFFFFFFFF);
        break;
    case 0b011: // SD
        memory_controller_.writeDoubleWord(ex_mem_reg_.alu_result,
                                           ex_mem_reg_.reg2_data & 0xFFFFFFFFFFFFFFFF);
        break;
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

void RV5StageVM_Base::redirect_pc(uint64_t target)
{
    program_counter_ = target;
    pc_redirected_ = true;
}

// IF stage plus PC update. Runs after WB/MEM/EX/ID so a redirect resolved this cycle
// is visible. The target is fetched next cycle, leaving a bubble in IF/ID now, the same
// as hardware that only learns the target at the end of the resolving stage.
void RV5StageVM_Base::fetch_and_advance_pc(bool stalled)
{
    if (pc_redirected_)
    {
        if_id_reg_.reset();
        pc_redirected_ = false;
        return;
    }

    pipeline_fetch();
    // A stall skipped the fetch, so hold the PC on the instruction still to be fetched.
    if (!stalled)
    {
        program_counter_ += 4;
    }
}

void RV5StageVM_Base::pipeline_memory()
{

    // --- B-Type Conditional Branch Resolution (3-Cycle Penalty) ---
    if (ex_mem_reg_.branch_taken && (ex_mem_reg_.instruction & 0b1111111) == 0b1100011)
    {
        // B-Type misprediction confirmed in MEM stage. Hardware flushes the pipeline.

        redirect_pc(ex_mem_reg_.branch_target_pc);
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
    }
    else if (ex_mem_reg_.mem_write)
    {
        if (is_F_Instruction)
        { // FSW
            memory_controller_.writeWord(ex_mem_reg_.alu_result,
                                         ex_mem_reg_.freg2_data & 0xFFFFFFFF);
        }
        else if (is_D_Instruction)
        { // FSD
            memory_controller_.writeDoubleWord(ex_mem_reg_.alu_result,
                                               ex_mem_reg_.freg2_data & 0xFFFFFFFFFFFFFFFF);
        }
        else
        {
            memory_writeback();
        }
    }
}

void RV5StageVM_Base::pipeline_writeback()
{
    // Every real instruction reaching WB retires, whether or not it writes a register
    // (stores, branches, jumps to x0, nops). Bubbles carry INVALID_PC.
    if (mem_wb_reg_.pc != INVALID_PC)
    {
        instructions_retired_++;
    }

    if (!mem_wb_reg_.reg_write && !mem_wb_reg_.freg_write)
    {
        return;
    }

    // --- FP Writeback Delegation ---
    // Before the rd != 0 check: for FP ops rd names an FPR, and f0 is writable.
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

    if (mem_wb_reg_.rd != 0)
    {
        uint64_t write_data =
            mem_wb_reg_.mem_to_reg ? mem_wb_reg_.memory_data : mem_wb_reg_.alu_result;
        // JAL/JALR write the link address, not the ALU result.
        uint8_t opcode = mem_wb_reg_.instruction & 0b1111111;
        if (opcode == 0b1100111 || opcode == 0b1101111)
        {
            write_data = mem_wb_reg_.pc + 4;
        }

        // Record state for Undo/Redo
        uint64_t old_value = registers_.ReadGpr(mem_wb_reg_.rd);
        if (old_value != write_data)
        {
            current_delta_.register_changes.push_back({mem_wb_reg_.rd,
                                                       0, // GPR type
                                                       old_value, write_data});
        }

        switch (opcode)
        {
        case 0b0110011: // R-Type
        case 0b0010011: // I-Type
        case 0b0010111: // AUIPC
        case 0b0111011: // OP-32 R-type (ADDW, SUBW, MULW, DIVW, DIVUW, REMW, REMUW, SLLW, SRLW, SRAW)
        case 0b0011011: // OP-32 I-type (ADDIW, SLLIW, SRLIW, SRAIW)
        {
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

void RV5StageVM_Base::pipeline_execute()
{
    if (handleSystemInExecute())
    {
        return;
    }

    const uint32_t instruction = id_ex_reg_.instruction;
    const bool isFp = instruction_set::isFInstruction(instruction) ||
                      instruction_set::isDInstruction(instruction);

    uint64_t aluResult =
        isFp ? executeFp(instruction_set::isDInstruction(instruction)) : executeInteger();
    aluResult = applyUpperImmediate(aluResult);

    latchExMem(aluResult, isFp);
    resolveBranchOrJump(aluResult);
}

uint64_t RV5StageVM_Base::executeInteger()
{
    const uint32_t instruction = id_ex_reg_.instruction;

    const uint64_t aluIn1 = gprOperand(id_ex_reg_.rs1, id_ex_reg_.reg1_data);
    const uint64_t rs2Value = gprOperand(id_ex_reg_.rs2, id_ex_reg_.reg2_data);
    const uint64_t aluIn2 = id_ex_reg_.alu_src ? static_cast<uint64_t>(id_ex_reg_.imm) : rs2Value;

    alu::AluOp aluOperation = control_unit_.GetAluSignal(instruction, id_ex_reg_.alu_op > 0);
    uint64_t aluResult = 0;
    bool overflow = false;
    std::tie(aluResult, overflow) = alu::Alu::execute(aluOperation, aluIn1, aluIn2);

    // Store data must see the same (possibly forwarded) rs2 value as the ALU.
    ex_mem_reg_.reg2_data = rs2Value;
    return aluResult;
}

uint64_t RV5StageVM_Base::fpRs1Operand(bool isDouble) const
{
    const uint32_t instruction = id_ex_reg_.instruction;
    if (m_forwarding)
    {
        return instruction_set::fpReadsGprRs1(instruction)
                   ? forward_gpr(id_ex_reg_.rs1, id_ex_reg_.reg1_data)
                   : forwardFpr(id_ex_reg_.frs1, id_ex_reg_.freg1_data);
    }

    // Without forwarding the GPR is read from the register file in EX, i.e. after this
    // cycle's write back. Loads/stores use it as base address; fcvt/fmv from int as source.
    const uint8_t opcode = instruction & 0b1111111;
    const uint8_t funct7 = (instruction >> 25) & 0b1111111;
    const uint8_t cvtFromInt = isDouble ? 0b1101001 : 0b1101000;
    const uint8_t moveFromInt = isDouble ? 0b1111001 : 0b1111000;
    if (funct7 == cvtFromInt || funct7 == moveFromInt || opcode == 0b0000111 ||
        opcode == 0b0100111)
    {
        return registers_.ReadGpr(id_ex_reg_.rs1);
    }
    return id_ex_reg_.freg1_data;
}

uint64_t RV5StageVM_Base::executeFp(bool isDouble)
{
    const uint32_t instruction = id_ex_reg_.instruction;

    uint8_t rm = (instruction >> 12) & 0b111;
    if (rm == 0b111) // dynamic rounding mode
    {
        rm = registers_.ReadCsr(0x002);
    }

    const uint64_t frs2Value = fprOperand(id_ex_reg_.frs2, id_ex_reg_.freg2_data);
    const uint64_t reg1Value = fpRs1Operand(isDouble);
    // two casts: the inner one sign-extends the immediate
    const uint64_t reg2Value =
        id_ex_reg_.alu_src ? static_cast<uint64_t>(static_cast<int64_t>(id_ex_reg_.imm))
                           : frs2Value;
    const uint64_t reg3Value = fprOperand(id_ex_reg_.frs3, id_ex_reg_.freg3_data);

    // Store data for fsw/fsd (frs2), forwarded like any other operand.
    ex_mem_reg_.freg2_data = frs2Value;

    alu::AluOp aluOperation = control_unit_.GetAluSignal(instruction, id_ex_reg_.alu_op > 0);
    uint64_t aluResult = 0;
    uint8_t fcsrStatus = 0;
    if (isDouble)
    {
        std::tie(aluResult, fcsrStatus) =
            alu::Alu::dfpexecute(aluOperation, reg1Value, reg2Value, reg3Value, rm);
    }
    else
    {
        std::tie(aluResult, fcsrStatus) =
            alu::Alu::fpexecute(aluOperation, reg1Value, reg2Value, reg3Value, rm);
    }

    registers_.WriteCsr(0x003, fcsrStatus);
    return aluResult;
}

uint64_t RV5StageVM_Base::applyUpperImmediate(uint64_t aluResult) const
{
    const uint8_t opcode = id_ex_reg_.instruction & 0b1111111;
    if (opcode == 0b0110111) // lui
    {
        return static_cast<uint64_t>(id_ex_reg_.imm << 12);
    }
    if (opcode == 0b0010111) // auipc
    {
        return id_ex_reg_.pc + static_cast<uint64_t>(static_cast<int64_t>(static_cast<int32_t>(
                                   static_cast<uint32_t>(id_ex_reg_.imm) << 12)));
    }
    return aluResult;
}

void RV5StageVM_Base::latchExMem(uint64_t aluResult, bool isFp)
{
    ex_mem_reg_.prev_reg_write = ex_mem_reg_.reg_write;
    ex_mem_reg_.prev_rd = ex_mem_reg_.rd;
    ex_mem_reg_.prev_freg_write = ex_mem_reg_.freg_write;
    ex_mem_reg_.prev_frd = ex_mem_reg_.frd;
    ex_mem_reg_.pc = id_ex_reg_.pc;
    ex_mem_reg_.instruction = id_ex_reg_.instruction;
    ex_mem_reg_.alu_result = aluResult;
    ex_mem_reg_.f_alu_result = isFp ? aluResult : 0;
    ex_mem_reg_.rd = id_ex_reg_.rd;
    ex_mem_reg_.frd = id_ex_reg_.frd;
    if (!m_forwarding)
    {
        // Without forwarding both store-data fields come straight from ID/EX. With forwarding
        // the execute path latched its own (forwarded) one and leaves the other untouched.
        ex_mem_reg_.reg2_data = id_ex_reg_.reg2_data;
        ex_mem_reg_.freg2_data = id_ex_reg_.freg2_data;
    }
    ex_mem_reg_.reg_write = id_ex_reg_.reg_write;
    ex_mem_reg_.freg_write = id_ex_reg_.freg_write;
    ex_mem_reg_.mem_to_reg = id_ex_reg_.mem_to_reg;
    ex_mem_reg_.prev_mem_read = ex_mem_reg_.mem_read;
    ex_mem_reg_.prev_mem_write = ex_mem_reg_.mem_write;
    ex_mem_reg_.mem_read = id_ex_reg_.mem_read;
    ex_mem_reg_.mem_write = id_ex_reg_.mem_write;
    ex_mem_reg_.prev_branch_taken = ex_mem_reg_.branch_taken;
    ex_mem_reg_.branch_taken = false;
    ex_mem_reg_.branch_target_pc = 0;
}

void RV5StageVM_Base::resolveBranchOrJump(uint64_t aluResult)
{
    const uint32_t instruction = id_ex_reg_.instruction;
    const uint8_t opcode = instruction & 0b1111111;

    // Conditional branch (B-type): the ALU computed SUB (BEQ/BNE) or SLT/SLTU (the rest).
    if (id_ex_reg_.branch && opcode == 0b1100011)
    {
        bool conditionMet = false;
        switch ((instruction >> 12) & 0x7)
        {
        case 0b000: // BEQ
            conditionMet = (aluResult == 0);
            break;
        case 0b001: // BNE
            conditionMet = (aluResult != 0);
            break;
        case 0b100: // BLT
        case 0b110: // BLTU
            conditionMet = (aluResult == 1);
            break;
        case 0b101: // BGE
        case 0b111: // BGEU
            conditionMet = (aluResult == 0);
            break;
        }

        if (conditionMet)
        {
            // MEM redirects the PC and flushes.
            ex_mem_reg_.branch_taken = true;
            ex_mem_reg_.branch_target_pc = id_ex_reg_.pc + id_ex_reg_.imm;
        }
    }
    // Unconditional jump (JAL/JALR): redirect now, killing the instruction in IF/ID.
    else if (opcode == 0b1101111 || opcode == 0b1100111)
    {
        uint64_t jumpTarget = 0;
        if (opcode == 0b1101111)
        {
            jumpTarget = id_ex_reg_.pc + id_ex_reg_.imm; // JAL
        }
        else
        {
            jumpTarget = aluResult & ~1;                // JALR (ALU result is rs1 + imm)
            ex_mem_reg_.alu_result = id_ex_reg_.pc + 4; // link address
        }

        redirect_pc(jumpTarget);
        if_id_reg_.reset();
        ex_mem_reg_.branch_taken = true;
    }
}

bool RV5StageVM_Base::is_pipeline_drained() const
{
    // A stage holds a real instruction (possibly an actual `nop`) unless its PC is
    // INVALID_PC, which is what flushes, stalls and fetches past the end leave behind.
    if (if_id_reg_.pc != INVALID_PC || id_ex_reg_.pc != INVALID_PC ||
        ex_mem_reg_.pc != INVALID_PC || mem_wb_reg_.pc != INVALID_PC)
        return false;
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
    if (ex_mem_reg_.reg_write || ex_mem_reg_.freg_write || ex_mem_reg_.mem_read ||
        ex_mem_reg_.mem_write)
        return false;

    // MEM/WB: Check for final architectural side effect (GPR or FPR write).
    if (mem_wb_reg_.reg_write || mem_wb_reg_.freg_write)
        return false;

    return true;
}

void RV5StageVM_Base::Undo()
{
    if (!m_undoBuffer.canUndo())
    {
        return;
    }

    const RV5StageStepDelta &last = m_undoBuffer.current();

    for (auto it = last.register_changes.rbegin(); it != last.register_changes.rend(); ++it)
    {
        writeChangedRegister(*it, it->old_value);
    }
    memory_controller_.undoStep(); // main memory and caches, without counting as accesses

    program_counter_ = last.old_pc;
    cycle_s_ = last.old_cycle;
    instructions_retired_ = last.old_instructions_retired;
    stall_cycles_ = last.old_stall_cycles;
    branch_mispredictions_ = last.old_branch_mispredictions;

    if_id_reg_ = last.pipeline_register_change.old_if_id_reg;
    id_ex_reg_ = last.pipeline_register_change.old_id_ex_reg;
    ex_mem_reg_ = last.pipeline_register_change.old_ex_mem_reg;
    mem_wb_reg_ = last.pipeline_register_change.old_mem_wb_reg;

    m_undoBuffer.undo();

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
    auto entry = m_undoBuffer.redo();
    if (!entry)
    {
        return;
    }

    const RV5StageStepDelta &next = entry->get();

    for (const auto &change : next.register_changes)
    {
        writeChangedRegister(change, change.new_value);
    }
    memory_controller_.redoStep(); // main memory and caches, without counting as accesses

    program_counter_ = next.new_pc;
    cycle_s_ = next.new_cycle;
    instructions_retired_ = next.new_instructions_retired;
    stall_cycles_ = next.new_stall_cycles;
    branch_mispredictions_ = next.new_branch_mispredictions;

    if_id_reg_ = next.pipeline_register_change.new_if_id_reg;
    id_ex_reg_ = next.pipeline_register_change.new_id_ex_reg;
    ex_mem_reg_ = next.pipeline_register_change.new_ex_mem_reg;
    mem_wb_reg_ = next.pipeline_register_change.new_mem_wb_reg;

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
