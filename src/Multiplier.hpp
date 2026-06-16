#ifndef MULTIPLIER_HPP
#define MULTIPLIER_HPP

#include <systemc.h>

SC_MODULE(Multiplier) {
    sc_in<bool> clk;
    sc_in<bool> start;
    sc_in<int> rs_id_in;
    sc_in<sc_int<32>> op1;
    sc_in<sc_int<32>> op2;
    sc_in<int> opcode;       // 0 para MULT, 1 para DIV
    
    sc_out<sc_int<32>> result;
    sc_out<int> rs_id_out;
    sc_out<bool> done;

    int mult_latency;     // Tempo de latência para multiplicação
    int div_latency;      // Tempo de latência para divisão

    // Processo que simula a operação
    void compute();

    SC_HAS_PROCESS(Multiplier);
    Multiplier(sc_module_name name, int m_lat = 10, int d_lat = 40) : sc_module(name), mult_latency(m_lat), div_latency(d_lat) {
        SC_CTHREAD(compute, clk.pos());
    }
};

#endif // MULTIPLIER_HPP