/**
 * @brief This file contains the definition of the InstructionType enum class,
 * along with enum to string mapping
 * 
 */

#pragma once

#include <string>
#include <array>
#include "utils/to_index.hpp"
namespace Kites
{
/*TODO
* Make the enum value name consistent across the code base 
* Either it shoudl follo ALL_CAPS or PascalCase
* Also we can use magic_enum library to avoid writing the mapping manually
* but this would do for now
*/
namespace instruction_set
{
enum class InstructionType 
{
    R_TYPE = 0,
    I_TYPE,
    S_TYPE,
    B_TYPE,
    U_TYPE,
    J_TYPE,
    F_TYPE,
    INSTRUCTION_TYPE_COUNT, // this is used to get the count of valid instruction types
    UNKNOWN // putting unknown in the last becase we only want the
           //count of valid instruction types
};

inline std::array<std::string, toIndex(InstructionType::INSTRUCTION_TYPE_COUNT)> 
instructionTypeNames = 
{
    "R-Type", 
    "I-Type", 
    "S-Type", 
    "B-Type", 
    "U-Type", 
    "J-Type", 
    "F-Type"
};
}// namespace instruction_set
}// namespace Kites
