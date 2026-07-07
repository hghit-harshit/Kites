/**
 * @file rvss_control_unit.cpp
 * @brief RVSS Control Unit implementation
 * @author Vishank Singh, https://github.com/VishankSingh
 */

#include "processor/rvss/rvss_control_unit.h"
#include "processor/alu.h"

#include <cstdint>

namespace Kites
{
void RVSSControlUnit::SetControlSignals(uint32_t instruction)
{
    uint8_t opcode = instruction & 0b1111111;

    alu_src_ = mem_to_reg_ = reg_write_ = mem_read_ = mem_write_ = branch_ = false;
    alu_op_ = false;

    switch (opcode)
    {
    case 0b0110011:
    { // R-type (kAdd, kSub, kAnd, kOr, kXor, kSll, kSrl, etc.)
        reg_write_ = true;
        alu_op_ = true;
        break;
    }
    case 0b0000011:
    { // Load instructions (LB, LH, LW, LD)
        alu_src_ = true;
        mem_to_reg_ = true;
        reg_write_ = true;
        mem_read_ = true;
        break;
    }
    case 0b0100011:
    { // Store instructions (SB, SH, SW, SD)
        alu_src_ = true;
        alu_op_ = true;
        mem_write_ = true;
        break;
    }
    case 0b1100011:
    { // branch_ instructions (BEQ, BNE, BLT, BGE)
        alu_op_ = true;
        branch_ = true;
        break;
    }
    case 0b0010011:
    { // I-type alu instructions (ADDI, ANDI, ORI, XORI, SLTI, SLLI, SRLI)
        alu_src_ = true;
        reg_write_ = true;
        alu_op_ = true;
        break;
    }
    case 0b0110111:
    { // LUI (Load Upper Immediate)
        alu_src_ = true;
        reg_write_ = true;
        alu_op_ = true; // alu will add immediate to zero
        break;
    }
    case 0b0010111:
    { // AUIPC (Add Upper Immediate to PC)
        alu_src_ = true;
        reg_write_ = true;
        alu_op_ = true; // alu will add immediate to PC
        break;
    }
    case 0b1101111:
    { // JAL (Jump and Link)
        reg_write_ = true;
        branch_ = true;
        break;
    }
    case 0b1100111:
    { // JALR (Jump and Link Register)
        alu_src_ = true;
        reg_write_ = true;
        branch_ = true;
        break;
    }
    case 0b0000001:
    { // kMul
        reg_write_ = true;
        alu_op_ = true;
        break;
    }

    // F extension + D extension
    case 0b0000111:
    { // F-Type Load instructions (FLW, FLD)
        alu_src_ = true;
        mem_to_reg_ = true;
        reg_write_ = true;
        mem_read_ = true;
        break;
    }
    case 0b0100111:
    { // F-Type Store instructions (FSW, FSD)
        alu_src_ = true;
        alu_op_ = true;
        mem_write_ = true;
        break;
    }
    case 0b1010011:
    { // F-Type R-type instructions (FADD, FSUB, FMUL, FDIV, etc.)
        reg_write_ = true;
        alu_op_ = true;
        break;
    }

    default:
        break;
    }
}

alu::AluOp RVSSControlUnit::GetAluSignal(uint32_t instruction, bool ALUOp)
{
    (void)ALUOp; // Suppress unused variable warning
    // DONT UNCOMMENT THIS WITHOUT SUPPORTING ALUOP IN CONTROL SIGNAL SETTING
    // if (!AluOp) {
    //     return alu::AluOp::kNone;
    // }
    uint8_t opcode = instruction & 0b1111111;
    uint8_t funct3 = (instruction >> 12) & 0b111;
    uint8_t funct7 = (instruction >> 25) & 0b1111111;
    uint8_t funct5 = (instruction >> 20) & 0b11111;
    uint8_t funct2 = (instruction >> 25) & 0b11;
    // uint8_t funct6 = (instruction >> 26) & 0b111111;

    switch (opcode)
    {
    case 0b0110011:
    { // R-Type
        switch (funct3)
        {
        case 0b000:
        { // kAdd, kSub, kMul
            switch (funct7)
            {
            case 0x0000000:
            { // kAdd
                return alu::AluOp::ADD;
                break;
            }
            case 0b0100000:
            { // kSub
                return alu::AluOp::SUB;
                break;
            }
            case 0b0000001:
            { // kMul
                return alu::AluOp::MUL;
                break;
            }
            }
            break;
        }
        case 0b001:
        { // kSll, kMulh
            switch (funct7)
            {
            case 0b0000000:
            { // kSll
                return alu::AluOp::SLL;
                break;
            }
            case 0b0000001:
            { // kMulh
                return alu::AluOp::MULH;
                break;
            }
            }
            break;
        }
        case 0b010:
        { // kSlt, kMulhsu
            switch (funct7)
            {
            case 0b0000000:
            { // kSlt
                return alu::AluOp::SLT;
                break;
            }
            case 0b0000001:
            { // kMulhsu
                return alu::AluOp::MULHSU;
                break;
            }
            }
            break;
        }
        case 0b011:
        { // kSltu, kMulhu
            switch (funct7)
            {
            case 0b0000000:
            { // kSltu
                return alu::AluOp::SLTU;
                break;
            }
            case 0b0000001:
            { // kMulhu
                return alu::AluOp::MULHU;
                break;
            }
            }
            break;
        }
        case 0b100:
        { // kXor, kDiv
            switch (funct7)
            {
            case 0b0000000:
            { // kXor
                return alu::AluOp::XOR;
                break;
            }
            case 0b0000001:
            { // kDiv
                return alu::AluOp::DIV;
                break;
            }
            }
            break;
        }
        case 0b101:
        { // kSrl, kSra, kDivu
            switch (funct7)
            {
            case 0b0000000:
            { // kSrl
                return alu::AluOp::SRL;
                break;
            }
            case 0b0100000:
            { // kSra
                return alu::AluOp::SRA;
                break;
            }
            case 0b0000001:
            { // kDivu
                return alu::AluOp::DIVU;
                break;
            }
            }
            break;
        }
        case 0b110:
        { // kOr, kRem
            switch (funct7)
            {
            case 0b0000000:
            { // kOr
                return alu::AluOp::OR;
                break;
            }
            case 0b0000001:
            { // kRem
                return alu::AluOp::REM;
                break;
            }
            }
            break;
        }
        case 0b111:
        { // kAnd, kRemu
            switch (funct7)
            {
            case 0b0000000:
            { // kAnd
                return alu::AluOp::AND;
                break;
            }
            case 0b0000001:
            { // kRemu
                return alu::AluOp::REMU;
                break;
            }
            }
            break;
        }
        }
        break;
    }
    case 0b0010011:
    { // I-Type
        switch (funct3)
        {
        case 0b000:
        { // ADDI
            return alu::AluOp::ADD;
            break;
        }
        case 0b001:
        { // SLLI
            return alu::AluOp::SLL;
            break;
        }
        case 0b010:
        { // SLTI
            return alu::AluOp::SLT;
            break;
        }
        case 0b011:
        { // SLTIU
            return alu::AluOp::SLTU;
            break;
        }
        case 0b100:
        { // XORI
            return alu::AluOp::XOR;
            break;
        }
        case 0b101:
        { // SRLI & SRAI
            switch (funct7)
            {
            case 0b0000000:
            { // SRLI
                return alu::AluOp::SRL;
                break;
            }
            case 0b0100000:
            { // SRAI
                return alu::AluOp::SRA;
                break;
            }
            }
            break;
        }
        case 0b110:
        { // ORI
            return alu::AluOp::OR;
            break;
        }
        case 0b111:
        { // ANDI
            return alu::AluOp::AND;
            break;
        }
        }
        break;
    }
    case 0b1100011:
    { // B-Type
        switch (funct3)
        {
        case 0b000:
        { // BEQ
            return alu::AluOp::SUB;
            break;
        }
        case 0b001:
        { // BNE
            return alu::AluOp::SUB;
            break;
        }
        case 0b100:
        { // BLT
            return alu::AluOp::SLT;
            break;
        }
        case 0b101:
        { // BGE
            return alu::AluOp::SLT;
            break;
        }
        case 0b110:
        { // BLTU
            return alu::AluOp::SLTU;
            break;
        }
        case 0b111:
        { // BGEU
            return alu::AluOp::SLTU;
            break;
        }
        }
        break;
    }
    case 0b0000011:
    { // Load
        return alu::AluOp::ADD;
        break;
    }
    case 0b0100011:
    { // Store
        return alu::AluOp::ADD;
        break;
    }
    case 0b1100111:
    { // JALR
        return alu::AluOp::ADD;
        break;
    }
    case 0b1101111:
    { // JAL
        return alu::AluOp::ADD;
        break;
    }
    case 0b0110111:
    { // LUI
        return alu::AluOp::ADD;
        break;
    }
    case 0b0010111:
    { // AUIPC
        return alu::AluOp::ADD;
        break;
    }
    case 0b0000000:
    { // FENCE
        return alu::AluOp::NONE;
        break;
    }
    case 0b1110011:
    { // SYSTEM
        switch (funct3)
        {
        case 0b000: // ECALL
            return alu::AluOp::NONE;
            break;
        case 0b001: // CSRRW
            return alu::AluOp::NONE;
            break;
        default:
            break;
        }
        break;
    }
    case 0b0011011:
    { // R4-Type
        switch (funct3)
        {
        case 0b000:
        { // ADDIW
            return alu::AluOp::ADDW;
            break;
        }
        case 0b001:
        { // SLLIW
            return alu::AluOp::SLLW;
            break;
        }
        case 0b101:
        { // SRLIW & SRAIW
            switch (funct7)
            {
            case 0b0000000:
            { // SRLIW
                return alu::AluOp::SRLW;
                break;
            }
            case 0b0100000:
            { // SRAIW
                return alu::AluOp::SRAW;
                break;
            }
            }
            break;
        }
        }
        break;
    }
    case 0b0111011:
    { // R4-Type
        switch (funct3)
        {
        case 0b000:
        { // ADDW, SUBW, MULW
            switch (funct7)
            {
            case 0b0000000:
            { // ADDW
                return alu::AluOp::ADDW;
                break;
            }
            case 0b0100000:
            { // SUBW
                return alu::AluOp::SUBW;
                break;
            }
            case 0b0000001:
            { // MULW
                return alu::AluOp::MULW;
                break;
            }
            }
            break;
        }
        case 0b001:
        { // SLLW
            return alu::AluOp::SLLW;
            break;
        }
        case 0b100:
        { // DIVW
            switch (funct7)
            { // DIVW
            case 0b0000001:
            { // DIVW
                return alu::AluOp::DIVW;
                break;
            }
            }
            break;
        }
        case 0b101:
        { // SRLW, SRAW, DIVUW
            switch (funct7)
            {
            case 0b0000000:
            { // SRLW
                return alu::AluOp::SRLW;
                break;
            }
            case 0b0100000:
            { // SRAW
                return alu::AluOp::SRAW;
                break;
            }
            case 0b0000001:
            { // DIVUW
                return alu::AluOp::DIVUW;
                break;
            }
            }
            break;
        }
        case 0b110:
        { // REMW
            switch (funct7)
            {
            case 0b0000001:
            { // REMW
                return alu::AluOp::REMW;
                break;
            }
            }
            break;
        }
        case 0b111:
        { // REMUW
            switch (funct7)
            {
            case 0b0000001:
            { // REMUW
                return alu::AluOp::REMUW;
                break;
            }
            }
            break;
        }
        }
        break;
    }

        // F extension + D extension
        // TODO: correct this

    case 0b1000011:
    {
        return alu::AluOp::FMADD_S;
    }

    case 0b1010011:
    {
        switch (funct7)
        {
        case 0b0000000:
        { // FADD_S
            return alu::AluOp::FADD_S;
        }
        case 0b0000001:
        { // FADD_D
            return alu::AluOp::FADD_D;
        }
        case 0b0000100:
        { // FSUB_S
            return alu::AluOp::FSUB_S;
        }
        case 0b0000101:
        { // FSUB_D
            return alu::AluOp::FSUB_D;
        }
        case 0b0001000:
        { // FMUL_S
            return alu::AluOp::FMUL_S;
        }
        case 0b0001001:
        { // FMUL_D
            return alu::AluOp::FMUL_D;
        }
        case 0b0001100:
        { // FDIV_S
            return alu::AluOp::FDIV_S;
        }
        case 0b0001101:
        { // FDIV_D
            return alu::AluOp::FDIV_D;
        }
        case 0b0101100:
        { // FSQRT_S
            return alu::AluOp::FSQRT_S;
        }
        case 0b0101101:
        { // FSQRT_D
            return alu::AluOp::FSQRT_D;
        }
        case 0b1100000:
        { // FCVT.(W|WU|L|LU).S
            switch (funct5)
            {
            case 0b00000:
            { // FCVT_W_S
                return alu::AluOp::FCVT_W_S;
            }
            case 0b00001:
            { // FCVT_WU_S
                return alu::AluOp::FCVT_WU_S;
            }
            case 0b00010:
            { // FCVT_L_S
                return alu::AluOp::FCVT_L_S;
            }
            case 0b00011:
            { // FCVT_LU_S
                return alu::AluOp::FCVT_LU_S;
            }
            }
            break;
        }
        case 0b1100001:
        { // FCVT.(W|WU|L|LU).D
            switch (funct5)
            {
            case 0b00000:
            { // FCVT_W_D
                return alu::AluOp::FCVT_W_D;
            }
            case 0b00001:
            { // FCVT_WU_D
                return alu::AluOp::FCVT_WU_D;
            }
            case 0b00010:
            { // FCVT_L_D
                return alu::AluOp::FCVT_L_D;
            }
            case 0b00011:
            { // FCVT_LU_D
                return alu::AluOp::FCVT_LU_D;
            }
            }
            break;
        }
        case 0b1101000:
        { // FCVT.S.(W|WU|L|LU)
            switch (funct5)
            {
            case 0b00000:
            { // FCVT_S_W
                return alu::AluOp::FCVT_S_W;
            }
            case 0b00001:
            { // FCVT_S_WU
                return alu::AluOp::FCVT_S_WU;
            }
            case 0b00010:
            { // FCVT_S_L
                return alu::AluOp::FCVT_S_L;
            }
            case 0b00011:
            { // FCVT_S_LU
                return alu::AluOp::FCVT_S_LU;
            }
            }
            break;
        }
        case 0b1101001:
        { // FCVT.D.(W|WU|L|LU)
            switch (funct5)
            {
            case 0b00000:
            { // FCVT_D_W
                return alu::AluOp::FCVT_D_W;
            }
            case 0b00001:
            { // FCVT_D_WU
                return alu::AluOp::FCVT_D_WU;
            }
            case 0b00010:
            { // FCVT_D_L
                return alu::AluOp::FCVT_D_L;
            }
            case 0b00011:
            { // FCVT_D_LU
                return alu::AluOp::FCVT_D_LU;
            }
            }
            break;
        }
        case 0b0010000:
        { // FSGNJ(N|X).S
            switch (funct3)
            {
            case 0b000:
            { // FSGNJ
                return alu::AluOp::FSGNJ_S;
            }
            case 0b001:
            { // FSGNJN
                return alu::AluOp::FSGNJN_S;
            }
            case 0b010:
            { // FSGNJX
                return alu::AluOp::FSGNJX_S;
            }
            }
            break;
        }
        case 0b0010001:
        { // FSGNJ(N|X).D
            switch (funct3)
            {
            case 0b000:
            { // FSGNJ
                return alu::AluOp::FSGNJ_D;
            }
            case 0b001:
            { // FSGNJN
                return alu::AluOp::FSGNJN_D;
            }
            case 0b010:
            { // FSGNJX
                return alu::AluOp::FSGNJX_D;
            }
            }
            break;
        }
        case 0b0010100:
        { // F(MIN|MAX).S
            switch (funct3)
            {
            case 0b000:
            { // FMIN
                return alu::AluOp::FMIN_S;
            }
            case 0b001:
            { // FMAX
                return alu::AluOp::FMAX_S;
            }
            }
            break;
        }
        case 0b0010101:
        { // F(MIN|MAX).D
            switch (funct3)
            {
            case 0b000:
            { // FMIN
                return alu::AluOp::FMIN_D;
            }
            case 0b001:
            { // FMAX
                return alu::AluOp::FMAX_D;
            }
            }
            break;
        }
        case 0b1010000:
        { // F(EQ|LT|LE).S
            switch (funct3)
            {
            case 0b010:
            { // FEQ
                return alu::AluOp::FEQ_S;
            }
            case 0b001:
            { // FLT
                return alu::AluOp::FLT_S;
            }
            case 0b000:
            { // FLE
                return alu::AluOp::FLE_S;
            }
            }
            break;
        }
        case 0b1010001:
        { // F(EQ|LT|LE).D
            switch (funct3)
            {
            case 0b010:
            { // FEQ
                return alu::AluOp::FEQ_D;
            }
            case 0b001:
            { // FLT
                return alu::AluOp::FLT_D;
            }
            case 0b000:
            { // FLE
                return alu::AluOp::FLE_D;
            }
            }
            break;
        }
        case 0b1111000:
        { // FMV.W.X
            return alu::AluOp::FMV_W_X;
        }
        case 0b1111001:
        { // FMV.D.X
            return alu::AluOp::FMV_D_X;
        }
        case 0b1110000:
        { // FMV.X.W, FCLASS.S
            switch (funct3)
            {
            case 0b000:
            {
                return alu::AluOp::FMV_X_W;
            }
            case 0b001:
            {
                return alu::AluOp::FCLASS_S;
            }
            }
            break;
        }
        case 0b1110001:
        { // FMV.X.D, FCLASS.D
            switch (funct3)
            {
            case 0b000:
            {
                return alu::AluOp::FMV_X_D;
            }
            case 0b001:
            {
                return alu::AluOp::FCLASS_D;
            }
            }
            break;
        }
        case 0b1000011:
        { // FMADD.S, FMADD.D
            switch (funct2)
            {
            case 0b00:
            { // FMADD.S
                return alu::AluOp::FMADD_S;
            }
            case 0b01:
            { // FMADD.D
                return alu::AluOp::FMADD_D;
            }
            }
            break;
        }
        case 0b1000111:
        { // FMSUB.S, FMSUB.D
            switch (funct2)
            {
            case 0b00:
            { // FMSUB.S
                return alu::AluOp::FMSUB_S;
            }
            case 0b01:
            { // FMSUB.D
                return alu::AluOp::FMSUB_D;
            }
            }
            break;
        }
        case 0b1001011:
        { // FNMADD.S, FNMADD.D
            switch (funct2)
            {
            case 0b00:
            { // FNMADD.S
                return alu::AluOp::FNMADD_S;
            }
            case 0b01:
            { // FNMADD.D
                return alu::AluOp::FNMADD_D;
            }
            }
            break;
        }
        case 0b1001111:
        { // FNMSUB.S, FNMSUB.D
            switch (funct2)
            {
            case 0b00:
            { // FNMSUB.S
                return alu::AluOp::FNMSUB_S;
            }
            case 0b01:
            { // FNMSUB.D
                return alu::AluOp::FNMSUB_D;
            }
            }
            break;
        }
        }
        break;
    }

    case 0b0000111:
    { // F-Type Load
        switch (funct3)
        {
        case 0b010:
        { // FLW
            return alu::AluOp::ADD;
        }
        case 0b011:
        { // FLD
            return alu::AluOp::ADD;
        }
        }
        break;
    }

    case 0b0100111:
    { // F-Type Store
        switch (funct3)
        {
        case 0b010:
        { // FSW
            return alu::AluOp::ADD;
            break;
        }
        case 0b011:
        { // FSD
            return alu::AluOp::ADD;
            break;
        }
        default:
            break;
        }
        break;
    }
    }

    return alu::AluOp::NONE;
}
}//namespace Kites
