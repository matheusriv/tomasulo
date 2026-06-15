#ifndef LOAD_STORE_UNIT_HPP
#define LOAD_STORE_UNIT_HPP

#include <string>
#include <systemc.h>

class LoadStoreUnit {
public:
    std::string name;
    bool busy;
    bool is_store;
    sc_uint<32> address;
    std::string qj; // Para armazenar o valor a ser guardado (se vier de outra instrução)
    sc_int<32> vj;
    int inst_id;
    int ready_cycles;

public:
    // Construtores
    LoadStoreUnit();
    LoadStoreUnit(std::string n);
};

#endif // LOAD_STORE_UNIT_HPP