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
    sc_int<32> vj;
    int qj_rob; // -1 se o valor já estiver pronto (operand do store)
    int dest_rob;
    int inst_id;
    int ready_cycles;

public:
    // Construtores
    LoadStoreUnit();
    LoadStoreUnit(std::string n);
};

#endif // LOAD_STORE_UNIT_HPP