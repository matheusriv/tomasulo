#ifndef ADDER_HPP
#define ADDER_HPP

#include <systemc.h>

SC_MODULE(Adder) {
    sc_in<bool> clk;
    sc_in<bool> start;          // Sinal para iniciar a execução
    sc_in<int> rs_id_in;        // ID (índice) da Estação de Reserva que enviou a instrução
    sc_in<sc_int<32>> op1;      // Operando 1 (Vj)
    sc_in<sc_int<32>> op2;      // Operando 2 (Vk)
    sc_in<int> opcode;          // 0 para ADD, 1 para SUB (pode ser ajustado)
    
    sc_out<sc_int<32>> result;  // Resultado da operação
    sc_out<int> rs_id_out;      // ID da Estação de Reserva sendo concluída
    sc_out<bool> done;          // Sinaliza que a execução terminou (avisa o CDB)

    int latency;  // Tempo de latência

    // Processo que simula a operação
    void compute();

    SC_HAS_PROCESS(Adder);
    Adder(sc_module_name name, int lat = 2) : sc_module(name), latency(lat) {
        SC_CTHREAD(compute, clk.pos()); // Thread sensível à borda de subida do clock
    }
};

#endif // ADDER_HPP