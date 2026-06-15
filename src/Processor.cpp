#include <systemc.h>
#include <iostream>
#include <vector>
#include <string>
#include <iomanip>
#include <algorithm>
#include "InstructionQueue.hpp"
#include "Memory.hpp"
#include "Register.hpp"
#include "ReservationStation.hpp"
#include "LoadStoreUnit.hpp"
#include "Adder.hpp"
#include "Multiplier.hpp"
#include "loader/loader.hpp"

// ==========================================
// MÓDULO PRINCIPAL DO TOMASULO
// ==========================================
SC_MODULE(TomasuloProcessor) {
    sc_in<bool> clk;

    // Estruturas de dados internas
    InstructionQueue instruction_queue;
    Memory memory;
    std::vector<Register> registers;
    std::vector<ReservationStation> reservation_stations;
    std::vector<LoadStoreUnit> load_store_units;
    
    // Histórico de instruções para o diagrama de tempo
    std::vector<Instruction> inst_history;

    // Sinais do CDB para Adder
    sc_signal<bool> add_start;
    sc_signal<int> add_rs_id_in;
    sc_signal<sc_int<32>> add_op1;
    sc_signal<sc_int<32>> add_op2;
    sc_signal<int> add_opcode;
    sc_signal<sc_int<32>> add_result;
    sc_signal<int> add_rs_id_out;
    sc_signal<bool> add_done;

    // Sinais do CDB para Multiplier
    sc_signal<bool> mult_start;
    sc_signal<int> mult_rs_id_in;
    sc_signal<sc_int<32>> mult_op1;
    sc_signal<sc_int<32>> mult_op2;
    sc_signal<int> mult_opcode;
    sc_signal<sc_int<32>> mult_result;
    sc_signal<int> mult_rs_id_out;
    sc_signal<bool> mult_done;

    // Instâncias das ALUs
    Adder* adder;
    Multiplier* multiplier;

    int current_cycle;

    SC_HAS_PROCESS(TomasuloProcessor);
    TomasuloProcessor(sc_module_name name) : sc_module(name), memory(1024) {
        current_cycle = 0;
        
        // Inicializar estruturas (32 Registradores)
        registers.resize(32);
        
        // Estações de Reserva (ex: 3 Add/Sub, 2 Mult/Div)
        reservation_stations.push_back(ReservationStation("Add1"));
        reservation_stations.push_back(ReservationStation("Add2"));
        reservation_stations.push_back(ReservationStation("Add3"));
        reservation_stations.push_back(ReservationStation("Mult1"));
        reservation_stations.push_back(ReservationStation("Mult2"));

        // Load/Store Units
        load_store_units.push_back(LoadStoreUnit("Load1"));
        load_store_units.push_back(LoadStoreUnit("Load2"));
        load_store_units.push_back(LoadStoreUnit("Store1"));

        // Instanciar Adder
        adder = new Adder("adder_inst", 2); // 2 Ciclos de latência
        adder->clk(clk);
        adder->start(add_start);
        adder->rs_id_in(add_rs_id_in);
        adder->op1(add_op1);
        adder->op2(add_op2);
        adder->opcode(add_opcode);
        adder->result(add_result);
        adder->rs_id_out(add_rs_id_out);
        adder->done(add_done);

        // Instanciar Multiplier
        multiplier = new Multiplier("mult_inst", 10); // 10 Ciclos de latência
        multiplier->clk(clk);
        multiplier->start(mult_start);
        multiplier->rs_id_in(mult_rs_id_in);
        multiplier->op1(mult_op1);
        multiplier->op2(mult_op2);
        multiplier->opcode(mult_opcode);
        multiplier->result(mult_result);
        multiplier->rs_id_out(mult_rs_id_out);
        multiplier->done(mult_done);

        SC_CTHREAD(run, clk.pos()); // Dispara a thead com a borda de subida
    }

    ~TomasuloProcessor() {
        delete adder;
        delete multiplier;
    }

    void run();
    void print_status();
    void print_timing_diagram();
    void broadcast_cdb(int rs_id, sc_int<32> result);
};

void TomasuloProcessor::broadcast_cdb(int rs_id, sc_int<32> result) {
    std::string rs_name = reservation_stations[rs_id].name;
    
    // Atualiza RS liberando dependências
    for (auto& rs : reservation_stations) {
        if (rs.busy) {
            if (rs.qj == rs_name) { rs.vj = result; rs.qj = ""; }
            if (rs.qk == rs_name) { rs.vk = result; rs.qk = ""; }
        }
    }
    
    // Atualiza Registradores
    for (auto& reg : registers) {
        if (reg.Qi == rs_name) {
            reg.value = result;
            reg.Qi = "";
        }
    }
}

void TomasuloProcessor::run() {
    add_start.write(false);
    mult_start.write(false);

    while (true) {
        current_cycle++;
        
        // --- 1. WRITEBACK (CDB Broadcast) ---
        int broadcast_rs_id = -1;
        sc_int<32> broadcast_val = 0;

        if (mult_done.read() == true) {
            broadcast_rs_id = mult_rs_id_out.read();
            broadcast_val = mult_result.read();
            for (auto& inst : inst_history) {
                if (inst.id == reservation_stations[broadcast_rs_id].inst_id) {
                    inst.cycle_writeback = current_cycle;
                }
            }
            reservation_stations[broadcast_rs_id].busy = false;
        } 
        else if (add_done.read() == true) {
            broadcast_rs_id = add_rs_id_out.read();
            broadcast_val = add_result.read();
            for (auto& inst : inst_history) {
                if (inst.id == reservation_stations[broadcast_rs_id].inst_id) {
                    inst.cycle_writeback = current_cycle;
                }
            }
            reservation_stations[broadcast_rs_id].busy = false;
        }

        if (broadcast_rs_id != -1) {
            broadcast_cdb(broadcast_rs_id, broadcast_val);
        }

        // --- 2. EXECUTE ---
        // TODO: Inserir a lógica de verificação de Qj/Qk vazios e despacho para Adder/Multiplier

        // --- 3. ISSUE ---
        // TODO: Inserir a lógica de pop_front da instruction_queue e mapeamento nas Reservation Stations

        // Imprime o status do ciclo
        print_status();
        
        // Parada de segurança caso as instruções acabem (Após 50 ciclos ociosos encerra)
        if (instruction_queue.isEmpty() && current_cycle > 50) {
            sc_stop();
        }

        wait(); // Avança 1 ciclo de clock
    }
}

// ==========================================
// FUNÇÕES DE IMPRESSÃO / RELATÓRIO
// ==========================================
void TomasuloProcessor::print_status() {
    std::cout << "\n========================================================================\n";
    std::cout << " CICLO DE CLOCK: " << current_cycle << "\n";
    std::cout << "========================================================================\n";

    std::cout << "--- ESTAÇÕES DE RESERVA ---\n";
    std::cout << std::left 
              << std::setw(8) << "Time" 
              << std::setw(8) << "Name" 
              << std::setw(6) << "Busy" 
              << std::setw(8) << "Op" 
              << std::setw(8) << "Vj" 
              << std::setw(8) << "Vk" 
              << std::setw(8) << "Qj" 
              << std::setw(8) << "Qk" << "\n";
    for (const auto& rs : reservation_stations) {
        std::cout << std::left 
                  << std::setw(8) << rs.ready_cycles 
                  << std::setw(8) << rs.name 
                  << std::setw(6) << (rs.busy ? "yes" : "no") 
                  << std::setw(8) << rs.op 
                  << std::setw(8) << rs.vj.to_int() 
                  << std::setw(8) << rs.vk.to_int() 
                  << std::setw(8) << rs.qj 
                  << std::setw(8) << rs.qk << "\n";
    }

    std::cout << "\n--- LOAD/STORE UNITS ---\n";
    std::cout << std::left 
              << std::setw(8) << "Name" 
              << std::setw(6) << "Busy" 
              << std::setw(10) << "Address" << "\n";
    for (const auto& lsu : load_store_units) {
        std::cout << std::left 
                  << std::setw(8) << lsu.name 
                  << std::setw(6) << (lsu.busy ? "yes" : "no") 
                  << std::setw(10) << lsu.address.to_uint() << "\n";
    }

    std::cout << "\n--- REGISTRADORES ---\n";
    for (size_t i = 0; i < registers.size(); i++) {
        std::cout << "R" << i << ": ";
        if (registers[i].Qi != "") {
            std::cout << std::left << std::setw(6) << registers[i].Qi;
        } else {
            std::cout << std::left << std::setw(6) << registers[i].value.to_int();
        }
        if ((i + 1) % 8 == 0) std::cout << "\n";
    }
}

void TomasuloProcessor::print_timing_diagram() {
    std::cout << "\n========================================================================\n";
    std::cout << " DIAGRAMA DE TEMPO FINAL (ISSUE | EXECUTE | WRITEBACK)\n";
    std::cout << "========================================================================\n";
    std::cout << std::left 
              << std::setw(10) << "Inst ID" 
              << std::setw(10) << "Name" 
              << std::setw(10) << "Issue" 
              << std::setw(15) << "Execute" 
              << std::setw(10) << "Writeback" << "\n";
              
    for (const auto& inst : inst_history) {
        std::string exec_time = std::to_string(inst.cycle_execute_start) + "-" + std::to_string(inst.cycle_execute_end);
        std::cout << std::left 
                  << std::setw(10) << inst.id 
                  << std::setw(10) << inst.name 
                  << std::setw(10) << inst.cycle_issue 
                  << std::setw(15) << exec_time 
                  << std::setw(10) << inst.cycle_writeback << "\n";
    }
}

// ==========================================
// TESTBENCH PRINCIPAL
// ==========================================
int sc_main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Uso: " << argv[0] << " <arquivo_instrucoes.txt>\n";
        return 1;
    }

    InstructionQueue inst_queue;
    sc_int<32> mem_dados[1024] = {0}; 
    
    // Carrega instruções do arquivo passado por argumento no terminal
    load_program(argv[1], inst_queue, mem_dados);

    // Definição do Clock principal
    sc_clock clk("clk", 10, SC_NS);

    // Instancia o núcleo de processamento e liga o Clock
    TomasuloProcessor proc("tomasulo_proc");
    proc.clk(clk);
    
    // Alimenta as classes do processador com os dados carregados
    proc.instruction_queue = inst_queue;
    for(int i=0; i<1024; i++) {
        proc.memory.write(i, mem_dados[i]);
    }

    std::cout << "Simulação Iniciada!\n";
    std::cout << "-> Pressione [ENTER] para avançar 1 ciclo de clock.\n";
    std::cout << "-> Digite 'c' e pressione [ENTER] para rodar ate o fim (Auto-Run).\n";

    bool auto_run = false;
    while (sc_get_status() != SC_STOPPED) {
        if (!auto_run) {
            std::string input;
            std::getline(std::cin, input);
            if (input == "c" || input == "C") {
                auto_run = true;
                sc_start(); // Roda continuamente até sc_stop() ser chamado
                break;
            } else {
                sc_start(10, SC_NS); // Avança exatamente 1 ciclo de clock
            }
        }
    }

    proc.print_timing_diagram(); // Exibe a tabela de estatísticas após a simulação
    
    return 0;
}