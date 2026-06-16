#include "Multiplier.hpp"

void Multiplier::compute() {
    // Inicializa as saídas
    done.write(false);
    result.write(0);
    rs_id_out.write(-1);

    while (true) {
        if (start.read() == true) {
            int id = rs_id_in.read();
            sc_int<32> val1 = op1.read();
            sc_int<32> val2 = op2.read();
            int op = opcode.read();

            if (op == 0) { // Exemplo: 0 = MUL
                if (mult_latency > 1) wait(mult_latency - 1);
                result.write(val1 * val2);
            } else {       // Exemplo: 1 = DIV
                if (div_latency > 1) wait(div_latency - 1);
                if (val2 != 0) {
                    result.write(val1 / val2);
                } else {
                    result.write(0); // Tratamento simples para divisão por zero
                }
            }
            
            rs_id_out.write(id);
            done.write(true);
            
            wait();
        } else {
            done.write(false);
            rs_id_out.write(-1);
            wait();
        }
    }
}