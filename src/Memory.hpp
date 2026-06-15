#ifndef MEMORY_HPP
#define MEMORY_HPP

#include <vector>
#include <systemc.h>

class Memory {
public:
    std::vector<sc_int<32>> data;

    Memory(int size = 1024);

    sc_int<32> read(int address) const;
    void write(int address, sc_int<32> value);

    sc_int<32>* get_raw_data();
};

#endif // MEMORY_HPP