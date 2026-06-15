#include "Memory.hpp"
#include <stdexcept>

Memory::Memory(int size) {
    data.resize(size, 0); // Inicializa com zeros
}

sc_int<32> Memory::read(int address) const {
    if (address < 0 || (size_t)address >= data.size()) {
        throw std::out_of_range("Memory read out of bounds!");
    }
    return data[address];
}

void Memory::write(int address, sc_int<32> value) {
    if (address < 0 || (size_t)address >= data.size()) {
        throw std::out_of_range("Memory write out of bounds!");
    }
    data[address] = value;
}

sc_int<32>* Memory::get_raw_data() {
    return data.data();
}