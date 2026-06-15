#include "LoadBuffer.hpp"

LoadBuffer::LoadBuffer() 
    : name(""), busy(false), address(0), dest_reg(-1), inst_id(-1), ready_cycles(0) {}

LoadBuffer::LoadBuffer(std::string n) 
    : name(n), busy(false), address(0), dest_reg(-1), inst_id(-1), ready_cycles(0) {}