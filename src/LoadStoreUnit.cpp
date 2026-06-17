#include "LoadStoreUnit.hpp"

LoadStoreUnit::LoadStoreUnit() 
    : name(""), busy(false), is_store(false), address(0), vj(0), qj_rob(-1), dest_rob(-1), inst_id(-1), ready_cycles(0) {}

LoadStoreUnit::LoadStoreUnit(std::string n) 
    : name(n), busy(false), is_store(false), address(0), vj(0), qj_rob(-1), dest_rob(-1), inst_id(-1), ready_cycles(0) {}
