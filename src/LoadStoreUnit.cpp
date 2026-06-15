#include "LoadStoreUnit.hpp"

// Construtor padrão
LoadStoreUnit::LoadStoreUnit() 
    : name(""), busy(false), is_store(false), address(0), qj(""), vj(0), inst_id(-1), ready_cycles(0) {}

// Construtor parametrizado
LoadStoreUnit::LoadStoreUnit(std::string n) 
    : name(n), busy(false), is_store(false), address(0), qj(""), vj(0), inst_id(-1), ready_cycles(0) {}
