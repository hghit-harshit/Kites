/**
 * @file alu.h
 * @brief Contains the definition of the alu class for performing arithmetic and logic operations.
 * @author Vishank Singh, httpa://github.com/VishankSingh
 */
#ifndef ALU_H
#define ALU_H

#include <cfenv>
#include <cmath>
#include <cstdint>
#include <ostream>

// #pragma float_control(precise, on)
// #pragma STDC FENV_ACCESS ON

#pragma GCC diagnostic ignored "-Wstrict-aliasing"

#pragma GCC optimize("no-fast-math")

namespace Kites
{
#define FCSR_INVALID_OP (1 << 0)  // Invalid operation
#define FCSR_DIV_BY_ZERO (1 << 1) // Divide by zero
#define FCSR_OVERFLOW (1 << 2)    // Overflow
#define FCSR_UNDERFLOW (1 << 3)   // Underflow
#define FCSR_INEXACT (1 << 4)     // Inexact result

namespace alu
{
/*TODO
*Make the naming consistent, in this enum
* right now we have both kname and NAME_CON,
* we should make them PascalCase.
*/
enum class AluOp
{
    NONE,   ///< No operation.
    ADD,    ///< Addition operation.
    ADDW,   ///< Addition word operation.
    SUB,    ///< Subtraction operation.
    SUBW,   ///< Subtraction word operation.
    MUL,    ///< Multiplication operation.
    MULH,   ///< Multiplication high operation.
    MULHSU, ///< Multiplication high signed and unsigned operation.
    MULHU,  ///< Multiplication high unsigned operation.
    MULW,   ///< Multiplication word operation.
    DIV,    ///< Division operation.
    DIVW,   ///< Division word operation.
    DIVU,   ///< Unsigned division operation.
    DIVUW,  ///< Unsigned division word operation.
    REM,    ///< Remainder operation.
    REMW,   ///< Remainder word operation.
    REMU,   ///< Unsigned remainder operation.
    REMUW,  ///< Unsigned remainder word operation.
    AND,    ///< Bitwise And operation.
    OR,     ///< Bitwise Or operation.
    XOR,    ///< Bitwise Xor operation.
    SLL,    ///< Shift left logical operation.
    SLLW,   ///< Shift left logical word operation.
    SRL,    ///< Shift right logical operation.
    SRLI,   ///< Shift right logical immediate operation.
    SRLW,   ///< Shift right logical word operation.
    SRA,    ///< Shift right arithmetic operation.
    SRAI,   ///< Shift right arithmetic immediate operation.
    SRAW,   ///< Shift right arithmetic word operation.
    SLT,    ///< Set less than operation.
    SLTU,   ///< Unsigned set less than operation.

    // Floating point operations
    FMADD_S,  ///< Floating point multiply-add single operation.
    FMSUB_S,  ///< Floating point multiply-subtract single operation.
    FNMADD_S, ///< Floating point negative multiply-add single operation.
    FNMSUB_S, ///< Floating point negative multiply-subtract single operation.

    FADD_S,   ///< Floating point addition operation.
    FSUB_S,   ///< Floating point subtraction operation.
    FMUL_S,   ///< Floating point multiplication operation.
    FDIV_S,   ///< Floating point division operation.
    FSQRT_S,  ///< Floating point square root operation.
    FSGNJ_S,  ///< Floating point sign inject operation.
    FSGNJN_S, ///< Floating point sign inject negative operation.
    FSGNJX_S, ///< Floating point sign inject kXor operation.
    FMIN_S,   ///< Floating point minimum operation.
    FMAX_S,   ///< Floating point maximum operation.
    FEQ_S,    ///< Floating point equal operation.
    FLT_S,    ///< Floating point less than operation.
    FLE_S,    ///< Floating point less than or equal operation.
    FCLASS_S, ///< Floating point class operation.

    FCVT_W_S,  ///< Floating point convert to word operation.
    FCVT_WU_S, ///< Floating point convert to unsigned word operation.
    FCVT_L_S,  ///< Floating point convert double to long operation.
    FCVT_LU_S, ///< Floating point convert double to unsigned long operation.

    FCVT_S_W,  ///< Floating point convert word to operation.
    FCVT_S_WU, ///< Floating point convert unsigned word to operation.
    FCVT_S_L,  ///< Floating point convert long to single operation.
    FCVT_S_LU, ///< Floating point convert unsigned long to single operation.

    FMV_X_W, ///< Floating point move to integer operation.
    FMV_W_X, ///< Floating point move from integer operation.

    FMADD_D,  ///< Floating point multiply-add double operation.
    FMSUB_D,  ///< Floating point multiply-subtract double operation.
    FNMADD_D, ///< Floating point negative multiply-add double operation.
    FNMSUB_D, ///< Floating point negative multiply-subtract double operation.

    FADD_D,   ///< Floating point addition double operation.
    FSUB_D,   ///< Floating point subtraction double operation.
    FMUL_D,   ///< Floating point multiplication double operation.
    FDIV_D,   ///< Floating point division double operation.
    FSQRT_D,  ///< Floating point square root double operation.
    FSGNJ_D,  ///< Floating point sign inject double operation.
    FSGNJN_D, ///< Floating point sign inject negative double operation.
    FSGNJX_D, ///< Floating point sign inject kXor double operation.
    FMIN_D,   ///< Floating point minimum double operation.
    FMAX_D,   ///< Floating point maximum double operation.
    FEQ_D,    ///< Floating point equal double operation.
    FLT_D,    ///< Floating point less than double operation.
    FLE_D,    ///< Floating point less than or equal double operation.
    FCLASS_D, ///< Floating point class double operation.

    FCVT_W_D,  ///< Floating point convert double to word operation.
    FCVT_WU_D, ///< Floating point convert double to unsigned word operation.
    FCVT_L_D,  ///< Floating point convert double to long operation.
    FCVT_LU_D, ///< Floating point convert double to unsigned long operation.

    FCVT_D_W,  ///< Floating point convert word to double operation.
    FCVT_D_WU, ///< Floating point convert unsigned word to double operation.
    FCVT_D_L,  ///< Floating point convert long to double operation.
    FCVT_D_LU, ///< Floating point convert unsigned long to double operation.

    FCVT_S_D, ///< Floating point convert double to single operation.
    FCVT_D_S, ///< Floating point convert single to double operation.

    FMV_D_X, ///< Floating point move to integer double operation.
    FMV_X_D, ///< Floating point move from integer double operation.
};

inline std::ostream &operator<<(std::ostream &os, const AluOp &op)
{
    switch (op)
    {
    case AluOp::NONE:
        os << "NONE";
        break;
    case AluOp::ADD:
        os << "ADD";
        break;
    case AluOp::SUB:
        os << "SUB";
        break;
    case AluOp::MUL :
        os << "MUL";
        break;
    case AluOp::DIV:
        os << "DIV";
        break;
    case AluOp::DIVU:
        os << "DIVU";
        break;
    case AluOp::REM:
        os << "REM";
        break;
    case AluOp::REMU:
        os << "REMU";
        break;
    case AluOp::AND:
        os << "AND";
        break;
    case AluOp::OR:
        os << "OR";
        break;
    case AluOp::XOR:
        os << "XOR";
        break;
    case AluOp::SLL:
        os << "SLL";
        break;
    case AluOp::SRL:
        os << "SRL";
        break;
    case AluOp::SRA:
        os << "SRA";
        break;
    case AluOp::SLT:
        os << "SLT";
        break;
    case AluOp::SLTU:
        os << "SLTU";
        break;
    case AluOp::ADDW:
        os << "ADDW";
        break;
    case AluOp::SUBW:
        os << "SUBW";
        break;
    case AluOp::MULW:
        os << "MULW";
        break;
    case AluOp::DIVW:
        os << "DIVW";
        break;
    case AluOp::DIVUW:
        os << "DIVUW";
        break;
    case AluOp::REMW:
        os << "REMW";
        break;
    case AluOp::REMUW:
        os << "REMUW";
        break;
    case AluOp::MULH:
        os << "MULH";
        break;
    case AluOp::MULHSU:
        os << "MULHSU";
        break;
    case AluOp::MULHU:
        os << "MULHU";
        break;
    case AluOp::SLLW:
        os << "SLLW";
        break;
    case AluOp::SRLW:
        os << "SRLW";
        break;
    case AluOp::SRAW:
        os << "SRAW";
        break;
    case AluOp::FMADD_S:
        os << "FMADD_S";
        break;
    case AluOp::FMSUB_S:
        os << "FMSUB_S";
        break;
    case AluOp::FNMADD_S:
        os << "FNMADD_S";
        break;
    case AluOp::FNMSUB_S:
        os << "FNMSUB_S";
        break;
    case AluOp::FADD_S:
        os << "FADD_S";
        break;
    case AluOp::FSUB_S:
        os << "FSUB_S";
        break;
    case AluOp::FMUL_S:
        os << "FMUL_S";
        break;
    case AluOp::FDIV_S:
        os << "FDIV_S";
        break;
    case AluOp::FSQRT_S:
        os << "FSQRT_S";
        break;
    case AluOp::FSGNJ_S:
        os << "FSGNJ_S";
        break;
    case AluOp::FSGNJN_S:
        os << "FSGNJN_S";
        break;
    case AluOp::FSGNJX_S:
        os << "FSGNJX_S";
        break;
    case AluOp::FMIN_S:
        os << "FMIN_S";
        break;
    case AluOp::FMAX_S:
        os << "FMAX_S";
        break;
    case AluOp::FEQ_S:
        os << "FEQ_S";
        break;
    case AluOp::FLT_S:
        os << "FLT_S";
        break;
    case AluOp::FLE_S:
        os << "FLE_S";
        break;
    case AluOp::FCLASS_S:
        os << "FCLASS_S";
        break;
    case AluOp::FCVT_W_S:
        os << "FCVT_W_S";
        break;
    case AluOp::FCVT_WU_S:
        os << "FCVT_WU_S";
        break;
    case AluOp::FCVT_L_S:
        os << "FCVT_L_S";
        break;
    case AluOp::FCVT_LU_S:
        os << "FCVT_LU_S";
        break;
    case AluOp::FCVT_S_W:
        os << "FCVT_S_W";
        break;
    case AluOp::FCVT_S_WU:
        os << "FCVT_S_WU";
        break;
    case AluOp::FCVT_S_L:
        os << "FCVT_S_L";
        break;
    case AluOp::FCVT_S_LU:
        os << "FCVT_S_LU";
        break;
    case AluOp::FMADD_D:
        os << "FMADD_D";
        break;
    case AluOp::FMSUB_D:
        os << "FMSUB_D";
        break;
    case AluOp::FNMADD_D:
        os << "FNMADD_D";
        break;
    case AluOp::FNMSUB_D:
        os << "FNMSUB_D";
        break;
    case AluOp::FADD_D:
        os << "FADD_D";
        break;
    case AluOp::FSUB_D:
        os << "FSUB_D";
        break;
    case AluOp::FMUL_D:
        os << "FMUL_D";
        break;
    case AluOp::FDIV_D:
        os << "FDIV_D";
        break;
    case AluOp::FSQRT_D:
        os << "FSQRT_D";
        break;
    case AluOp::FSGNJ_D:
        os << "FSGNJ_D";
        break;

    default:
        os << "UNKNOWN";
        break;
    }
    return os;
}
/**
 * @brief The alu class is responsible for performing arithmetic and logic operations.
 */
class Alu
{
  public:
    bool carry_ = false;    ///< Carry flag.
    bool zero_ = false;     ///< Zero flag.
    bool negative_ = false; ///< Negative flag.
    bool overflow_ = false; ///< Overflow flag.

    Alu() = default;
    ~Alu() = default;

    /**
     * @brief Executes the given alu operation.
     * @tparam T Integer type (int32_t, uint32_t, etc.).
     * @param op The alu operation.
     * @param a First operand.
     * @param b Second operand.
     * @return A pair (result, overflow_flag).
     */
    [[nodiscard]] static std::pair<uint64_t, bool> execute(AluOp op, uint64_t a, uint64_t b);

    // TODO: check all the floating point operations

    [[nodiscard]] static std::pair<uint64_t, uint8_t>
    fpexecute(AluOp op, uint64_t ina, uint64_t inb, uint64_t inc, uint8_t rm);

    [[nodiscard]] static std::pair<uint64_t, bool> dfpexecute(AluOp op, uint64_t ina, uint64_t inb,
                                                              uint64_t inc, uint8_t rm);

    void setFlags(bool carry, bool zero, bool negative, bool overflow);
};
} // namespace alu
}//namespace Kites
#endif // ALU_H