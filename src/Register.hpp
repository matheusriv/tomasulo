#ifndef REGISTER_HPP
#define REGISTER_HPP

#include <string>
#include <systemc.h>

class Register {
public:
    sc_int<32> value;
    int rob_id; // -1 se estiver pronto, senão ID no Reorder Buffer que produzirá o valor

    // Construtores
    Register();
    Register(sc_int<32> initial_value);
};

#endif // REGISTER_HPP