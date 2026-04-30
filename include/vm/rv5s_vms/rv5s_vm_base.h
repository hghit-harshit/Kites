#pragma once 
#include "vm/vm_base.h"
#include "vm/pipeline_registers.h"
#include "vm/vm_manager.h"
#include "rv5s_control_unit.h"


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

struct RV5StageStepDelta
{
    uint64_t old_pc;
    uint64_t new_pc;
    unsigned int old_cycle;
    unsigned int new_cycle;
    unsigned int old_instructions_retired;
    unsigned int new_instructions_retired;
    unsigned int old_stall_cycles;
    unsigned int new_stall_cycles;
    unsigned int old_branch_mispredictions;
    unsigned int new_branch_mispredictions;
    std::vector<RegisterChange> register_changes;
	std::vector<MemoryChange> memory_changes;
    PipelineRegisterChange pipeline_register_change;
};



class RV5StageVM_Base : public VmBase
{
public:
    RV5StageVM_Base() = default;
    ~RV5StageVM_Base() = default;

    // --- VM Control Functions ---
 
    void PrintType()
    {
        std::cout << "rv5s_vm" << std::endl;
    }
    void DebugRun() override;
    void Reset() override;
    void Undo() override;
    void Redo() override;
protected:

    // Pipeline Registers
    IF_ID_Register if_id_reg_;
    ID_EX_Register id_ex_reg_;
    EX_MEM_Register ex_mem_reg_;
    MEM_WB_Register mem_wb_reg_;

    // The control unit for the pipeline
    RV5SControlUnit control_unit_;

    std::stack<RV5StageStepDelta> undo_stack_{};
    std::stack<RV5StageStepDelta> redo_stack_{};
    RV5StageStepDelta current_delta_{};

    void memory_writeback();
    void memory_writeback_float();
    void memory_writeback_double();

    void register_write_back(const uint64_t& write_data);
    void memory_read();
    //void memory_read_float();
    //void memory_read_double();

   

    void begin_step_delta();
    void finalize_step_delta();

    void print_pipeline_registers_debug();
    bool is_pipeline_drained() const;
    void SetVMStateMap() override;
    void Run() override; // run debug run adn reset are same across all rv5s vms
    
    // --- Private methods for each pipeline stage ---
    virtual void pipeline_fetch() = 0;

    void pipeline_decode();

    virtual void pipeline_execute() = 0;
    virtual uint64_t pipeline_execute_float() = 0;
    virtual uint64_t pipeline_execute_double() = 0;

    uint64_t execute_float();
    uint64_t execute_double();

    void execute_csr(){};
    // therse function are same across all rv5s vms so we can define them here
    // and implement them in the .cpp file
    void pipeline_memory();
    //void pipeline_memory_float();
    //void pipeline_memory_double();


    void pipeline_writeback();
    void pipeline_writeback_float();
    void pipeline_writeback_double();

    // --- Specialized handler functions (called from pipeline stages) ---
    virtual void handle_syscall() = 0;
};