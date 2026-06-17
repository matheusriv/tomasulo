#ifndef RESERVATION_STATION_HPP
#define RESERVATION_STATION_HPP

#include <string>
#include <systemc.h>

// Estruturas de Status do Tomasulo
class ReservationStation {
public:
    std::string name;
    bool busy;
    std::string op;
    sc_int<32> vj, vk;
    int qj_rob;
    int qk_rob;
    int dest_rob;
    int inst_id;
    int ready_cycles; // Contagem regressiva de execução

    // Construtores
    ReservationStation();
    ReservationStation(std::string n);
};

#endif // RESERVATION_STATION_HPP