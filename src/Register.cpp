#include "Register.hpp"

Register::Register() : value(0), Qi("") {}

// Construtor com valor inicial
Register::Register(sc_int<32> initial_value) : value(initial_value), Qi("") {}