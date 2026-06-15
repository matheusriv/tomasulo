#include "ReservationStation.hpp"

// Construtor padrão
ReservationStation::ReservationStation() 
    : name(""), busy(false), op(""), vj(0), vk(0), qj(""), qk(""), dest_reg(-1), inst_id(-1), ready_cycles(0) {}

// Construtor parametrizado
ReservationStation::ReservationStation(std::string n) 
    : name(n), busy(false), op(""), vj(0), vk(0), qj(""), qk(""), dest_reg(-1), inst_id(-1), ready_cycles(0) {}