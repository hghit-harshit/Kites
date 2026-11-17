#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include "common/assembled_program.h"


namespace Kites
{
struct ReorderResult
{
    std::vector<uint32_t> words;     // reordered machine code
    std::vector<int> origIndex;      // origIndex[k] = original instruction number of slot k
    int cyclesBefore = 0;
    int cyclesAfter = 0;
};

ReorderResult reorderWords(const std::vector<uint32_t> &text, bool forwarding);
std::string formatReordered(const ReorderResult &r);

AssembledProgram applyReordering(const AssembledProgram &in, const ReorderResult &r);
} // namespace Kites


