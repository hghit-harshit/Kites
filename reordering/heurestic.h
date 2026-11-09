#pragma once
#include <vector>
#include "DAG.h"

using DelayMatrix = std::vector<std::vector<int>>;
using DAG_matrix  = std::vector<std::vector<depend_description>>;

struct sched_state {
    const std::vector<int> &issue;  
    int cycle;                      
};

class heurestics {
    DelayMatrix dly;
    DAG_matrix  dpd;
    int n;
    std::vector<int> succ_count, interlocks, depth;
public:
    heurestics(DelayMatrix delays, DAG_matrix matr);
    int get_stalls(int i, const sched_state &st) const;
    int get_raw_successor_count(int i) const { return succ_count[i]; }
    int interlocks_with_successor(int i) const { return interlocks[i]; }
    int depth_to_leaf(int i) const { return depth[i]; }
    std::vector<int> get_heurestic_tuple(int i, const sched_state &st) const;
};