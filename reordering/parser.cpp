#include "parser.h"
#include <sstream>
#include <stdexcept>
using namespace std;

// helper functions

static int sext(const string &bits)
{
    int v = stoi(bits, nullptr, 2);
    return (bits[0] == '1') ? v - (1 << bits.size()) : v;
}

static std::string hexCharToBinary(char hex)
{
    switch (std::toupper(hex))
    {
    case '0':
        return "0000";
    case '1':
        return "0001";
    case '2':
        return "0010";
    case '3':
        return "0011";
    case '4':
        return "0100";
    case '5':
        return "0101";
    case '6':
        return "0110";
    case '7':
        return "0111";
    case '8':
        return "1000";
    case '9':
        return "1001";
    case 'A':
        return "1010";
    case 'B':
        return "1011";
    case 'C':
        return "1100";
    case 'D':
        return "1101";
    case 'E':
        return "1110";
    case 'F':
        return "1111";
    default:
        return ""; // Handles invalid characters
    }
}

static std::string hexToBinary(const std::string &hexStr)
{
    std::string binaryStr;
    binaryStr.reserve(hexStr.length() * 4);

    for (size_t i = 2; i < hexStr.size(); ++i) // skip "0x"
    {
        std::string chunk = hexCharToBinary(hexStr[i]);
        if (chunk.empty())
            throw std::invalid_argument("Invalid hex character");
        binaryStr += chunk;
    }
    return binaryStr; // 32 chars for an 8-digit word
}

static InstrFormat format_classifier(int opcode)
{
    switch (opcode)
    {

    case 0b0110011:
        return InstrFormat::R;

    case 0b0010011:
        return InstrFormat::I;

    case 0b0000011:
        return InstrFormat::L;

    case 0b0100011:
        return InstrFormat::S;

    case 0b1100011:
        return InstrFormat::B;

    case 0b1101111:
        return InstrFormat::J;

    case 0b0110111:
        return InstrFormat::U;

    case 0b1100111:
        return InstrFormat::I;

    default:
        return InstrFormat::Unknown;
    }
}

static vector<int> operand_number_giver(string binary_code, InstrFormat type)
{
    vector<int> registers(3, -1);
    switch (type)
    {
    case InstrFormat::R:
    {
        registers[0] = stoi(binary_code.substr(7, 5), nullptr, 2);  // this is rs2
        registers[1] = stoi(binary_code.substr(12, 5), nullptr, 2); // this is rs1
        registers[2] = stoi(binary_code.substr(20, 5), nullptr, 2); // this is rd
        break;
    }
    case InstrFormat::I:
    {
        registers[0] = sext(binary_code.substr(0, 12));             // first corresponds to imm
        registers[1] = stoi(binary_code.substr(12, 5), nullptr, 2); // this is rs1
        registers[2] = stoi(binary_code.substr(20, 5), nullptr, 2); // thi is rd
        break;
    }
    case InstrFormat::S:
    {
        registers[0] = sext(binary_code.substr(0, 7) + binary_code.substr(20, 5)); // this is imm
        registers[1] = stoi(binary_code.substr(7, 5), nullptr, 2);                 // this is rs2
        registers[2] = stoi(binary_code.substr(12, 5), nullptr, 2);                // thsi is rs1
        break;
    }
    case InstrFormat::B:
    {
        registers[0] =
            sext(binary_code.substr(0, 1) + binary_code.substr(24, 1) + binary_code.substr(1, 6) +
                 binary_code.substr(20, 4) + "0");                  // this is imm
        registers[1] = stoi(binary_code.substr(7, 5), nullptr, 2);  // this is rs2
        registers[2] = stoi(binary_code.substr(12, 5), nullptr, 2); // thsi is rs1
        break;
    }
    case InstrFormat::U:
    {
        registers[0] = sext(binary_code.substr(0, 20) + "000000000000"); // this is imm

        registers[1] = stoi(binary_code.substr(20, 5), nullptr, 2); // thsi is rd
        break;
    }
    case InstrFormat::J:
    {
        registers[0] = sext(binary_code.substr(0, 1) + binary_code.substr(12, 8) +
                            binary_code.substr(11, 1) + binary_code.substr(1, 10) + "0");
        registers[1] = stoi(binary_code.substr(20, 5), nullptr, 2); // this is rd
        break;
    }
    case InstrFormat::L:
        registers[0] = sext(binary_code.substr(0, 12));             // first corresponds to imm
        registers[1] = stoi(binary_code.substr(12, 5), nullptr, 2); // this is rs1
        registers[2] = stoi(binary_code.substr(20, 5), nullptr, 2); // thi is rd
        break;
    }

    return registers;
}

parser::parser(string s) : hex_code_text(std::move(s))
{
}

parsed_instruction parser::decoder(string s)
{
    int opcode = stoi(s.substr(25, 7), nullptr, 2);
    parsed_instruction inst;
    inst.format = format_classifier(opcode);
    inst.inst = s;
    vector<int> Registers = operand_number_giver(s, inst.format);
    switch (inst.format)
    {
    case InstrFormat::R:
    {
        inst.rs1 = Registers[1];
        inst.rs2 = Registers[0];
        inst.rd = Registers[2];
        break;
    }
    case InstrFormat::I:
    {
        inst.rs1 = Registers[1];
        inst.imm = Registers[0];
        inst.rd = Registers[2];
        break;
    }
    case InstrFormat::L:
    {
        inst.rs1 = Registers[1];
        inst.imm = Registers[0];
        inst.rd = Registers[2];
        break;
    }
    case InstrFormat::B:
    {
        inst.rs1 = Registers[2];
        inst.rs2 = Registers[1];
        inst.memory_add_offset = Registers[0];
        break;
    }

    case InstrFormat::S:
    {
        inst.rs1 = Registers[2];
        inst.offset = Registers[0];
        inst.rs2 = Registers[1];
        break;
    }
    case InstrFormat::U:
    {

        inst.imm = Registers[0];
        inst.rd = Registers[1];
        break;
    }
    case InstrFormat::J:
    {

        inst.rd = Registers[1];
        inst.memory_add_offset = Registers[0];
        break;
    }
    }

    return inst;
}

vector<parsed_instruction> parser::instructions()
{
    stringstream ss(hex_code_text);
    string word;

    vector<parsed_instruction> ans;

    while (ss >> word)
    {
        string binary_code = hexToBinary(word);

        ans.push_back(decoder(binary_code));
    }

    for (int i = 0; i < ans.size(); i++)
    {
        ans[i].index = i;
    }

    return ans;
}
