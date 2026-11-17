#include "reorder_program.h"
#include "common/instructions.h"      // for disassemble(uint32_t)
#include "reordering/DAG.h"
#include "reordering/parser.h"
#include "reordering/scheduler.h"
#include <cstdio>
#include <sstream>
#include <iomanip>

namespace Kites
{
static std::string toHexString(const std::vector<uint32_t> &words)
{
    std::string s;
    char buf[16];
    for (uint32_t w : words)
    {
        snprintf(buf, sizeof buf, "0x%08x ", w); // keep "0x" + 8 digits, your parser needs it
        s += buf;
    }
    return s;
}


AssembledProgram applyReordering(const AssembledProgram &in, const ReorderResult &r)
{
    const size_t n = in.text_buffer.size();
    if (r.words.size() != n || r.origIndex.size() != n)
        throw std::runtime_error("reorder: instruction count mismatch");

    std::vector<unsigned> newPos(n);               // old instruction number -> new slot
    for (size_t k = 0; k < n; ++k)
    {
        if (r.origIndex[k] < 0 || static_cast<size_t>(r.origIndex[k]) >= n)
            throw std::runtime_error("reorder: bad instruction index");
        newPos[r.origIndex[k]] = static_cast<unsigned>(k);
    }

    AssembledProgram out = in;
    out.text_buffer = r.words;
    out.instruction_number_line_number_mapping.clear();
    out.instruction_number_instruction_text_mapping.clear();
    out.instruction_number_disassembly_mapping.clear();   // DumpDisasssembly rebuilds it

    for (size_t k = 0; k < n; ++k)
    {
        const unsigned old = static_cast<unsigned>(r.origIndex[k]);
        if (auto it = in.instruction_number_line_number_mapping.find(old);
            it != in.instruction_number_line_number_mapping.end())
            out.instruction_number_line_number_mapping[k] = it->second;
        if (auto it = in.instruction_number_instruction_text_mapping.find(old);
            it != in.instruction_number_instruction_text_mapping.end())
            out.instruction_number_instruction_text_mapping[k] = it->second;
        if (old < in.intermediate_code.size() && k < out.intermediate_code.size())
            out.intermediate_code[k] = in.intermediate_code[old];
    }

    // breakpoints: each source line must now point at its instruction's new slot
    for (auto &entry : out.line_number_instruction_number_mapping)
        if (entry.second < n)
            entry.second = newPos[entry.second];

    return out;
}

ReorderResult reorderWords(const std::vector<uint32_t> &text, bool forwarding)
{
    parser p(toHexString(text));
    std::vector<parsed_instruction> instrs = p.instructions();
    DAG dag(instrs, forwarding);
    scheduler s(dag.blocks(), dag.depend_matrix_full(), forwarding);

    ReorderResult r;
    std::vector<parsed_instruction> out = s.code_scheduler(&r.cyclesBefore, &r.cyclesAfter);
    for (const auto &pi : out)
    {
        r.words.push_back(static_cast<uint32_t>(std::stoul(pi.inst, nullptr, 2))); // inst = 32-char binary string
        r.origIndex.push_back(pi.index);
    }
    return r;
}

std::string formatReordered(const ReorderResult &r)
{
    // Same layout as the Disassembly view: "<addr>: <hex word>   <instruction>".
    // The original -> new instruction numbers are drawn by the Reorder tab's right gutter
    // (from r.origIndex), so they are intentionally not part of the text.
    const size_t n = r.words.size();
    int hexDigits = 1;
    for (size_t t = n * 4; t >>= 4;)
        ++hexDigits;

    std::ostringstream o;
    for (size_t k = 0; k < n; ++k)
    {
        o << "  " << std::setfill(' ') << std::setw(hexDigits) << std::right << std::hex << (k * 4)
          << std::dec << std::left << std::setw(0) << ": " << std::setfill('0') << std::setw(8)
          << std::right << std::hex << r.words[k] << std::dec << std::setfill(' ')
          << "             " << instruction_set::disassemble(r.words[k]) << "\n";
    }
    return o.str();
}
} // namespace Kites