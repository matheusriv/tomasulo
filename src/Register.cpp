#include "Register.hpp"

Register::Register() : value(0), rob_id(-1) {} // Inicializa apontando para lugar nenhum no ROB

// Construtor com valor inicial
Register::Register(sc_int<32> initial_value) : value(initial_value), rob_id(-1) {} // Inicializa apontando para lugar nenhum no ROB