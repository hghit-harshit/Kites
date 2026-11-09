#pragma once
#include <string>
#include <vector>

enum class InstrFormat
{
    R,
    I,
    L,
    S,
    B,
    U,
    J,
    Unknown
};

struct parsed_instruction
{
    InstrFormat format = InstrFormat::Unknown;
    std::string inst;
    int rs1 = -1, rs2 = -1, rd = -1;
    int imm = -1;
    int memory_add_offset = -1;
    int offset = -1;
    int index;
    int index_in_block = -1;
};

class parser
{
    std::string hex_code_text;

  public:
    explicit parser(std::string s);
    parsed_instruction decoder(std::string s);
    std::vector<parsed_instruction> instructions();
};