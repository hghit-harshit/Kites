/**
 * @file rv5s_hazard_unit.cpp
 * @brief Implementation for the 5-stage pipeline Hazard Detection Unit.
 * @author Atharva and Harshit
 */
#include "vm/rv5s_vms/rv5s_hazard_unit.h"

// --- Hazard Detection Logic for Mode 3 (Stall All) ---
bool RV5SHazardUnit::check_for_data_stall_no_forwarding(
    const IF_ID_Register& if_id, 
    const ID_EX_Register& id_ex,
    const EX_MEM_Register& ex_mem)
{
    uint32_t instruction_id = if_id.instruction;
    uint8_t rs1_id = get_rs1(instruction_id);
    uint8_t rs2_id = get_rs2(instruction_id);

    // Hazard 1: ID depends on EX (Instruction in ID/EX register)
    // The result from EX is not written back until the WB stage (3 cycles away).
    // The ID instruction needs 2 stall cycles.
    if (id_ex.reg_write && id_ex.rd != 0) {
        if (rs1_id == id_ex.rd || rs2_id == id_ex.rd) {
            return true;
        }
    }

    // Hazard 2: ID depends on MEM (Instruction in EX/MEM register)
    // The result from MEM is not written back until the WB stage (2 cycles away).
    // The ID instruction needs 1 stall cycle.
    if (ex_mem.reg_write && ex_mem.rd != 0) {
        if (rs1_id == ex_mem.rd || rs2_id == ex_mem.rd) {
             return true;
        }
    }

    return false;
}

// --- Control Hazard Detection ---
bool RV5SHazardUnit::check_for_control_stall(const IF_ID_Register& if_id)
{
    uint32_t instruction_id = if_id.instruction;
    uint8_t opcode_id = get_opcode(instruction_id);
    
    // Check for conditional branches (B-type)
    // We stall for a few cycles to resolve the branch outcome.
    if (opcode_id == 0b1100011) { 
        return true;
    }
    
    // Jumps (JAL/JALR) are generally resolved with a single flush/bubble, 
    // but a full stall is safer in a No-Forwarding/No-Prediction mode. 
    // However, since we handle JAL/JALR with a quick flush in MEM, we exclude them 
    // from the multi-cycle stall here.

    return false;
}
