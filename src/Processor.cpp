#include <systemc.h>
#include <iostream>
#include <vector>
#include <string>
#include <iomanip>
#include <algorithm>

// ==========================================
// MÓDULO PRINCIPAL DO TOMASULO
// ==========================================
SC_MODULE(TomasuloProcessor) {

}

// ==========================================
// FUNÇÕES DE IMPRESSÃO / RELATÓRIO
// ==========================================
void TomasuloProcessor::print_status() {
    std::cout << "\n========================================================================\n";
    std::cout << " CICLO DE CLOCK: " << "\n";
    std::cout << "========================================================================\n";
    
}

void TomasuloProcessor::print_timing_diagram() {
    std::cout << "\n========================================================================\n";
    std::cout << " DIAGRAMA DE TEMPO FINAL (ISSUE | EXECUTE | WRITEBACK)\n";
    std::cout << "========================================================================\n";
}

// ==========================================
// TESTBENCH PRINCIPAL
// ==========================================
int sc_main(int argc, char* argv[]) {

    std::cout << "Simulação Iniciada.\n";
    sc_start();
    
    return 0;
}