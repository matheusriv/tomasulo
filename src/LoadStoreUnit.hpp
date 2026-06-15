#ifndef LOAD_STORE_UNIT_HPP
#define LOAD_STORE_UNIT_HPP

#include <string>

class LoadStoreUnit {
public:
    std::string name;
    bool busy;
    bool is_store;
    int address;
    std::string qj; // Para armazenar o valor a ser guardado (se vier de outra instrução)
    int vj;
    int inst_id;
    int ready_cycles;

public:
    // Construtores
    LoadStoreUnit();
    LoadStoreUnit(std::string n);
};

#endif // LOAD_STORE_UNIT_HPP