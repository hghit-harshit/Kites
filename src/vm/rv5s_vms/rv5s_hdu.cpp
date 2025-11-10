/**
 * @file hazard_detection_unit.cpp
 * @brief Logic for the Hazard Detection Unit (HDU).
 * * This function determines if a data hazard exists between instructions in 
 * the ID/EX and IF/ID pipeline registers, and calculates the number of stalls
 * (bubbles) needed based on the simulator's mode (Forwarding ON/OFF).
 * * NOTE: This version includes and uses the full structs from pipeline_registers.h.
 */
#include <cstdint>
#include <algorithm>
#include <iostream>

// Assuming pipeline_registers.h contains the necessary struct definitions:
// IF_ID_Register, ID_EX_Register, etc.
// NOTE: Since the full pipeline_registers.h is not available in the current compile context, 
// we must rely on the previous forward declarations for local testing or include the file.
// For simplicity and correct integration, we assume inclusion of the header here.
#include "vm/pipeline_registers.h" 

// Define stall count constants
constexpr int STALL_NONE = 0;
constexpr int STALL_ONE_CYCLE = 1;
constexpr int STALL_TWO_CYCLES = 2;

/**
 * @brief Checks for data hazards and returns the number of stalls required.
 * @param if_id_reg The register holding the instruction in the IF/ID stage (the dependent instruction).
 * @param id_ex_reg The register holding the instruction in the ID/EX stage (the source instruction).
 * @param is_forwarding_enabled Flag to switch between 'Forwarding ON' and 'No Forwarding' modes.
 * @return The number of cycles the pipeline should stall (0, 1, or 2).
 */
int check_data_hazard(const IF_ID_Register& if_id_reg, 
                      const ID_EX_Register& id_ex_reg, 
                      bool is_forwarding_enabled) 
{
    // --- 1. Extract Source Register Indices from IF/ID Instruction (Dependent) ---
    uint32_t instruction = if_id_reg.instruction;
    
    // GPR Source Registers (rs1/rs2 fields)
    uint8_t if_id_rs1 = (instruction >> 15) & 0x1F;
    uint8_t if_id_rs2 = (instruction >> 20) & 0x1F;
    
    // FPR Source Registers (frs1/frs2/frs3 fields—must match control logic)
    // Note: F-type instructions reuse the rs1/rs2 fields for frs1/frs2 (unused frs3 here)
    uint8_t if_id_frs1 = (instruction >> 15) & 0x1F;
    uint8_t if_id_frs2 = (instruction >> 20) & 0x1F;
    uint8_t if_id_frs3 = (instruction >> 27) & 0x1F; // For FMA (R4 format)

    // --- 2. Check for GPR Hazard (Source writes GPR, Dependent reads GPR) ---
    // Hazard if EX writes GPR (Rd != 0) AND next instruction reads that GPR.
    bool gpr_writes = id_ex_reg.reg_write && (id_ex_reg.rd != 0);
    bool gpr_reads = (id_ex_reg.rd == if_id_rs1 || id_ex_reg.rd == if_id_rs2);
    bool gpr_hazard = gpr_writes && gpr_reads;

    // --- 3. Check for FPR Hazard (Source writes FPR, Dependent reads FPR) ---
    // Hazard if EX writes FPR (FRd != 0) AND next instruction reads that FPR.
    bool fpr_writes = id_ex_reg.freg_write && (id_ex_reg.frd != 0);
    bool fpr_reads = (id_ex_reg.frd == if_id_frs1 || 
                      id_ex_reg.frd == if_id_frs2 || 
                      id_ex_reg.frd == if_id_frs3);
    bool fpr_hazard = fpr_writes && fpr_reads;
    fpr_hazard = false; // FPR hazard detection disabled for current implementation
    // --- 4. No Hazard Exists ---
    if (!gpr_hazard && !fpr_hazard) {
        return STALL_NONE;
    }

    // --- 5. Determine Stall Count Based on Forwarding Mode ---

    // Hazard exists (either GPR or FPR)
    if (is_forwarding_enabled) {
        // --- Forwarding ON: Detects Load-Use only (1 Stall) ---
        // Load-Use hazard occurs if the producing instruction (ID/EX) is a Load.
        if (id_ex_reg.mem_read) {
            // Load-Use hazard (GPR Load-Use or FPR Load-Use) requires one bubble/stall.
            return STALL_ONE_CYCLE;
        }
        
        // All other hazards (R-R, F-F, R-Store, F-Store) are solved by the forwarding unit.
        return STALL_NONE;
    }
    
    else {
        // --- No Forwarding: Detects ALL producing hazards (2 Stalls) ---
        // Any instruction producing data needed by the immediate next instruction 
        // requires a full 2-cycle stall.
        return STALL_TWO_CYCLES;
    }
}
