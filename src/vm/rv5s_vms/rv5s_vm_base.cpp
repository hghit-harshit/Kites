#include "vm/rv5s_vms/rv5s_vm_base.h"
constexpr uint32_t NOP = 0x00000013;

// void RV5StageVM_Base::print_pipeline_registers_debug()
// {
//     auto inst_to_mnemonic = [](uint32_t inst) -> const char*
//     {
//         if (inst == NOP) return "NOP";
//         uint8_t opc = inst & 0x7F;
//         switch (opc)
//         {
//             case 0b1101111: return "JAL";
//             case 0b1100111: return "JALR";
//             case 0b1100011: return "BR";
//             case 0b0000011: return "LOAD";
//             case 0b0100011: return "STORE";
//             case 0b0010011: return "ALU_IMM";
//             case 0b0110011: return "ALU_REG";
//             case 0b1110011: return "SYSTEM";
//             default: return "OTHER";
//         }
//     };

//     auto line = []()
//     {
//         std::cout << "\u250C";                 // ┌
//         for (int i = 0; i < 55; i++) std::cout << "\u2500"; // ────
//         std::cout << "\u2510\n";              // ┐
//     };

//     auto separator = []()
//     {
//         std::cout << "\u251C";                 // ├
//         for (int i = 0; i < 55; i++) std::cout << "\u2500";
//         std::cout << "\u2524\n";              // ┤
//     };

//     auto bottom = []()
//     {
//         std::cout << "\u2514";                 // └
//         for (int i = 0; i < 55; i++) std::cout << "\u2500";
//         std::cout << "\u2518\n";              // ┘
//     };

//     auto print_row = [&](const char *stage, uint32_t inst)
//     {
//         std::stringstream ss;
//         ss << "0x" << std::hex << std::setw(8) << std::setfill('0') << inst << std::dec;
//         std::cout << "\u2502 " << std::left << std::setw(6) << stage
//                   << " \u2502 " << std::setw(12) << ss.str()
//                   << " \u2502 " << std::setw(10) << inst_to_mnemonic(inst)
//                   << " \u2502\n";
//     };

//     line();
//     std::cout << "\u2502 Stage   \u2502 Instruction  \u2502 Mnemonic   \u2502\n";
//     separator();
//     print_row("IF/ID", if_id_reg_.instruction);
//     print_row("ID/EX", id_ex_reg_.instruction);
//     print_row("EX/MEM", ex_mem_reg_.instruction);
//     bottom();
// }


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


/* void RV5StageVM_Base::print_pipeline_registers_debug()
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
} */

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