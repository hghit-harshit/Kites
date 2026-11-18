#pragma once
#include "processor/pipeline_registers.h"
#include "processor/processor_base.h"
#include "processor/processor_manager.h"
#include "rv5s_control_unit.h"

namespace Kites
{
struct PipelineRegisterChange
{
    IF_ID_Register old_if_id_reg;
    IF_ID_Register new_if_id_reg;

    ID_EX_Register old_id_ex_reg;
    ID_EX_Register new_id_ex_reg;

    EX_MEM_Register old_ex_mem_reg;
    EX_MEM_Register new_ex_mem_reg;

    MEM_WB_Register old_mem_wb_reg;
    MEM_WB_Register new_mem_wb_reg;
};

struct RV5StageStepDelta : public StepDelta
{
    PipelineRegisterChange pipeline_register_change;
};

class RV5StageVM_Base : public ProcessorBase
{
  public:
    /**
     * @param hazardDetection stall the front end on data hazards (H_* variants)
     * @param forwarding      forward results from EX/MEM and MEM/WB into EX (*_F variants)
     */
    RV5StageVM_Base(bool hazardDetection, bool forwarding);
    ~RV5StageVM_Base() = default;

    // --- VM Control Functions ---

    void PrintType()
    {
        std::cout << "rv5s_vm" << std::endl;
    }
    void DebugRun() override;
    void Step() override;
    void Reset() override;
    void Undo() override;
    void Redo() override;

  protected:
    const bool m_hazardDetection;
    const bool m_forwarding;

    // Set while a hazard stall freezes the front end (IF/ID register and PC).
    bool stall_fetch_and_decode_ = false;

    // Pipeline Registers
    IF_ID_Register if_id_reg_;
    ID_EX_Register id_ex_reg_;
    EX_MEM_Register ex_mem_reg_;
    MEM_WB_Register mem_wb_reg_;

    // The control unit for the pipeline
    RV5SControlUnit control_unit_;

    // Per-cycle deltas; the first entry is an empty sentinel (see UndoBuffer).
    UndoBuffer<RV5StageStepDelta> m_undoBuffer{UndoBuffer<RV5StageStepDelta>::unbounded};
    RV5StageStepDelta current_delta_;

    // Set when EX/MEM redirected the PC this cycle; consumed by fetch_and_advance_pc().
    bool pc_redirected_ = false;

    void memory_writeback();
    void memory_writeback_float();
    void memory_writeback_double();

    void register_write_back(const uint64_t &write_data);
    void memory_read();
    // void memory_read_float();
    // void memory_read_double();

    void begin_step_delta();
    void finalize_step_delta();

    void print_pipeline_registers_debug();
    bool is_pipeline_drained() const;
    void setProcessorState() override;
    void Run() override; // run debug run adn reset are same across all rv5s vms
    bool StepAndNotify() override;

    // --- Pipeline stages ---
    void pipeline_fetch();

    void redirect_pc(uint64_t target);
    void fetch_and_advance_pc(bool stalled);

    void pipeline_decode();

    void pipeline_execute();

    /**
     * @brief Hook run at the start of EX. Returning true skips the rest of the stage, leaving
     * EX/MEM untouched. Only the no-hazard/no-forwarding variant uses it (for SYSTEM opcodes).
     */
    virtual bool handleSystemInExecute()
    {
        return false;
    }

    /// @brief Hazard-detection stall decision for this cycle; updates the stall countdown.
    bool computeStall();

    /// @brief Integer ALU path of EX; also latches the (forwarded) store data into EX/MEM.
    uint64_t executeInteger();
    /// @brief F/D path of EX; also latches the (forwarded) FP store data into EX/MEM.
    uint64_t executeFp(bool isDouble);
    /// @brief rs1 operand of an F/D instruction: a GPR for loads/stores/conversions, else an FPR.
    uint64_t fpRs1Operand(bool isDouble) const;

    /// @brief Value of GPR @p rs seen in EX: forwarded when forwarding is enabled.
    uint64_t gprOperand(uint8_t rs, uint64_t latched) const
    {
        return m_forwarding ? forward_gpr(rs, latched) : latched;
    }
    /// @brief Value of FPR @p frs seen in EX: forwarded when forwarding is enabled.
    uint64_t fprOperand(uint8_t frs, uint64_t latched) const
    {
        return m_forwarding ? forwardFpr(frs, latched) : latched;
    }

    /// @brief LUI/AUIPC results replace the ALU output.
    uint64_t applyUpperImmediate(uint64_t aluResult) const;
    /// @brief Copies ID/EX into EX/MEM (except store data, which the execute paths latch).
    void latchExMem(uint64_t aluResult, bool isFp);
    /// @brief Resolves a taken branch (flagged for MEM) or a JAL/JALR redirect.
    void resolveBranchOrJump(uint64_t aluResult);

    /**
     * @brief Value of GPR @p rs as seen by the instruction in EX, with forwarding applied:
     * EX/MEM first, then MEM/WB, else the value latched in ID. Used by the forwarding
     * pipelines for FP instructions that read a GPR (load/store base, fcvt from int, fmv.*.x).
     */
    uint64_t forward_gpr(uint8_t rs, uint64_t latched) const
    {
        if (rs == 0)
        {
            return 0;
        }
        if (ex_mem_reg_.reg_write && ex_mem_reg_.rd == rs)
        {
            return ex_mem_reg_.alu_result;
        }
        if (mem_wb_reg_.prev_reg_write && mem_wb_reg_.prev_rd == rs)
        {
            return mem_wb_reg_.prev_mem_to_reg ? mem_wb_reg_.prev_memory_data
                                               : mem_wb_reg_.prev_alu_result;
        }
        return latched;
    }

    /**
     * @brief Value of FPR @p frs as seen by the instruction in EX, with forwarding applied:
     * EX/MEM first, then MEM/WB, else the value latched in ID. f0 is a normal register.
     */
    uint64_t forwardFpr(uint8_t frs, uint64_t latched) const
    {
        if (ex_mem_reg_.freg_write && ex_mem_reg_.frd == frs)
        {
            return ex_mem_reg_.f_alu_result;
        }
        if (mem_wb_reg_.prev_freg_write && mem_wb_reg_.prev_frd == frs)
        {
            return mem_wb_reg_.prev_mem_to_reg ? mem_wb_reg_.prev_f_memory_data
                                               : mem_wb_reg_.prev_f_alu_result;
        }
        return latched;
    }

    void pipeline_memory();
    void pipeline_writeback();
    void pipeline_writeback_float();
    void pipeline_writeback_double();
};
}//namespace Kites