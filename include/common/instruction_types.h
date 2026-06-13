#pragma once

enum class InstructionType 
{
    R_TYPE = 0,
    I_TYPE,
    S_TYPE,
    B_TYPE,
    U_TYPE,
    J_TYPE,
    F_TYPE,
    NUMBER_OF_TYPES,
    UNKNOWN // putting unknown in the last becase we only want the
           //count of valid instruction types
};
