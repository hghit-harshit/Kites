#include "vm/rv5s_vms/rv5s_vm_base.h"
constexpr uint32_t NOP = 0x00000013;

void RV5StageVM_Base::print_pipeline_registers_debug()
{
    std::cout << "--- Pipeline Debug (Cycle " << cycle_s_ << ") ---" << std::endl;
    std::cout << "PC: 0x" << std::hex << program_counter_ << std::dec << std::endl;

    auto inst_to_mnemonic = [](uint32_t inst) -> const char*
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

    // Pretty-print pipeline registers in an ASCII box
    // Compact table: Stage | Instruction (hex) | Mnemonic
    auto print_row = [&](const char *stage, uint32_t inst)
    {
        std::cout << "| " << stage << " | 0x" << std::hex << inst << std::dec
                  << " | " << inst_to_mnemonic(inst) << " |\n";
    };

    std::cout << "+-----------------------------------------------+\n";
    std::cout << "| Stage   | Instruction (hex) | Mnemonic         |\n";
    std::cout << "+-----------------------------------------------+\n";
    print_row("IF/ID", if_id_reg_.instruction);
    print_row("ID/EX", id_ex_reg_.instruction);
    print_row("EX/MEM", ex_mem_reg_.instruction);
    print_row("MEM/WB", mem_wb_reg_.instruction);
    std::cout << "+-----------------------------------------------+\n";
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