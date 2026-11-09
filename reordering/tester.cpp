#include "parser.h"
#include <cstdio>

int main()
{


    std::string hexcode =
        "0x10000093 0x20000113 0x0000a183 0x0040a203 0x004182b3 0x0080a303 0x405303b3 0x00c0a403 "
        "0x003444b3 0x00748513 0x008575b3 0x0075e633 0x003616b3 0x0046d713 0x00e12023 0x00c707b3 "
        "0x00f3a833 0x00f12223 0x06480893 0x41088933 0x00391993 0x01312423 0x0100aa03 0x013a0ab3 "
        "0x0ffafb13 0x100b6b93 0x015bcc33 0x01812623 0x402c5c93 0x018cbd33 0x01ac8db3 0x417d8e33 "
        "0x01c12823 0x0d1e0e93 0x01beef33 0x01e12a23 ";

    parser p(hexcode);

    std::vector<parsed_instruction> mewo = p.instructions();

    for (auto it : mewo)
    {
        printf("%i rs1 =  %d rs2 = %d  rd = %d imm = %d mem_off = %d off = %d \n", it.format,
               it.rs1, it.rs2, it.rd, it.imm, it.memory_add_offset, it.offset);
    }

    return 0;
}