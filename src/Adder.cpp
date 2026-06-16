#include "Adder.hpp"

void Adder::compute() {
    // Inicializa as saídas
    done.write(false);
    result.write(0);
    rs_id_out.write(-1);

    while (true) {
        if (start.read() == true) {
            // Lê as portas de entrada
            int id = rs_id_in.read();
            sc_int<32> val1 = op1.read();
            sc_int<32> val2 = op2.read();
            int op = opcode.read();

            // Simula a latência de hardware configurada
            if (latency > 1) wait(latency - 1);
            
            if (op == 0) { // Exemplo: 0 = ADD
                result.write(val1 + val2);
            } else {       // Exemplo: 1 = SUB
                result.write(val1 - val2);
            }
            
            rs_id_out.write(id);
            done.write(true); // Levanta a flag indicando que finalizou
            
            // Espera 1 ciclo para que o módulo do Tomasulo leia o resultado via CDB
            wait(); 
        } else {
            // Mantém "done" em falso e avança um ciclo de clock enquanto estiver ocioso
            done.write(false);
            rs_id_out.write(-1);
            wait();
        }
    }
}