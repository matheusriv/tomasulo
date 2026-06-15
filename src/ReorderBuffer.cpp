#include "ReorderBuffer.hpp"

// Construtor padrão
ReorderBuffer::ReorderBuffer() 
    : entry_id(-1), busy(false), instruction(""), state(""), destination(""), value(0), ready(false) {}

// Construtor parametrizado
ReorderBuffer::ReorderBuffer(int id) 
    : entry_id(id), busy(false), instruction(""), state(""), destination(""), value(0), ready(false) {}