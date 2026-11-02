#pragma once
#include <cstdint>

namespace Kites::reorder
{

enum class Kind : uint8_t { Lui, Auipc, Jal, Jalr, Branch, Load, Store, OpImm, Op, Ecall, Other };

struct DecodedInst
{
    uint32_t word = 0;
    Kind kind = Kind::Other;
    uint8_t rd = 0, rs1 = 0, rs2 = 0;      // raw bit fields (bits 7-11, 15-19, 20-24)
    bool writes_rd = false;                // true dependency info
    bool reads_rs1 = false;
    bool reads_rs2 = false;
    bool is_load = false, is_store = false;
    bool is_branch = false, is_jump = false;
    bool is_barrier = false;               // never moves and ends a block
};

inline DecodedInst decode(uint32_t w)
{
    DecodedInst d;
    d.word = w;
    d.rd  = (w >> 7)  & 0x1F;
    d.rs1 = (w >> 15) & 0x1F;
    d.rs2 = (w >> 20) & 0x1F;

    switch (w & 0x7F)
    {
    case 0x37: d.kind = Kind::Lui;   d.writes_rd = true; break;
    case 0x17: d.kind = Kind::Auipc; d.writes_rd = true; d.is_barrier = true; break;
    case 0x6F: d.kind = Kind::Jal;   d.writes_rd = true; d.is_jump = true; d.is_barrier = true; break;
    case 0x67: d.kind = Kind::Jalr;  d.writes_rd = true; d.reads_rs1 = true; d.is_jump = true; d.is_barrier = true; break;
    case 0x63: d.kind = Kind::Branch; d.reads_rs1 = d.reads_rs2 = true; d.is_branch = true; d.is_barrier = true; break;
    case 0x03: d.kind = Kind::Load;  d.writes_rd = true; d.reads_rs1 = true; d.is_load = true; break;
    case 0x23: d.kind = Kind::Store; d.reads_rs1 = d.reads_rs2 = true; d.is_store = true; break;
    case 0x13: case 0x1B: d.kind = Kind::OpImm; d.writes_rd = true; d.reads_rs1 = true; break;
    case 0x33: case 0x3B: d.kind = Kind::Op;    d.writes_rd = true; d.reads_rs1 = d.reads_rs2 = true; break;
    case 0x73: d.kind = Kind::Ecall; d.is_barrier = true; break;
    default:   d.kind = Kind::Other; d.is_barrier = true; break;
    }
    return d;
}

} // namespace Kites::reorder
