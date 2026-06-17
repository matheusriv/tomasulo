#include "ReservationStation.hpp"

ReservationStation::ReservationStation() 
    : name(""), busy(false), op(""), vj(0), vk(0), qj_rob(-1), qk_rob(-1), dest_rob(-1), inst_id(-1), ready_cycles(0) {}

ReservationStation::ReservationStation(std::string n) 
    : name(n), busy(false), op(""), vj(0), vk(0), qj_rob(-1), qk_rob(-1), dest_rob(-1), inst_id(-1), ready_cycles(0) {}