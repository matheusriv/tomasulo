#ifndef LOADER_HPP
#define LOADER_HPP

#include <string>
#include <systemc.h>
#include "../instruction.hpp"
#include "../InstructionQueue.hpp"

void load_program(const std::string& filepath, InstructionQueue& instruction_queue, sc_int<32>* mem_dados);

#endif // LOADER_HPP