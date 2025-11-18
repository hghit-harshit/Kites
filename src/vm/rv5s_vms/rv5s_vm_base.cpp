#include "vm/rv5s_vms/rv5s_vm_base.h"
constexpr uint32_t NOP = 0x00000013;
#include <thread>


void RV5StageVM_Base::Run()
{
    ClearStop();
    while (!stop_requested_ && (program_counter_ < program_size_ || !is_pipeline_drained()))
    {
        //handling pause
        {
            QMutexLocker locker(&pause_mutex_);
            // using while because thie wait can be interrupted by spurious wakeups
            while(pause_requested_ && !stop_requested_)
                pause_wait_condition_.wait(&pause_mutex_);
            
            if(stop_requested_) // if we were requested to stop while paused
                break;
        }
        Step();
        SetVMStateMap();
        SetActiveWireNames();
        emit updateCircuitStateSignal(active_wires_);
        emit vmStateChangedSignal(vm_state_);
        
        // handling the delay
        {
            QMutexLocker locker(&pause_mutex_);
            if(stop_requested_ || pause_requested_)
                continue;
            pause_wait_condition_.wait(&pause_mutex_, step_delay_);
            // we wait for step_delay_ milliseconds or until notified to wake up
            
        }
    }
    if(stop_requested_)
    {
        emit vmStateChangedSignal(vm_state_);
        // we emit the vm state changed signal once more to update the ui
        // this will get rid of any highlights as they pause when we stop 
        // before execution is complete
    }
    //vm_state_["EditorLines"] = {};
   // vm_state_["DisassemblyLines"] = {};
   // emit vmStateChangedSignal(vm_state_);
    // we emit it once more after the pipeline is drained 
    // so the ui can get the final state and remove the last highlight
}

void RV5StageVM_Base::DebugRun()
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
    }
    print_pipeline_registers_debug();
}

void RV5StageVM_Base::SetVMStateMap()
{
    vm_state_.clear();
	vm_state_["ProgramCounter"] = static_cast<qulonglong>(program_counter_);
	vm_state_["CurrentInstruction"] = static_cast<qulonglong>(current_instruction_);
	vm_state_["Cycles"] = static_cast<qulonglong>(cycle_s_);
	vm_state_["InstructionsRetired"] = static_cast<qulonglong>(instructions_retired_);
	vm_state_["CPI"] = static_cast<double>(cpi_);
	vm_state_["IPC"] = static_cast<double>(ipc_);
	vm_state_["StallCycles"] = static_cast<qulonglong>(stall_cycles_);
	vm_state_["BranchMispredictions"] = static_cast<qulonglong>(branch_mispredictions_);
    
    vm_state_["EditorLines"] = QVariantMap{
        {"CI", static_cast<qulonglong>(program_.instruction_number_line_number_mapping[(program_counter_)/ 4])},
        {"IF/ID",(if_id_reg_.pc < UINT64_MAX ? static_cast<qulonglong>(program_.instruction_number_line_number_mapping[(if_id_reg_.pc)/ 4]) : -1)},
        {"ID/EX",(id_ex_reg_.pc < UINT64_MAX ? static_cast<qulonglong>(program_.instruction_number_line_number_mapping[(id_ex_reg_.pc)/ 4]) : -1)},
        {"EX/MEM",(ex_mem_reg_.pc < UINT64_MAX ? static_cast<qulonglong>(program_.instruction_number_line_number_mapping[(ex_mem_reg_.pc)/ 4]) : -1)},
        {"MEM/WB", (mem_wb_reg_.pc < UINT64_MAX ? static_cast<qulonglong>(program_.instruction_number_line_number_mapping[(mem_wb_reg_.pc)/ 4]) : -1)} }; 
	vm_state_["DisassemblyLines"] = QVariantMap{
        {"CI",static_cast<qulonglong>(program_.instruction_number_disassembly_mapping[(program_counter_)/ 4])},
        {"IF/ID",(if_id_reg_.pc < UINT64_MAX ? static_cast<qulonglong>(program_.instruction_number_disassembly_mapping[(if_id_reg_.pc)/ 4]) : -1)},
        {"ID/EX",(id_ex_reg_.pc < UINT64_MAX ? static_cast<qulonglong>(program_.instruction_number_disassembly_mapping[(id_ex_reg_.pc)/ 4]) : -1)},
        {"EX/MEM",(ex_mem_reg_.pc < UINT64_MAX ? static_cast<qulonglong>(program_.instruction_number_disassembly_mapping[(ex_mem_reg_.pc)/ 4]) : -1)},
        {"MEM/WB",(mem_wb_reg_.pc < UINT64_MAX ? static_cast<qulonglong>(program_.instruction_number_disassembly_mapping[(mem_wb_reg_.pc)/ 4]) : -1)}};
}


void RV5StageVM_Base::print_pipeline_registers_debug()
{
    std::cout << "\xEF\xBB\xBF"; // UTF-8 BOM (optional but helps old terminals)

    std::cout << "\n┌──────────────────────────────────────────────────────────┐\n";
    std::cout << "│ Pipeline Debug (Cycle " << cycle_s_ << ")"
              << std::string(34 - std::to_string(cycle_s_).size(), ' ')
              << "│\n";
    std::cout << "├──────────────────────────────────────────────────────────┤\n";

    std::cout << "│ PC: 0x" << std::hex << std::setw(8) << std::setfill('0')
              << program_counter_ << std::dec
              << "                                           │\n";

    auto inst_to_mnemonic = [](uint32_t inst) -> const char*
    {
        if (inst == NOP) return "NOP";
        uint8_t opc = inst & 0x7F;
        switch (opc)
        {
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

    auto print_row = [&](const char *stage, uint32_t inst)
    {
        std::stringstream ss;
        ss << "0x" << std::hex << std::setw(8) << std::setfill('0') << inst << std::dec;

        std::cout << "│ " << std::left << std::setw(6) << stage
                  << "│ " << std::setw(12) << ss.str()
                  << "│ " << std::setw(10) << inst_to_mnemonic(inst)
                  << "│\n";
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


bool RV5StageVM_Base::is_pipeline_drained() const
{
    // IF/ID and ID/EX registers directly store the instruction word.
    if (if_id_reg_.instruction != NOP) return false;
    if (id_ex_reg_.instruction != NOP) return false;
    if(ex_mem_reg_.instruction != NOP) return false;
    if(mem_wb_reg_.instruction != NOP) return false;
    // EX/MEM: Check for architectural side effects (Reg Write, Mem Read, Mem Write).
    // A NOP will have all these control signals disabled (false).
    if (ex_mem_reg_.reg_write || ex_mem_reg_.mem_read || ex_mem_reg_.mem_write) return false;

    // MEM/WB: Check for final architectural side effect (Reg Write).
    if (mem_wb_reg_.reg_write) return false;
    
    return true;
}