#ifndef REORDER_BUFFER_HPP
#define REORDER_BUFFER_HPP

#include <string>
#include <systemc.h>

class ReorderBuffer {
public:
    int entry_id;
    bool busy;
    std::string instruction;
    std::string state; // Issue, Execute, Write Result, Commit
    std::string destination;
    sc_int<32> value;
    bool ready;

    // Construtores
    ReorderBuffer();
    ReorderBuffer(int id);
};

#endif // REORDER_BUFFER_HPP