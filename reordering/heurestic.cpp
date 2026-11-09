#include "heurestic.h"
#include <algorithm>

heurestics::heurestics(DelayMatrix delays, DAG_matrix matr)
    : dly(std::move(delays)), dpd(std::move(matr)), n((int)dly.size()),
      succ_count(n, 0), interlocks(n, 0), depth(n, 0)
{
    
    for (int i = n - 1; i >= 0; i--)
        for (int j = i + 1; j < n; j++) {
            if (dpd[i][j].frmt == dpd_format::NA) continue;
            succ_count[i]++;
            if (dly[i][j] > 1) interlocks[i] = 1;          
            depth[i] = std::max(depth[i], dly[i][j] + depth[j]);  
        }
}


int heurestics::get_stalls(int i, const sched_state &st) const
{
    int earliest = st.cycle + 1, t = earliest;
    for (int k = 0; k < i; k++)
        if (dpd[k][i].frmt != dpd_format::NA && st.issue[k] != -1)
            t = std::max(t, st.issue[k] + dly[k][i]);
    return t - earliest;
}


std::vector<int> heurestics::get_heurestic_tuple(int i, const sched_state &st) const
{
    return { get_stalls(i, st),   // 1. avoid interlock with the previous instruction
             -interlocks[i],      // 2. prefer ones that interlock with a successor
             -succ_count[i],      // 3. prefer more successors
             -depth[i],           // 4. prefer longer path to leaf
             i };                 // 5. tie-break: original program order
}