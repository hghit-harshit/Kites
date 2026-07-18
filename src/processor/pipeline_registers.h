/**
 * @file pipeline_registers.h
 * @brief Pipeline registers supporting RV64I/M/F/D extensions.
 * @version 0.3
 * @date 2025-11-03
 * @copyright Copyright (c) 2025
 * */

#pragma once

#include <cstdint>
#include "processor/processor_constants.h"
namespace Kites
{
// --- Data passed from Fetch (IF) to Decode (ID) ---
struct IF_ID_Register
{
    uint32_t instruction {NOP};  // A NOP instruction (addi x0, x0, 0)
    uint64_t pc {INVALID_PC};

    void reset()
    {
        // Resetting injects a NOP, used for flushing the pipeline.
        instruction = NOP;
        pc          = INVALID_PC;
    }

    void insertNop()
    {
        // Inserts a NOP instruction into the pipeline stage.
        instruction = NOP;
    }
};

  // --- Data passed from Decode (ID) to Execute (EX) ---
struct ID_EX_Register
{
      // --- GPR Data ---
    uint64_t pc {INVALID_PC};
    uint32_t instruction {NOP};  // Pass full instruction for decoding in EX
    uint64_t reg1_data   {0};           // GPR rs1 data
    uint64_t reg2_data   {0};           // GPR rs2 data
    int32_t  imm         {0};

    // --- FPR Data (for F-type instructions) --- (ignore these not used)
    uint64_t freg1_data {0};  // FPR frs1 data
    uint64_t freg2_data {0};  // FPR frs2 data
    uint64_t freg3_data {0};  // FPR frs3 data (for FMA instructions)

    // --- Register Indices (for GPR and FPR) ---
    uint8_t rs1 {0};  // GPR source 1 index
    uint8_t rs2 {0};  // GPR source 2 index
    uint8_t rd  {0};  // GPR destination index

    // ignore these not used
    uint8_t frs1 {0};  // FPR source 1 index
    uint8_t frs2 {0};  // FPR source 2 index
    uint8_t frs3 {0};  // FPR source 3 index
    uint8_t frd  {0};  // FPR destination index

    // --- Control signals generated in Decode stage ---
    bool reg_write  {false};  // Write to GPRs (x0-x31)
    bool freg_write {false};  // Write to FPRs (f0-f31)(ignore these)

    bool mem_to_reg {false};
    bool mem_read   {false};
    bool mem_write  {false};
    bool branch     {false};
    bool alu_src    {false};

    uint8_t alu_op {0};      // Hint for the ALU Control Unit

    void reset()
    {
          // Resetting injects a "bubble"
        pc         = INVALID_PC;
        reg1_data  = reg2_data  = imm        = rs1  = rs2  = rd   = 0;
        freg1_data = freg2_data = freg3_data = frs1 = frs2 = frs3 = frd = 0;

        instruction = NOP;
        reg_write   = freg_write = mem_to_reg = mem_read = mem_write = branch = alu_src = false;
        alu_op      = 0;
    }
};

  // --- Data passed from Execute (EX) to Memory (MEM) ---
struct EX_MEM_Register
{
    uint64_t pc {INVALID_PC};           // Passing PC for highlighting purposes
    uint32_t instruction {NOP};  // Pass full instruction for reference in MEM
    // --- GPR Results ---
    uint64_t alu_result {0};  // GPR Write Data (ALU Result, Link Address, etc.)
    uint64_t reg2_data  {0};  // Data from rs2, needed for Store instructions

    uint8_t rd      {0};  // GPR Destination register index
    uint8_t prev_rd {0};  // Previous GPR Destination register index (for one cycle delay)

    // --- FPR Results ---
    uint64_t f_alu_result {0};  // FPR Write Data (F-ALU Result, Conversions)
    uint64_t freg2_data   {0};  // Data from frs2, needed for Store instructions
    uint8_t  frd          {0};  // FPR Destination register index
    uint8_t  prev_frd     {0};  // Previous FPR Destination register index (for one cycle delay)
    // Branching info calculated in EX stage
    bool     branch_taken      {false};
    bool     prev_branch_taken {false};  // we'll use this for hightlighting purposes
    uint64_t branch_target_pc  {0};

    // Control signals passed through from the previous stage
    bool reg_write       {false};  // GPR Write enable
    bool prev_reg_write  {false};  // Previous GPR Write enable (used for checking one cycle delay)
    bool freg_write      {false};  // FPR Write enable
    bool prev_freg_write {false};  // Previous FPR Write enable (used for checking one cycle delay)
    bool mem_to_reg      {false};
    bool mem_read        {false};  // for highlighting purposes
    bool mem_write       {false};

    bool prev_mem_read  {false};  // Previous mem_read signal (for highlighting purposes)
    bool prev_mem_write {false};  // Previous mem_write signal (for highlighting purposes)

    void reset()
    {
        pc          = INVALID_PC;
        instruction = NOP;

        alu_result = 0;
        reg2_data  = 0;
        rd         = 0;
        prev_rd    = 0;

        f_alu_result = 0;
        freg2_data   = 0;
        frd          = 0;
        prev_frd     = 0;

        branch_taken      = false;
        prev_branch_taken = false;
        branch_target_pc  = 0;

        reg_write       = false;
        prev_reg_write  = false;
        freg_write      = false;
        prev_freg_write = false;
        mem_to_reg      = false;
        mem_read        = false;
        mem_write       = false;
        prev_mem_read   = false;
        prev_mem_write  = false;
    }
};

  // --- Data passed from Memory (MEM) to Writeback (WB) ---
struct MEM_WB_Register
{
    uint64_t pc {INVALID_PC};           // Passing PC for highlighting purposes
    uint32_t instruction {NOP};  // Pass full instruction for reference in WB
    // --- GPR Results ---
    uint64_t memory_data {0};           // GPR Write Data (Data read from memory in a Load)
    uint64_t alu_result  {0};           // GPR Write Data (ALU result, Link Address, etc.)
    uint8_t  rd          {0};           // GPR Destination register index

    uint64_t prev_memory_data {0};  // Previous Memory data (for forwarding)
    uint8_t  prev_rd          {0};  // Previous GPR Destination register index
    uint64_t prev_alu_result  {0};  // Previous ALU result
    // --- FPR Results ---
    uint64_t f_memory_data {0};  // FPR Write Data (Data read from memory in FLW/FLD)
    uint64_t f_alu_result  {0};  // FPR Write Data (F-ALU result)
    uint8_t  frd           {0};  // FPR Destination register index

    uint64_t prev_f_memory_data {0};  // Previous FPR Memory data (for forwarding)
    uint64_t prev_f_alu_result {0};   // Previous FPR ALU
    uint8_t  prev_frd {0};            // Previous FPR Destination register index
    // Control signals passed through
    bool reg_write  {false};  // GPR Write enable
    bool freg_write {false};  // FPR Write enable
    bool mem_to_reg {false};

    bool prev_mem_to_reg {false};  // Previous mem_to_reg signal (for forwarding)
    bool prev_reg_write  {false};  // Previous reg_write signal (for forwarding)
    bool prev_freg_write {false};  // Previous FPR write enable (for forwarding)
    void reset()
    {
        pc          = INVALID_PC;
        instruction = NOP;

        memory_data = 0;
        alu_result  = 0;
        rd          = 0;

        prev_memory_data = 0;
        prev_rd          = 0;
        prev_alu_result  = 0;

        f_memory_data = 0;
        f_alu_result  = 0;
        frd           = 0;

        prev_f_memory_data = 0;
        prev_f_alu_result  = 0;
        prev_frd           = 0;

        reg_write       = false;
        freg_write      = false;
        mem_to_reg      = false;
        prev_mem_to_reg = false;
        prev_reg_write  = false;
        prev_freg_write = false;
    }
};
}// namespace Kites