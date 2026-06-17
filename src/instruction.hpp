#ifndef INSTRUCTION_HPP
#define INSTRUCTION_HPP

#include <string>
#include <systemc.h>

enum OpType {
    OP_LOGICAL,
    OP_JUMP,
    OP_MEMORY,
    OP_IMMEDIATE,
    OP_UNKNOWN
};

class Instruction {
public:
    int id;
    std::string name;
    OpType type;
    sc_int<32> rd, rs, rt;
    sc_int<32> imm;

    // Controle do Diagrama de Tempos
    int cycle_issue = -1;
    int cycle_execute_start = -1;
    int cycle_execute_end = -1;
    int cycle_writeback = -1;
    int cycle_commit = -1;
};

#endif // INSTRUCTION_HPP