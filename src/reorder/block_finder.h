#pragma once
#include <cstddef>
#include <cstdint>
#include <vector>

#include "reorder/decoded_inst.h"

namespace Kites::reorder
{

struct Block
{
    size_t begin;         // index of first instruction in the block
    size_t end;           // one past the last instruction (exclusive)
    bool hasTerminator;   // true if the last instruction is a branch or jump (it must stay put)
};

// Byte offset of a B-type (branch) instruction, sign-extended.
inline int32_t branchImm(uint32_t w)
{
    uint32_t imm = ((w >> 31) & 0x1) << 12 | ((w >> 7) & 0x1) << 11 |
                   ((w >> 25) & 0x3F) << 5 | ((w >> 8) & 0xF) << 1;
    return static_cast<int32_t>(imm << 19) >> 19;
}

// Byte offset of a J-type (jal) instruction, sign-extended.
inline int32_t jalImm(uint32_t w)
{
    uint32_t imm = ((w >> 31) & 0x1) << 20 | ((w >> 12) & 0xFF) << 12 |
                   ((w >> 20) & 0x1) << 11 | ((w >> 21) & 0x3FF) << 1;
    return static_cast<int32_t>(imm << 11) >> 11;
}

inline std::vector<Block> findBlocks(const std::vector<uint32_t> &words)
{
    const size_t n = words.size();
    std::vector<Block> blocks;
    if (n == 0)
        return blocks;

    // leader[i] == true means "a new block starts at instruction i".
    // Index n is a sentinel that closes the last block.
    std::vector<bool> leader(n + 1, false);
    leader[0] = true;
    leader[n] = true;

    for (size_t i = 0; i < n; ++i)
    {
        const DecodedInst d = decode(words[i]);

        if (d.is_branch || d.is_jump)
        {
            leader[i + 1] = true;                       // fall-through / return point

            if (d.kind == Kind::Branch || d.kind == Kind::Jal)
            {
                const int32_t off = (d.kind == Kind::Branch) ? branchImm(words[i])
                                                             : jalImm(words[i]);
                if (off % 4 == 0)
                {
                    const int64_t target = static_cast<int64_t>(i) + off / 4;
                    if (target >= 0 && target < static_cast<int64_t>(n))
                        leader[static_cast<size_t>(target)] = true;
                }
            }
        }
        else if (d.is_barrier)                          // ecall, auipc, unknown
        {
            leader[i] = true;                           // barrier starts its own block
            leader[i + 1] = true;                       // and ends it
        }
    }

    size_t begin = 0;
    for (size_t i = 1; i <= n; ++i)
    {
        if (!leader[i])
            continue;
        const DecodedInst last = decode(words[i - 1]);
        blocks.push_back({begin, i, last.is_branch || last.is_jump});
        begin = i;
    }
    return blocks;
}

} // namespace Kites::reorder
