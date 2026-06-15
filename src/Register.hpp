#ifndef REGISTER_HPP
#define REGISTER_HPP

#include <string>
#include <systemc.h>

class Register {
public:
    sc_int<32> value;
    std::string Qi; // Nome da estação de reserva que produzirá o valor, "" se estiver pronto

    // Construtores
    Register();
    Register(sc_int<32> initial_value);
};

#endif // REGISTER_HPP