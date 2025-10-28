/**
 * @file rv5s_control_unit.cpp
 * @brief Implementation for the 5-stage pipelined Control Unit.
 * @author Atharva and Harshit
 */
#include "vm/rv5s/rv5s_control_unit.h"
#include "common/instructions.h" // For instruction encoding maps if needed
#include "vm/alu.h"

#include <cstdint>

void RV5SControlUnit::SetControlSignals(uint32_t instruction) 
{
    // This is the "Main Control Unit" logic. It runs in the Decode stage.
    // It only looks at the opcode to set the main control signals.
    Reset();
    uint8_t opcode = instruction & 0b1111111;

    switch (opcode) {
        // Base Integer Instructions
        case 0b0110011: // R-type (ADD, SUB, AND, etc.)
            reg_write_ = true;
            alu_op_ = 2; // ALUOp code '2' tells ALU Control to decode R-type.
            break;
        case 0b0010011: // I-type ALU (ADDI, ANDI, etc.)
            alu_src_ = true;
            reg_write_ = true;
            alu_op_ = 1; // ALUOp code '1' for I-type/Branch.
            break;
        case 0b0000011: // I-type Load (LW, LB, etc.)
            alu_src_ = true;
            mem_to_reg_ = true;
            reg_write_ = true;
            mem_read_ = true;
            alu_op_ = 0; // ALUOp code '0' tells ALU to perform ADD for address calculation.
            break;
        case 0b0100011: // S-type Store (SW, SB, etc.)
            alu_src_ = true;
            mem_write_ = true;
            alu_op_ = 0; // ALU must also perform ADD for address calculation.
            break;
        case 0b1100011: // B-type Branch (BEQ, BNE, etc.)
            branch_ = true;
            alu_op_ = 1; // ALUOp code '1' for I-type/Branch.
            break;
        case 0b0110111: // U-type LUI
            alu_src_ = true;
            reg_write_ = true;
            alu_op_ = 0; // ALU can just pass through the immediate (e.g., by adding to zero).
            break;
        case 0b0010111: // U-type AUIPC
            reg_write_ = true;
            alu_op_ = 0; // ALU adds PC to the immediate.
            break;
        case 0b1101111: // J-type JAL
            reg_write_ = true;
            break;
        case 0b1100111: // I-type JALR
            alu_src_ = true;
            reg_write_ = true;
            alu_op_ = 0; // ALU calculates target address (reg + imm).
            break;

        // RV32/64 W Instructions
        case 0b0011011: // ADDIW, SLLIW, etc.
             alu_src_ = true;
             reg_write_ = true;
             alu_op_ = 2; // Decode as R-type (but with immediate)
             break;
        case 0b0111011: // ADDW, SUBW, etc.
            reg_write_ = true;
            alu_op_ = 2; // Decode as R-type
            break;

        // F and D Extensions
        case 0b0000111: // F-Type Load (FLW, FLD)
            alu_src_ = true;
            mem_to_reg_ = true;
            reg_write_ = true;
            mem_read_ = true;
            alu_op_ = 0; // ALU performs ADD for address calculation
            break;
        case 0b0100111: // F-Type Store (FSW, FSD)
            alu_src_ = true;
            mem_write_ = true;
            alu_op_ = 0; // ALU performs ADD for address calculation
            break;
        case 0b1010011: // F-Type R-type (FADD, FSUB, etc.)
            reg_write_ = true;
            alu_op_ = 3; // ALUOp code '3' for FP R-type decode
            break;
        case 0b1000011: // FMADD
        case 0b1000111: // FMSUB
        case 0b1001011: // FNMADD
        case 0b1001111: // FNMSUB
            reg_write_ = true;
            alu_op_ = 4; // ALUOp code '4' for FMA-type decode
            break;
    }
}

alu::AluOp RV5SControlUnit::GetAluSignal(uint32_t instruction, bool ALUOp) {
    // This is the "ALU Control Unit" logic. It runs in the Execute stage.
    (void)ALUOp; // The boolean ALUOp from the base class is ignored in this design.

    uint8_t opcode = instruction & 0b1111111; 
    uint8_t funct3 = (instruction >> 12) & 0b111;
    uint8_t funct7 = (instruction >> 25) & 0b1111111;
    uint8_t funct5 = (instruction >> 20) & 0b11111;
    uint8_t funct2 = (instruction >> 25) & 0b11;
    
    switch (alu_op_) {
        case 0: // ALUOp=0 -> Must be ADD (for Loads, Stores, JALR, LUI, AUIPC).
            return alu::AluOp::kAdd;
        
        case 1: // ALUOp=1 -> Decode for Branch comparison or I-type ALU.
            if (opcode == 0b1100011) { // B-type Branch
                 switch(funct3) {
                    case 0b000: return alu::AluOp::kSub;  // BEQ, BNE
                    case 0b001: return alu::AluOp::kSub;
                    case 0b100: return alu::AluOp::kSlt;  // BLT
                    case 0b101: return alu::AluOp::kSlt;  // BGE
                    case 0b110: return alu::AluOp::kSltu; // BLTU
                    case 0b111: return alu::AluOp::kSltu; // BGEU
                    default: return alu::AluOp::kNone;
                 }
            } else { // I-type ALU (opcode 0b0010011)
                switch(funct3) {
                    case 0b000: return alu::AluOp::kAdd;  // ADDI
                    case 0b010: return alu::AluOp::kSlt;  // SLTI
                    case 0b011: return alu::AluOp::kSltu; // SLTIU
                    case 0b100: return alu::AluOp::kXor;  // XORI
                    case 0b110: return alu::AluOp::kOr;   // ORI
                    case 0b111: return alu::AluOp::kAnd;  // ANDI
                    case 0b001: return alu::AluOp::kSll;  // SLLI
                    case 0b101: // For SRAI/SRLI, we need to check funct7
                        if (funct7 == 0b0100000) return alu::AluOp::kSra; // SRAI
                        return alu::AluOp::kSrl; // SRLI
                    default: return alu::AluOp::kNone;
                }
            }
            break;

        case 2: // ALUOp=2 -> Full R-type and W-type decode.
            if (opcode == 0b0111011 || opcode == 0b0011011) { // 32-bit W instructions
                 switch (funct3) {
                    case 0b000:
                        if (opcode == 0b0011011) return alu::AluOp::kAddw; // ADDIW
                        if (funct7 == 0b0000001) return alu::AluOp::kMulw;
                        if (funct7 == 0b0100000) return alu::AluOp::kSubw;
                        return alu::AluOp::kAddw;
                    case 0b001: return alu::AluOp::kSllw; // SLLIW, SLLW
                    case 0b100: if (funct7 == 0b0000001) return alu::AluOp::kDivw; break;
                    case 0b101:
                        if (funct7 == 0b0000001) return alu::AluOp::kDivuw;
                        if (funct7 == 0b0100000) return alu::AluOp::kSraw;
                        return alu::AluOp::kSrlw;
                    case 0b110: if (funct7 == 0b0000001) return alu::AluOp::kRemw; break;
                    case 0b111: if (funct7 == 0b0000001) return alu::AluOp::kRemuw; break;
                 }
            }
            // Fall through to standard R-type if not a W instruction
            switch(funct3) {
                case 0b000: // ADD, SUB, MUL
                    if (funct7 == 0b0000001) return alu::AluOp::kMul;
                    if (funct7 == 0b0100000) return alu::AluOp::kSub;
                    return alu::AluOp::kAdd;
                case 0b001: // SLL, MULH
                    if (funct7 == 0b0000001) return alu::AluOp::kMulh;
                    return alu::AluOp::kSll;
                case 0b010: // SLT, MULHSU
                    if (funct7 == 0b0000001) return alu::AluOp::kMulhsu;
                    return alu::AluOp::kSlt;
                case 0b011: // SLTU, MULHU
                    if (funct7 == 0b0000001) return alu::AluOp::kMulhu;
                    return alu::AluOp::kSltu;
                case 0b100: // XOR, DIV
                    if (funct7 == 0b0000001) return alu::AluOp::kDiv;
                    return alu::AluOp::kXor;
                case 0b101: // SRL, SRA, DIVU
                    if (funct7 == 0b0000001) return alu::AluOp::kDivu;
                    if (funct7 == 0b0100000) return alu::AluOp::kSra;
                    return alu::AluOp::kSrl;
                case 0b110: // OR, REM
                    if (funct7 == 0b0000001) return alu::AluOp::kRem;
                    return alu::AluOp::kOr;
                case 0b111: // AND, REMU
                    if (funct7 == 0b0000001) return alu::AluOp::kRemu;
                    return alu::AluOp::kAnd;
            }
            break;

        case 3: // ALUOp=3 -> Floating Point R-type decode
            switch (funct7) {
                case 0b0000000: return alu::AluOp::FADD_S;
                case 0b0000001: return alu::AluOp::FADD_D;
                case 0b0000100: return alu::AluOp::FSUB_S;
                case 0b0000101: return alu::AluOp::FSUB_D;
                case 0b0001000: return alu::AluOp::FMUL_S;
                case 0b0001001: return alu::AluOp::FMUL_D;
                case 0b0001100: return alu::AluOp::FDIV_S;
                case 0b0001101: return alu::AluOp::FDIV_D;
                case 0b0101100: if(funct5 == 0) return alu::AluOp::FSQRT_S; break;
                case 0b0101101: if(funct5 == 0) return alu::AluOp::FSQRT_D; break;
                case 0b0010000: // FSGNJ.S, FSGNJN.S, FSGNJX.S
                    if (funct3 == 0b000) return alu::AluOp::FSGNJ_S;
                    if (funct3 == 0b001) return alu::AluOp::FSGNJN_S;
                    if (funct3 == 0b010) return alu::AluOp::FSGNJX_S;
                    break;
                case 0b0010001: // FSGNJ.D, FSGNJN.D, FSGNJX.D
                    if (funct3 == 0b000) return alu::AluOp::FSGNJ_D;
                    if (funct3 == 0b001) return alu::AluOp::FSGNJN_D;
                    if (funct3 == 0b010) return alu::AluOp::FSGNJX_D;
                    break;
                case 0b0010100: // FMIN.S, FMAX.S
                    if (funct3 == 0b000) return alu::AluOp::FMIN_S;
                    if (funct3 == 0b001) return alu::AluOp::FMAX_S;
                    break;
                case 0b0010101: // FMIN.D, FMAX.D
                    if (funct3 == 0b000) return alu::AluOp::FMIN_D;
                    if (funct3 == 0b001) return alu::AluOp::FMAX_D;
                    break;
                case 0b1010000: // FEQ.S, FLT.S, FLE.S
                    if (funct3 == 0b010) return alu::AluOp::FEQ_S;
                    if (funct3 == 0b001) return alu::AluOp::FLT_S;
                    if (funct3 == 0b000) return alu::AluOp::FLE_S;
                    break;
                case 0b1010001: // FEQ.D, FLT.D, FLE.D
                    if (funct3 == 0b010) return alu::AluOp::FEQ_D;
                    if (funct3 == 0b001) return alu::AluOp::FLT_D;
                    if (funct3 == 0b000) return alu::AluOp::FLE_D;
                    break;
                case 0b1100000: // FCVT.W.S, FCVT.WU.S, ...
                    if (funct5 == 0b00000) return alu::AluOp::FCVT_W_S;
                    if (funct5 == 0b00001) return alu::AluOp::FCVT_WU_S;
                    if (funct5 == 0b00010) return alu::AluOp::FCVT_L_S;
                    if (funct5 == 0b00011) return alu::AluOp::FCVT_LU_S;
                    break;
                case 0b1100001: // FCVT.W.D, FCVT.WU.D, ...
                    if (funct5 == 0b00000) return alu::AluOp::FCVT_W_D;
                    if (funct5 == 0b00001) return alu::AluOp::FCVT_WU_D;
                    if (funct5 == 0b00010) return alu::AluOp::FCVT_L_D;
                    if (funct5 == 0b00011) return alu::AluOp::FCVT_LU_D;
                    break;
                case 0b1101000: // FCVT.S.W, FCVT.S.WU, ...
                    if (funct5 == 0b00000) return alu::AluOp::FCVT_S_W;
                    if (funct5 == 0b00001) return alu::AluOp::FCVT_S_WU;
                    if (funct5 == 0b00010) return alu::AluOp::FCVT_S_L;
                    if (funct5 == 0b00011) return alu::AluOp::FCVT_S_LU;
                    break;
                case 0b1101001: // FCVT.D.W, FCVT.D.WU, ...
                    if (funct5 == 0b00000) return alu::AluOp::FCVT_D_W;
                    if (funct5 == 0b00001) return alu::AluOp::FCVT_D_WU;
                    if (funct5 == 0b00010) return alu::AluOp::FCVT_D_L;
                    if (funct5 == 0b00011) return alu::AluOp::FCVT_D_LU;
                    break;
                 case 0b1110000: // FCLASS.S, FMV.X.W
                    if (funct3 == 0b001) return alu::AluOp::FCLASS_S;
                    if (funct3 == 0b000) return alu::AluOp::FMV_X_W;
                    break;
                case 0b1110001: // FCLASS.D, FMV.X.D
                    if (funct3 == 0b001) return alu::AluOp::FCLASS_D;
                    if (funct3 == 0b000) return alu::AluOp::FMV_X_D;
                    break;
                case 0b1111000: return alu::AluOp::FMV_W_X;
                case 0b1111001: return alu::AluOp::FMV_D_X;
            }
            break;

        case 4: // ALUOp=4 -> FMA-type decode
            switch(opcode) {
                case 0b1000011: return (funct2 == 0b00) ? alu::AluOp::kFmadd_s : alu::AluOp::FMADD_D;
                case 0b1000111: return (funct2 == 0b00) ? alu::AluOp::kFmsub_s : alu::AluOp::FMSUB_D;
                case 0b1001011: return (funct2 == 0b00) ? alu::AluOp::kFnmadd_s : alu::AluOp::FNMADD_D;
                case 0b1001111: return (funct2 == 0b00) ? alu::AluOp::kFnmsub_s : alu::AluOp::FNMSUB_D;
            }
            break;
    }
    
    return alu::AluOp::kNone; // Safety default
}

