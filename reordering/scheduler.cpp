#include "scheduler.h"
#include "DAG.h"
#include "parser.h"
#include <algorithm>
#include <numeric>

// helper function

int scheduler::delay(dpd_format type, parsed_instruction a, parsed_instruction)
{
    switch (type)
    {
    case dpd_format::NA:
        return 0;
    case dpd_format::RAW:
    case dpd_format::RAW3:
        if (forwarding)
            return a.format == InstrFormat::L ? 2 : 1;
        return 3;
    default:
        return 1;
    }
}

scheduler::scheduler(std::vector<std::vector<parsed_instruction>> blocks,
                     std::vector<std::vector<std::vector<depend_description>>> given, bool temp1)
{
    depend_matrix_full = given;
    forwarding = temp1;
    this->blocks = blocks;
}

std::vector<int> scheduler::block_scheduler(const std::vector<parsed_instruction> &instrs,
                                            const DAG_matrix &block)

{
    int n = instrs.size();
    std::vector<int> reordered_block;

    DelayMatrix dly(n, std::vector<int>(n, 0));

    std::vector<int> indeg(n, 0);

    for (int k = 0; k < n; k++)
    {
        for (int l = k + 1; l < n; l++)
        {
            if (block[k][l].frmt != dpd_format::NA)
            {
                dly[k][l] = delay(block[k][l].frmt, instrs[k], instrs[l]);
                indeg[l]++;
            }
        }
    }

    heurestics h(dly, block);

    std::vector<int> issue(n, -1);
    std::vector<int> ready;

    int cycle = -1;

    for (int i = 0; i < n; i++)
    {
        if (indeg[i] == 0)
            ready.push_back(i); // initializing instruction with zero indegree meaning
                                // totally indpendent instruction first
    }

    while (reordered_block.size() < n)
    {
        sched_state st{issue, cycle};

        int best = -1;
        std::vector<int> best_key;

        for (int node : ready)
        {
            std::vector<int> key = h.get_heurestic_tuple(node, st);
            if (best == -1 || key < best_key)
            {
                best = node;
                best_key = std::move(key);
            }
        }

        issue[best] = cycle + 1 + h.get_stalls(best, st);

        cycle = issue[best];
        reordered_block.push_back(best);

        ready.erase(std::find(ready.begin(), ready.end(), best));

        for (int j = best + 1; j < n; j++)
            if (block[best][j].frmt != dpd_format::NA && --indeg[j] == 0)
                ready.push_back(j);
    }

    return reordered_block;
}

int scheduler::block_cycles(std::vector<parsed_instruction> &instrs,
                            std::vector<std::vector<depend_description>> &block,
                            std::vector<int> &order)
{
    std::vector<int> issue(instrs.size(), -1);
    int cycle = -1;
    for (int node : order)
    {
        int t = cycle + 1;
        for (int k = 0; k < node; k++)
            if (block[k][node].frmt != dpd_format::NA && issue[k] != -1)
                t = std::max(t, issue[k] + delay(block[k][node].frmt, instrs[k], instrs[node]));
        issue[node] = t;
        cycle = t;
    }
    return cycle + 1;
}

std::vector<parsed_instruction> scheduler::code_scheduler(int *cycles_before, int *cycles_after)
{
    std::vector<parsed_instruction> program;

    int total_before = 0;
    int total_after = 0;
    for (int b = 0; b < blocks.size(); b++)
    {
        std::vector<parsed_instruction> &instrs = blocks[b];
        std::vector<std::vector<depend_description>> &dep = depend_matrix_full[b];

        std::vector<int> original_order(instrs.size());
        std::iota(original_order.begin(), original_order.end(), 0);

        std::vector<int> scheduled_order = block_scheduler(instrs, dep);

        int cycles_before_re = block_cycles(instrs, dep, original_order);
        int cycles_after_re = block_cycles(instrs, dep, scheduled_order);

        std::vector<int> final_order = original_order;

        if (cycles_after_re < cycles_before_re)
        {
            final_order = scheduled_order;
        }
        total_after += std::min(cycles_after_re,cycles_before_re);
        total_before += cycles_before_re;

        for (int order : final_order)
        {
            program.push_back(instrs[order]);
        }
    }

    *cycles_after = total_after;
    *cycles_before = total_before;

    return program;
}
