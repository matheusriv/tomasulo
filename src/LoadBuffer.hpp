#ifndef LOAD_BUFFER_HPP
#define LOAD_BUFFER_HPP

#include <string>
#include <systemc.h>

class LoadBuffer {
public:
    std::string name;
    bool busy;
    sc_uint<32> address;
    int dest_reg; // Registrador de destino
    int inst_id;
    int ready_cycles;

    // Construtores
    LoadBuffer();
    LoadBuffer(std::string n);
};

#endif // LOAD_BUFFER_HPP