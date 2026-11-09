
#include "DAG.h"
#include "parser.h"
#include <vector>

using namespace std;

DAG::DAG(vector<parsed_instruction> &temp, bool forw)
{
    instr = temp;
    this->forward = forw;
}

dpd_format DAG::dep_type(parsed_instruction a, parsed_instruction b)
{

    if (a.format == InstrFormat::Unknown || b.format == InstrFormat::Unknown)
    {
        return dpd_format::CTRL;
    }
    if (a.rd > 0 && (a.rd == b.rs1 || a.rd == b.rs2))
        return dpd_format::RAW;

    if (b.format == InstrFormat::B || b.format == InstrFormat::J)
        return dpd_format::CTRL;

    if (a.rd > 0 && a.rd == b.rd)
        return dpd_format::WAW;
    if (b.rd > 0 && (b.rd == a.rs1 || b.rd == a.rs2))
        return dpd_format::WAR;

    if (a.format == InstrFormat::S && b.format == InstrFormat::S)
        return dpd_format::WAW;
    if (a.format == InstrFormat::L && b.format == InstrFormat::S)
        return dpd_format::WAR;
    if (a.format == InstrFormat::S && b.format == InstrFormat::L)
        return dpd_format::MEM;

    return dpd_format::NA;
}

std::vector<std::vector<parsed_instruction>> DAG::blocks()
{
    int n = instr.size();

    std::vector<bool> leader(n, false);

    if (n > 0)
        leader[0] = true;

    for (int i = 0; i < n; i++)
    {
        if (instr[i].format == InstrFormat::B || instr[i].format == InstrFormat::J)
        {
            if (i + 1 < n)
                leader[i + 1] = true; // after a branch
            int target = i + instr[i].memory_add_offset / 4;
            if (target >= 0 && target < n)
                leader[target] = true; // branch target
        }
    }

    std::vector<std::vector<parsed_instruction>> out;
    std::vector<parsed_instruction> cur;
    for (int i = 0; i < n; i++)
    {
        if (leader[i] && !cur.empty())
        {
            out.push_back(cur);
            cur.clear();
        }
        cur.push_back(instr[i]);
        cur.back().index_in_block = (int)cur.size() - 1; // set on the stored copy
    }
    if (!cur.empty())
        out.push_back(cur);
    return out;
}

std::vector<std::vector<std::vector<depend_description>>> DAG::depend_matrix_full()
{
    std::vector<std::vector<std::vector<depend_description>>> ans;
    std::vector<std::vector<parsed_instruction>> block_vector = blocks();
    for (auto bloc : block_vector)
    {
        ans.push_back(depend_matrix_block(bloc));
    }

    return ans;
}

std::vector<std::vector<depend_description>>
DAG::depend_matrix_block(std::vector<parsed_instruction> block)
{
    vector<std::vector<depend_description>> depend_matrix(
        block.size(), std::vector<depend_description>(block.size()));

    for (int i = 0; i < block.size(); i++)
    {
        for (int j = i + 1; j < block.size(); j++)
        {
            depend_matrix[i][j].frmt = dep_type(block[i], block[j]); // i->j RAW
        }
    }

    return depend_matrix;
}
