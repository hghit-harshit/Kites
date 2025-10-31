/**
 * @file rv5s_hazard_unit.h
 * @brief Logic for detecting data and control hazards in the 5-stage pipeline.
 * @author Atharva and Harshit
 */
#ifndef RV5S_HAZARD_UNIT_H
#define RV5S_HAZARD_UNIT_H

#include "vm/pipeline_registers.h"
#include <cstdint>

class RV5SHazardUnit {
public:
    // In Mode 3 (No Forwarding), this checks for ALL dependencies (ALU-ALU, Load-Use).
    bool check_for_data_stall_no_forwarding(
        const IF_ID_Register& if_id, 
        const ID_EX_Register& id_ex,
        const EX_MEM_Register& ex_mem);

    // In Mode 4 (With Forwarding), this checks ONLY for the Load-Use hazard,
    // which is the only data hazard forwarding cannot resolve.
    bool check_for_load_use_stall(
        const IF_ID_Register& if_id, 
        const ID_EX_Register& id_ex,
        const EX_MEM_Register& ex_mem); // <-- ADDED FOR MODE 4

    // This checks specifically for control hazards (conditional branches)
    bool check_for_control_stall(const IF_ID_Register& if_id);
    
private:
    // Helper to extract register index from IF/ID
    uint8_t get_rs1(uint32_t instruction) const { return (instruction >> 15) & 0x1F; }
    uint8_t get_rs2(uint32_t instruction) const { return (instruction >> 20) & 0x1F; }
    uint8_t get_opcode(uint32_t instruction) const { return instruction & 0b1111111; }
};

#endif // RV5S_HAZARD_UNIT_H
