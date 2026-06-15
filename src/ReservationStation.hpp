#ifndef RESERVATION_STATION_HPP
#define RESERVATION_STATION_HPP

#include <string>

// Estruturas de Status do Tomasulo
class ReservationStation {
public:
    std::string name;
    bool busy;
    std::string op;
    int vj, vk;
    std::string qj, qk;
    int dest_reg;
    int inst_id;
    int ready_cycles; // Contagem regressiva de execução

    // Construtores
    ReservationStation();
    ReservationStation(std::string n);
};

#endif // RESERVATION_STATION_HPP