/**
 * @file instructions.h
 * @brief Contains the declarations for the InstructionEncoding struct and related functions.
 * @author Vishank Singh, https://github.com/VishankSingh
 */

#ifndef INSTRUCTIONS_H
#define INSTRUCTIONS_H

#include <bitset>
#include <cstdint>
#include <vector>
#include <string>
#include <unordered_map>

namespace instruction_set {


enum class Instruction {
  kadd, ksub, kand, kor, kxor, ksll, ksrl, ksra, kslt, ksltu,
  kaddw, ksubw, ksllw, ksrlw, ksraw,
  kaddi, kxori, kori, kandi, kslli, ksrli, ksrai, kslti, ksltiu,
  kaddiw, kslliw, ksrliw, ksraiw,
  klb, klh, klw, kld, klbu, klhu, klwu,
  ksb, ksh, ksw, ksd,
  kbeq, kbne, kblt, kbge, kbltu, kbgeu,
  klui, kauipc,
  kjal, kjalr,
  kecall, kebreak,
  kcsrrw, kcsrrs, kcsrrc, kcsrrwi, kcsrrsi, kcsrrci,
  kla, knop, kli, kmv, knot, kneg, knegw,
  ksextw, kseqz, ksnez, ksltz, ksgtz,
  kbeqz, kbnez, kblez, kbgez, kbltz, kbgtz,
  kbgt, kble, kbgtu, kbleu,
  kj, kjr, kret, kcall, ktail, kfence, kfence_i,

  kmul, kmulh, kmulhsu, kmulhu, kdiv, kdivu, krem, kremu,
  kmulw, kdivw, kdivuw, kremw, kremuw,

  kflw, kfsw, kfmadd_s, kfmsub_s, kfnmsub_s, kfnmadd_s,
  kfadd_s, kfsub_s, kfmul_s, kfdiv_s, kfsqrt_s,
  kfsgnj_s, kfsgnjn_s, kfsgnjx_s,
  kfmin_s, kfmax_s,
  kfcvt_w_s, kfcvt_wu_s, kfmv_x_w, kfmv_w_x,
  kfeq_s, kflt_s, kfle_s,
  kfclass_s, kfcvt_s_w, kfcvt_s_wu,
  kfcvt_l_s, kfcvt_lu_s, kfcvt_s_l, kfcvt_s_lu,

  kfld, kfsd, kfmadd_d, kfmsub_d, kfnmsub_d, kfnmadd_d,
  kfadd_d, kfsub_d, kfmul_d, kfdiv_d, kfsqrt_d,
  kfsgnj_d, kfsgnjn_d, kfsgnjx_d,
  kfmin_d, kfmax_d,
  kfcvt_s_d, kfcvt_d_s,
  kfeq_d, kflt_d, kfle_d,
  kfclass_d, kfcvt_w_d, kfcvt_wu_d, kfcvt_d_w, kfcvt_d_wu,
  kfcvt_l_d, kfcvt_lu_d, kfmv_x_d, kfcvt_d_l, kfcvt_d_lu, kfmv_d_x
};

// TODO: use enum class for instruction encoding


struct InstructionEncoding {
  int opcode;
  int funct2;
  int funct3;
  int funct5;
  int funct6;
  int funct7;
  
  InstructionEncoding(int opcode, int funct2, int funct3, int funct5, int funct6, int funct7)
  : opcode(opcode), funct2(funct2), funct3(funct3), funct5(funct5), funct6(funct6), funct7(funct7) {}
};

// opcode, funct2, funct3, funct5, func6, funct7
// TODO: use enum in place of strings from the parser stage
extern std::unordered_map<Instruction, InstructionEncoding> instruction_encoding_map;

extern std::unordered_map<std::string, Instruction> instruction_string_map;



struct RTypeInstructionEncoding {
  std::bitset<7> opcode;
  std::bitset<3> funct3;
  std::bitset<7> funct7;

  RTypeInstructionEncoding(unsigned int opcode, unsigned int funct3, unsigned int funct7)
      : opcode(opcode), funct3(funct3), funct7(funct7) {}
};

struct I1TypeInstructionEncoding {
  std::bitset<7> opcode;
  std::bitset<3> funct3;

  I1TypeInstructionEncoding(unsigned int opcode, unsigned int funct3)
      : opcode(opcode), funct3(funct3) {}
};

struct I2TypeInstructionEncoding {
  std::bitset<7> opcode;
  std::bitset<3> funct3;
  std::bitset<6> funct6;

  I2TypeInstructionEncoding(unsigned int opcode, unsigned int funct3, unsigned int funct6)
      : opcode(opcode), funct3(funct3), funct6(funct6) {}
};

struct I3TypeInstructionEncoding {
  std::bitset<7> opcode;
  std::bitset<3> funct3;
  std::bitset<7> funct7;

  I3TypeInstructionEncoding(unsigned int opcode, unsigned int funct3, unsigned int funct7)
      : opcode(opcode), funct3(funct3), funct7(funct7) {}
};

struct STypeInstructionEncoding {
  std::bitset<7> opcode;
  std::bitset<3> funct3;

  STypeInstructionEncoding(unsigned int opcode, unsigned int funct3)
      : opcode(opcode), funct3(funct3) {}
};

struct BTypeInstructionEncoding {
  std::bitset<7> opcode;
  std::bitset<3> funct3;

  BTypeInstructionEncoding(unsigned int opcode, unsigned int funct3)
      : opcode(opcode), funct3(funct3) {}
};

struct UTypeInstructionEncoding {
  std::bitset<7> opcode;

  UTypeInstructionEncoding(unsigned int opcode)
      : opcode(opcode) {}
};

struct JTypeInstructionEncoding {
  std::bitset<7> opcode;

  JTypeInstructionEncoding(unsigned int opcode)
      : opcode(opcode) {}
};

struct CSR_RTypeInstructionEncoding {
  std::bitset<7> opcode;
  std::bitset<3> funct3;

  CSR_RTypeInstructionEncoding(unsigned int opcode, unsigned int funct3)
      : opcode(opcode), funct3(funct3) {}
};

struct CSR_ITypeInstructionEncoding {
  std::bitset<7> opcode;
  std::bitset<3> funct3;

  CSR_ITypeInstructionEncoding(unsigned int opcode, unsigned int funct3)
      : opcode(opcode), funct3(funct3) {}
};

// Fextension instructions===========================================================================

struct FDRTypeInstructionEncoding { // fsgnj
  std::bitset<7> opcode;
  std::bitset<3> funct3;
  std::bitset<7> funct7;

  FDRTypeInstructionEncoding(unsigned int opcode, unsigned int funct3, unsigned int funct7)
      : opcode(opcode), funct3(funct3), funct7(funct7) {}
};

struct FDR1TypeInstructionEncoding { // fadd, fsub, fmul
  std::bitset<7> opcode;
  std::bitset<7> funct7;

  FDR1TypeInstructionEncoding(unsigned int opcode, unsigned int funct7)
      : opcode(opcode), funct7(funct7) {}
};

struct FDR2TypeInstructionEncoding { //fsqrt, have funct5 instead of rs2
  std::bitset<7> opcode;
  std::bitset<5> funct5;
  std::bitset<7> funct7;

  FDR2TypeInstructionEncoding(unsigned int opcode, unsigned int funct5, unsigned int funct7)
      : opcode(opcode), funct5(funct5), funct7(funct7) {}
};

struct FDR3TypeInstructionEncoding {
  std::bitset<7> opcode;
  std::bitset<3> funct3;
  std::bitset<5> funct5;
  std::bitset<7> funct7;

  FDR3TypeInstructionEncoding(unsigned int opcode, unsigned int funct3, unsigned int funct5, unsigned int funct7)
      : opcode(opcode), funct3(funct3), funct5(funct5), funct7(funct7) {}

};

struct FDR4TypeInstructionEncoding { //fmadd
  std::bitset<7> opcode;
  std::bitset<2> funct2;

  FDR4TypeInstructionEncoding(unsigned int opcode, unsigned int funct2)
      : opcode(opcode), funct2(funct2) {}
};

struct FDITypeInstructionEncoding {
  std::bitset<7> opcode;
  std::bitset<3> funct3;

  FDITypeInstructionEncoding(unsigned int opcode, unsigned int funct3)
      : opcode(opcode), funct3(funct3) {}
};

struct FDSTypeInstructionEncoding {
  std::bitset<7> opcode;
  std::bitset<3> funct3;

  FDSTypeInstructionEncoding(unsigned int opcode, unsigned int funct3)
      : opcode(opcode), funct3(funct3) {}
};

/**
 * @brief Enum that represents different syntax types for instructions.
 */
enum class SyntaxType {
  O_GPR_C_GPR_C_GPR,       ///< Opcode general-register , general-register , register
  O_GPR_C_GPR_C_I,        ///< Opcode general-register , general-register , immediate
  O_GPR_C_I,            ///< Opcode general-register , immediate
  O_GPR_C_GPR_C_IL,       ///< Opcode general-register , general-register , immediate , instruction_label
  O_GPR_C_GPR_C_DL,       ///< Opcode register , register , immediate , data_label
  O_GPR_C_IL,           ///< Opcode register , instruction_label
  O_GPR_C_DL,           ///< Opcode register , data_label
  O_GPR_C_I_LP_GPR_RP,    ///< Opcode register , immediate , lparen ( register )rparen
  O,                  ///< Opcode
  PSEUDO,              ///< Pseudo instruction

  O_GPR_C_CSR_C_GPR,       ///< Opcode general-register , csr , general-register
  O_GPR_C_CSR_C_I,        ///< Opcode general-register , csr , immediate

  O_FPR_C_FPR_C_FPR_C_FPR,    ///< Opcode floating-point-register , floating-point-register , floating-point-register , floating-point-register
  O_FPR_C_FPR_C_FPR_C_FPR_C_RM,    ///< Opcode floating-point-register , floating-point-register , floating-point-register , rounding_mode
  O_FPR_C_FPR_C_FPR,        ///< Opcode floating-point-register , floating-point-register , floating-point-register
  O_FPR_C_FPR_C_FPR_C_RM,    ///< Opcode floating-point-register , floating-point-register , floating-point-register , rounding_mode
  O_FPR_C_FPR,            ///< Opcode floating-point-register , floating-point-register
  O_FPR_C_FPR_C_RM,           ///< Opcode floating-point-register , floating-point-register , rounding_mode

  O_FPR_C_GPR,            ///< Opcode floating-point-register , general-register
  O_FPR_C_GPR_C_RM,           ///< Opcode floating-point-register , general-register , rounding_mode
  O_GPR_C_FPR,           ///< Opcode general-register , floating-point-register
  O_GPR_C_FPR_C_RM,       ///< Opcode general-register , floating-point-register , rounding_mode
  O_GPR_C_FPR_C_FPR,       ///< Opcode general-register , floating-point-register , floating-point-register
  O_FPR_C_I_LP_GPR_RP,    ///< Opcode floating-point-register , immediate , lparen ( general-register ) rparen
};

extern std::unordered_map<std::string, RTypeInstructionEncoding> R_type_instruction_encoding_map;
extern std::unordered_map<std::string, I1TypeInstructionEncoding> I1_type_instruction_encoding_map;
extern std::unordered_map<std::string, I2TypeInstructionEncoding> I2_type_instruction_encoding_map;
extern std::unordered_map<std::string, I3TypeInstructionEncoding> I3_type_instruction_encoding_map;
extern std::unordered_map<std::string, STypeInstructionEncoding> S_type_instruction_encoding_map;
extern std::unordered_map<std::string, BTypeInstructionEncoding> B_type_instruction_encoding_map;
extern std::unordered_map<std::string, UTypeInstructionEncoding> U_type_instruction_encoding_map;
extern std::unordered_map<std::string, JTypeInstructionEncoding> J_type_instruction_encoding_map;
extern std::unordered_map<std::string, CSR_RTypeInstructionEncoding> CSR_R_type_instruction_encoding_map;
extern std::unordered_map<std::string, CSR_ITypeInstructionEncoding> CSR_I_type_instruction_encoding_map;

extern std::unordered_map<std::string, FDRTypeInstructionEncoding> F_D_R_type_instruction_encoding_map;
extern std::unordered_map<std::string, FDR1TypeInstructionEncoding> F_D_R1_type_instruction_encoding_map;
extern std::unordered_map<std::string, FDR2TypeInstructionEncoding> F_D_R2_type_instruction_encoding_map;
extern std::unordered_map<std::string, FDR3TypeInstructionEncoding> F_D_R3_type_instruction_encoding_map;
extern std::unordered_map<std::string, FDR4TypeInstructionEncoding> F_D_R4_type_instruction_encoding_map;
extern std::unordered_map<std::string, FDITypeInstructionEncoding> F_D_I_type_instruction_encoding_map;
extern std::unordered_map<std::string, FDSTypeInstructionEncoding> F_D_S_type_instruction_encoding_map;

/**
 * @brief A map that associates instruction names with their expected syntax.
 * 
 * This map stores the expected syntax for various instructions, indexed by their names.
 */
extern std::unordered_map<std::string, std::vector<SyntaxType>> instruction_syntax_map;

bool isValidInstruction(const std::string &instruction);

bool isValidRTypeInstruction(const std::string &name);
bool isValidITypeInstruction(const std::string &instruction);
bool isValidI1TypeInstruction(const std::string &instruction);
bool isValidI2TypeInstruction(const std::string &instruction);
bool isValidI3TypeInstruction(const std::string &instruction);
bool isValidSTypeInstruction(const std::string &instruction);
bool isValidBTypeInstruction(const std::string &instruction);
bool isValidUTypeInstruction(const std::string &instruction);
bool isValidJTypeInstruction(const std::string &instruction);

bool isValidPseudoInstruction(const std::string &instruction);

bool isValidBaseExtensionInstruction(const std::string &instruction);

bool isValidCSRRTypeInstruction(const std::string &instruction);
bool isValidCSRITypeInstruction(const std::string &instruction);
bool isValidCSRInstruction(const std::string &instruction);

bool isValidFDRTypeInstruction(const std::string &instruction);
bool isValidFDR1TypeInstruction(const std::string &instruction);
bool isValidFDR2TypeInstruction(const std::string &instruction);
bool isValidFDR3TypeInstruction(const std::string &instruction);
bool isValidFDR4TypeInstruction(const std::string &instruction);
bool isValidFDITypeInstruction(const std::string &instruction);
bool isValidFDSTypeInstruction(const std::string &instruction);

bool isFInstruction(const uint32_t &instruction);
bool isDInstruction(const uint32_t &instruction);

std::string getExpectedSyntaxes(const std::string &opcode);

} // namespace instruction_set


#endif // INSTRUCTIONS_H
