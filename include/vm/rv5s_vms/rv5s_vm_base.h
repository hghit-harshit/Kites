#pragma once 
#include "vm/vm_base.h"
#include "vm/pipeline_registers.h"
#include "vm/vm_manager.h"
#include "rv5s_control_unit.h"


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
protected:

    // Pipeline Registers
    IF_ID_Register if_id_reg_;
    ID_EX_Register id_ex_reg_;
    EX_MEM_Register ex_mem_reg_;
    MEM_WB_Register mem_wb_reg_;

    // The control unit for the pipeline
    RV5SControlUnit control_unit_;

    void memory_writeback();
    void memory_writeback_float();
    void memory_writeback_double();

    void register_write_back(const uint64_t& write_data);
    void memory_read();
    void memory_read_float();
    void memory_read_double();


    void print_pipeline_registers_debug();
    bool is_pipeline_drained() const;
    void SetVMStateMap() override;
    void Run() override; // run debug run adn reset are same across all rv5s vms
    
    // --- Private methods for each pipeline stage ---
    virtual void pipeline_fetch() = 0;
    void pipeline_decode();
    virtual void pipeline_execute() = 0;
    virtual void pipeline_execute_float(){};
    virtual void pipeline_execute_double() {};

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