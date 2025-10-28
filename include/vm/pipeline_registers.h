/**
 * @file pipeline_registers.h
 * @brief 
 *This file defines the "latches" or registers that sit between each of the
 *five pipeline stages. They hold all the data and control signals needed
 *by the subsequent stages.
 * @version 0.1
 * @date 2025-10-29
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#pragma once

#include <cstdint>

// --- Data passed from Fetch (IF) to Decode (ID) ---
struct IF_ID_Register
{
    uint32_t instruction = 0x00000013; // A NOP instruction (addi x0, x0, 0)
    uint64_t pc = 0;

    void reset()
    {
        // Resetting injects a NOP, used for flushing the pipeline.
        instruction = 0x00000013;
        pc = 0;
    }
};

// --- Data passed from Decode (ID) to Execute (EX) ---
struct ID_EX_Register
{
    // Data values
    uint64_t pc = 0;
    uint32_t instruction = 0x00000013; // Pass full instruction for decoding in EX
    uint64_t reg1_data = 0;
    uint64_t reg2_data = 0;
    int32_t imm = 0;

    // Register indices (needed for forwarding and hazard detection)
    uint8_t rs1 = 0;
    uint8_t rs2 = 0;
    uint8_t rd = 0;

    // Control signals generated in Decode stage
    bool reg_write = false;
    bool mem_to_reg = false;
    bool mem_read = false;
    bool mem_write = false;
    bool branch = false;
    bool alu_src = false;
    uint8_t alu_op = 0; // Hint for the ALU Control Unit

    void reset()
    {
        // Resetting injects a "bubble" with all control signals off.
        pc = reg1_data = reg2_data = imm = rs1 = rs2 = rd = 0;
        instruction = 0x00000013;
        reg_write = mem_to_reg = mem_read = mem_write = branch = alu_src = false;
        alu_op = 0;
    }
};

// --- Data passed from Execute (EX) to Memory (MEM) ---
struct EX_MEM_Register
{
    // Data values
    uint64_t alu_result = 0;
    uint64_t reg2_data = 0; // Data from rs2, needed for Store instructions
    uint8_t rd = 0;         // Destination register index

    // Branching info calculated in EX stage
    bool branch_taken = false;
    uint64_t branch_target_pc = 0;

    // Control signals passed through from the previous stage
    bool reg_write = false;
    bool mem_to_reg = false;
    bool mem_read = false;
    bool mem_write = false;

    void reset()
    {
        alu_result = reg2_data = rd = 0;
        branch_taken = false;
        branch_target_pc = 0;
        reg_write = mem_to_reg = mem_read = mem_write = false;
    }
};

// --- Data passed from Memory (MEM) to Writeback (WB) ---
struct MEM_WB_Register
{
    // Data values
    uint64_t memory_data = 0; // Data read from memory in a Load
    uint64_t alu_result = 0;  // Result from the ALU
    uint8_t rd = 0;           // Destination register index

    // Control signals passed through
    bool reg_write = false;
    bool mem_to_reg = false;

    void reset()
    {
        memory_data = alu_result = rd = 0;
        reg_write = mem_to_reg = false;
    }
};
