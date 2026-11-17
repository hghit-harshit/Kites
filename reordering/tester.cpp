#include "DAG.h"
#include "parser.h"
#include "scheduler.h"
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
    std::vector<parsed_instruction> instr = p.instructions();

    bool forwarding = 1;
    DAG dag(instr, forwarding);

    scheduler s(dag.blocks(), dag.depend_matrix_full(), forwarding);

    int before = 0;
    int after = 0;

    std::vector<parsed_instruction> reordered_code = s.code_scheduler(&before, &after);

    printf("cycles before = %d\n",before);

    printf("with forwarding cycles after reoredering  = %d\n", after);
    forwarding = 0;
    after = 0;
    before  =0;
    s = scheduler(dag.blocks(),dag.depend_matrix_full(),forwarding);
    reordered_code = s.code_scheduler(&before, &after);
    printf("cycles before = %d\n",before);
    printf("without forwarding cycles after reoredering  = %d", after);

    return 0;
}