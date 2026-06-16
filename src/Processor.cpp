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
        
        registers.resize(32);
        
        // Estações de Reserva (3 Add/Sub, 2 Mult/Div)
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
        adder = new Adder("adder_inst");
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
        multiplier = new Multiplier("mult_inst");
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
    void broadcast_cdb(std::string rs_name, sc_int<32> result);
};

void TomasuloProcessor::broadcast_cdb(std::string rs_name, sc_int<32> result) {
    // Atualiza RS liberando dependências
    for (auto& rs : reservation_stations) {
        if (rs.busy) {
            if (rs.qj == rs_name) { rs.vj = result; rs.qj = ""; }
            if (rs.qk == rs_name) { rs.vk = result; rs.qk = ""; }
        }
    }
    
    // Atualiza LSU liberando dependências (Store aguardando valor para salvar)
    for (auto& lsu : load_store_units) {
        if (lsu.busy && lsu.is_store) {
            if (lsu.qj == rs_name) { lsu.vj = result; lsu.qj = ""; }
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
        
        // Desce os sinais de start do ciclo anterior
        add_start.write(false);
        mult_start.write(false);

        // --- 1. WRITEBACK (CDB Broadcast) ---
        std::string broadcast_rs_name = "";
        sc_int<32> broadcast_val = 0;

        if (mult_done.read() == true) {
            int broadcast_rs_id = mult_rs_id_out.read();
            broadcast_val = mult_result.read();
            broadcast_rs_name = reservation_stations[broadcast_rs_id].name;
            for (auto& inst : inst_history) {
                if (inst.id == reservation_stations[broadcast_rs_id].inst_id) {
                    inst.cycle_execute_end = current_cycle - 1;
                    inst.cycle_writeback = current_cycle;
                }
            }
            reservation_stations[broadcast_rs_id].busy = false;
        } 
        else if (add_done.read() == true) {
            int broadcast_rs_id = add_rs_id_out.read();
            broadcast_val = add_result.read();
            broadcast_rs_name = reservation_stations[broadcast_rs_id].name;
            for (auto& inst : inst_history) {
                if (inst.id == reservation_stations[broadcast_rs_id].inst_id) {
                    inst.cycle_execute_end = current_cycle - 1;
                    inst.cycle_writeback = current_cycle;
                }
            }
            reservation_stations[broadcast_rs_id].busy = false;
        }

        if (broadcast_rs_name != "") {
            broadcast_cdb(broadcast_rs_name, broadcast_val);
        }

        // --- 2. EXECUTE ---
        // Execute LSU
        for (auto& lsu : load_store_units) {
            if (lsu.busy) {
                if (!lsu.is_store) { // Load
                    if (lsu.ready_cycles == 2) {
                        for (auto& inst : inst_history) {
                            if (inst.id == lsu.inst_id) inst.cycle_execute_start = current_cycle;
                        }
                    }
                    if (lsu.ready_cycles == 1) {
                        for (auto& inst : inst_history) {
                            if (inst.id == lsu.inst_id) inst.cycle_execute_end = current_cycle;
                        }
                    }
                    if (lsu.ready_cycles == 0) {
                        sc_int<32> val = memory.read(lsu.address);
                        for (auto& inst : inst_history) {
                            if (inst.id == lsu.inst_id) inst.cycle_writeback = current_cycle;
                        }
                        broadcast_cdb(lsu.name, val);
                        lsu.busy = false;
                    }
                    if (lsu.ready_cycles > 0) lsu.ready_cycles--;
                } else { // Store
                    if (lsu.qj == "") { // Data is ready
                        if (lsu.ready_cycles == 2) {
                            for (auto& inst : inst_history) {
                                if (inst.id == lsu.inst_id) inst.cycle_execute_start = current_cycle;
                            }
                        }
                        if (lsu.ready_cycles == 1) {
                            for (auto& inst : inst_history) {
                                if (inst.id == lsu.inst_id) inst.cycle_execute_end = current_cycle;
                            }
                        }
                        if (lsu.ready_cycles == 0) {
                            memory.write(lsu.address, lsu.vj);
                            for (auto& inst : inst_history) {
                                if (inst.id == lsu.inst_id) inst.cycle_writeback = current_cycle;
                            }
                            lsu.busy = false;
                        }
                        if (lsu.ready_cycles > 0) lsu.ready_cycles--;
                    }
                }
            }
        }

        // atualiza contadores regressivos das estações de reserva em execução
        for (auto& rs : reservation_stations) {
            if (rs.busy && rs.qj == "" && rs.qk == "" && rs.ready_cycles > 0) {
                rs.ready_cycles--;
                if (rs.ready_cycles == 0) {
                    rs.ready_cycles = -1; // Mantém em -1 aguardando o resultado final do hardware
                }
            }
        }

        // Dispatch to Adder
        bool adder_dispatched = false;
        for (size_t i = 0; i < reservation_stations.size(); i++) {
            auto& rs = reservation_stations[i];
            if (rs.busy && rs.qj == "" && rs.qk == "" && rs.ready_cycles == 0 && rs.name.find("Add") != std::string::npos) {
                if (!adder_dispatched) {
                    add_rs_id_in.write(i);
                    add_op1.write(rs.vj);
                    add_op2.write(rs.vk);
                    add_opcode.write((rs.op == "add") ? 0 : 1);
                    add_start.write(true);
                    
                    for (auto& inst : inst_history) {
                        if (inst.id == rs.inst_id) inst.cycle_execute_start = current_cycle + 1;
                    }
                    rs.ready_cycles = 2; // Inicia o countdown (Latência do Adder)
                    adder_dispatched = true;
                }
            }
        }

        // Dispatch to Multiplier
        bool mult_dispatched = false;
        for (size_t i = 0; i < reservation_stations.size(); i++) {
            auto& rs = reservation_stations[i];
            if (rs.busy && rs.qj == "" && rs.qk == "" && rs.ready_cycles == 0 && rs.name.find("Mult") != std::string::npos) {
                if (!mult_dispatched) {
                    mult_rs_id_in.write(i);
                    mult_op1.write(rs.vj);
                    mult_op2.write(rs.vk);
                    mult_opcode.write((rs.op == "mul") ? 0 : 1);
                    mult_start.write(true);
                    
                    for (auto& inst : inst_history) {
                        if (inst.id == rs.inst_id) inst.cycle_execute_start = current_cycle + 1;
                    }
                    rs.ready_cycles = (rs.op == "mul") ? 10 : 40; // Inicia o countdown (Latência do Multiplier)
                    mult_dispatched = true;
                }
            }
        }

        // --- 3. ISSUE ---
        if (!instruction_queue.isEmpty()) {
            Instruction inst = instruction_queue.front();

            if (inst.type == OP_LOGICAL) {
                bool needs_mult = (inst.name == "mul" || inst.name == "div");
                for (size_t i = 0; i < reservation_stations.size(); i++) {
                    auto& rs = reservation_stations[i];
                    bool is_mult_rs = (rs.name.find("Mult") != std::string::npos);
                    
                    if (!rs.busy && (needs_mult == is_mult_rs)) {
                        rs.busy = true;
                        rs.op = inst.name;
                        rs.inst_id = inst.id;
                        rs.dest_reg = inst.rd;
                        rs.ready_cycles = 0; 

                        if (registers[inst.rs].Qi == "") {
                            rs.vj = registers[inst.rs].value;
                            rs.qj = "";
                        } else {
                            rs.qj = registers[inst.rs].Qi;
                        }

                        if (registers[inst.rt].Qi == "") {
                            rs.vk = registers[inst.rt].value;
                            rs.qk = "";
                        } else {
                            rs.qk = registers[inst.rt].Qi;
                        }

                        registers[inst.rd].Qi = rs.name;

                        inst.cycle_issue = current_cycle;
                        inst_history.push_back(inst);
                        instruction_queue.pop_front();
                        break;
                    }
                }
            } else if (inst.type == OP_MEMORY) {
                for (size_t i = 0; i < load_store_units.size(); i++) {
                    auto& lsu = load_store_units[i];
                    bool is_store_lsu = (lsu.name.find("Store") != std::string::npos);
                    bool is_inst_store = (inst.name == "st");
                    
                    if (!lsu.busy && (is_store_lsu == is_inst_store)) {
                        lsu.busy = true;
                        lsu.is_store = is_inst_store;
                        lsu.inst_id = inst.id;
                        lsu.ready_cycles = 2; // Tempo de latência da memória
                        
                        // rs é o registrador de endereço base
                        lsu.address = registers[inst.rs].value + inst.imm;

                        if (is_inst_store) {
                            if (registers[inst.rt].Qi == "") {
                                lsu.vj = registers[inst.rt].value;
                                lsu.qj = "";
                            } else {
                                lsu.qj = registers[inst.rt].Qi;
                            }
                        } else {
                            registers[inst.rt].Qi = lsu.name;
                            lsu.qj = "";
                        }

                        inst.cycle_issue = current_cycle;
                        inst_history.push_back(inst);
                        instruction_queue.pop_front();
                        break;
                    }
                }
            }
        }

        print_status();
        
        bool all_done = instruction_queue.isEmpty();
        if (all_done) {
            for (const auto& rs : reservation_stations) {
                if (rs.busy) { all_done = false; break; }
            }
            for (const auto& lsu : load_store_units) {
                if (lsu.busy) { all_done = false; break; }
            }
        }
        if (all_done) {
            sc_stop();
        }

        wait(); // Avança 1 ciclo de clock
    }
}

// ==========================================
// FUNÇÕES DE IMPRESSÃO
// ==========================================
void TomasuloProcessor::print_status() {
    std::cout << "\n========================================================================\n";
    std::cout << " CLOCK CYCLE: " << current_cycle << "\n";
    std::cout << "========================================================================\n";

    std::cout << "--- RESERVATION STATIONS ---\n";
    std::cout << std::left 
              << std::setw(8) << "Time" 
              << std::setw(8) << "Name" 
              << std::setw(6) << "Busy" 
              << std::setw(8) << "Op" 
              << std::setw(8) << "Vj" 
              << std::setw(8) << "Vk" 
              << std::setw(8) << "Qj" 
              << std::setw(8) << "Qk" 
              << std::setw(6) << "Rj" 
              << std::setw(6) << "Rk" << "\n";
    for (const auto& rs : reservation_stations) {
        std::string time_str = "";
        if (!rs.busy) {
            time_str = "";
        } else if (rs.qj != "" || rs.qk != "") {
            time_str = "-"; // Aguardando operandos
        } else if (rs.ready_cycles > 0) {
            time_str = std::to_string(rs.ready_cycles); // Mostra o countdown regressivo
        } else if (rs.ready_cycles == -1) {
            time_str = "0"; // Execução finalizada, aguardando Barramento (Writeback)
        } else {
            time_str = "-"; // Pronto para despachar neste ciclo
        }

        std::string rj_str = "";
        std::string rk_str = "";
        if (rs.busy) {
            rj_str = (rs.qj == "") ? "yes" : "no";
            rk_str = (rs.qk == "") ? "yes" : "no";
        }

        std::cout << std::left 
                  << std::setw(8) << time_str
                  << std::setw(8) << rs.name 
                  << std::setw(6) << (rs.busy ? "yes" : "no") 
                  << std::setw(8) << (rs.busy ? rs.op : "") 
                  << std::setw(8) << (rs.busy ? std::to_string(rs.vj.to_int()) : "") 
                  << std::setw(8) << (rs.busy ? std::to_string(rs.vk.to_int()) : "") 
                  << std::setw(8) << (rs.busy ? rs.qj : "") 
                  << std::setw(8) << (rs.busy ? rs.qk : "") 
                  << std::setw(6) << rj_str 
                  << std::setw(6) << rk_str << "\n";
    }

    std::cout << "\n--- LOAD/STORE UNITS ---\n";
    std::cout << std::left 
              << std::setw(8) << "Time" 
              << std::setw(8) << "Name" 
              << std::setw(6) << "Busy" 
              << std::setw(10) << "Address" << "\n";
    for (const auto& lsu : load_store_units) {
        std::string time_str = "";
        if (!lsu.busy) {
            time_str = "";
        } else if (lsu.is_store && lsu.qj != "") {
            time_str = "-"; // Store aguardando valor do registrador para salvar
        } else {
            time_str = std::to_string(lsu.ready_cycles);
        }

        std::cout << std::left 
                  << std::setw(8) << time_str
                  << std::setw(8) << lsu.name 
                  << std::setw(6) << (lsu.busy ? "yes" : "no") 
                  << std::setw(10) << (lsu.busy ? std::to_string(lsu.address.to_uint()) : "") << "\n";
    }

    std::cout << "\n--- REGISTERS ---\n";
    for (size_t i = 0; i < registers.size(); i += 2) {
        std::cout << "F" << i << ": ";
        if (registers[i].Qi != "") {
            std::cout << std::left << std::setw(6) << registers[i].Qi;
        } else {
            std::cout << std::left << std::setw(6) << registers[i].value.to_int();
        }
        if (((i / 2) + 1) % 8 == 0) std::cout << "\n";
    }
}

void TomasuloProcessor::print_timing_diagram() {
    std::cout << "\n========================================================================\n";
    std::cout << " FINAL TIMING DIAGRAM (ISSUE | EXECUTE | WRITEBACK)\n";
    std::cout << "========================================================================\n";
    std::cout << std::left 
              << std::setw(10) << "Inst ID" 
              << std::setw(10) << "Name" 
              << std::setw(10) << "Issue" 
              << std::setw(15) << "Execute" 
              << std::setw(10) << "Writeback" << "\n";
              
    for (const auto& inst : inst_history) {
        std::string issue_time = (inst.cycle_issue != -1) ? std::to_string(inst.cycle_issue) : "Pending";
        std::string exec_time = (inst.cycle_execute_start != -1 && inst.cycle_execute_end != -1) ? 
                                (std::to_string(inst.cycle_execute_start) + "-" + std::to_string(inst.cycle_execute_end)) : "Pending";
        std::string wb_time = (inst.cycle_writeback != -1) ? std::to_string(inst.cycle_writeback) : "Pending";

        std::cout << std::left 
                  << std::setw(10) << inst.id 
                  << std::setw(10) << inst.name 
                  << std::setw(10) << issue_time 
                  << std::setw(15) << exec_time 
                  << std::setw(10) << wb_time << "\n";
    }
}

// ==========================================
// FUNÇÃO AUXILIAR DE IMPRESSÃO
// ==========================================
void print_loaded_instructions(InstructionQueue temp_queue) {
    std::cout << "\n--- INSTRUCTIONS LOADED ---\n";
    std::cout << std::left << std::setw(6) << "ID" 
              << std::setw(10) << "Name"
              << std::setw(6) << "RD"
              << std::setw(6) << "RS"
              << std::setw(6) << "RT"
              << std::setw(8) << "IMM" << "\n";
              
    while (!temp_queue.isEmpty()) {
        Instruction inst = temp_queue.pop_front();
        std::cout << std::left << std::setw(6) << inst.id 
                  << std::setw(10) << inst.name
                  << std::setw(6) << inst.rd.to_int()
                  << std::setw(6) << inst.rs.to_int()
                  << std::setw(6) << inst.rt.to_int()
                  << std::setw(8) << inst.imm.to_int() << "\n";
    }
}

// ==========================================
// TESTBENCH PRINCIPAL
// ==========================================
int sc_main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "\nError! Usage: " << argv[0] << " <instructions_file.txt>\n";
        return 1;
    }

    sc_clock clk("clk", 10, SC_NS);

    TomasuloProcessor proc("tomasulo_proc");
    proc.clk(clk);
    
    InstructionQueue inst_queue;
    
    // carrega as instruções e escreve os dados diretamente na memória do processador
    load_program(argv[1], inst_queue, proc.memory.get_raw_data());
    // alimenta a fila de instruções no processador
    proc.instruction_queue = inst_queue;
    //print_loaded_instructions(inst_queue);

    std::cout << "\n === Simulation started! ===\n";
    std::cout << "-> Press [ENTER] to advance 1 clock cycle.\n";
    std::cout << "-> Type 'c' and press [ENTER] to run to completion (Auto-Run).\n";

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