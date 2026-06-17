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
#include "ReorderBuffer.hpp"

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
    
    // Reorder Buffer
    int rob_head;
    int rob_tail;
    int rob_capacity;
    std::vector<ReorderBuffer> reorder_buffer;

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
        
        rob_head = 0;
        rob_tail = 0;
        rob_capacity = 16;
        reorder_buffer.resize(rob_capacity);
        for(int i = 0; i < rob_capacity; i++) {
            reorder_buffer[i].entry_id = i;
        }

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
    void broadcast_cdb(int rob_id, sc_int<32> result);
};

void TomasuloProcessor::broadcast_cdb(int rob_id, sc_int<32> result) {
    // Atualiza RS liberando dependências
    for (auto& rs : reservation_stations) {
        if (rs.busy) {
            if (rs.qj_rob == rob_id) { rs.vj = result; rs.qj_rob = -1; }
            if (rs.qk_rob == rob_id) { rs.vk = result; rs.qk_rob = -1; }
        }
    }
    
    // Atualiza LSU liberando dependências (Store aguardando valor para salvar)
    for (auto& lsu : load_store_units) {
        if (lsu.busy && lsu.is_store) {
            if (lsu.qj_rob == rob_id) { lsu.vj = result; lsu.qj_rob = -1; }
        }
    }

    // Atualiza ROB
    reorder_buffer[rob_id].value = result;
    reorder_buffer[rob_id].ready = true;
    reorder_buffer[rob_id].state = "Writeback";
}

void TomasuloProcessor::run() {
    int initial_inst_count = instruction_queue.size();
    if (initial_inst_count > 0) {
        rob_capacity = initial_inst_count;
        reorder_buffer.resize(rob_capacity);
        for (int i = 0; i < rob_capacity; i++) {
            reorder_buffer[i].entry_id = i;
        }
    }

    add_start.write(false);
    mult_start.write(false);

    while (true) {
        current_cycle++;
        
        // Desce os sinais de start do ciclo anterior
        add_start.write(false);
        mult_start.write(false);

        // --- 1. COMMIT ---
        if (reorder_buffer[rob_head].busy && reorder_buffer[rob_head].state == "Writeback") {
            int b = rob_head;
            
            if (reorder_buffer[b].is_store) {
                memory.write(reorder_buffer[b].address, reorder_buffer[b].value);
            } else {
                if (reorder_buffer[b].dest_reg != -1) {
                    registers[reorder_buffer[b].dest_reg].value = reorder_buffer[b].value;
                    if (registers[reorder_buffer[b].dest_reg].rob_id == b) {
                        registers[reorder_buffer[b].dest_reg].rob_id = -1;
                    }
                }
            }
            
            reorder_buffer[b].state = "Commit";
            for (auto& inst : inst_history) {
                if (inst.id == reorder_buffer[b].inst_id) {
                    inst.cycle_commit = current_cycle;
                }
            }
            
            reorder_buffer[b].busy = false;
            rob_head = (rob_head + 1) % rob_capacity;
        }

        // --- 2. WRITEBACK (CDB Broadcast) ---
        int broadcast_rob_id = -1;
        sc_int<32> broadcast_val = 0;

        if (mult_done.read() == true) {
            int broadcast_rs_id = mult_rs_id_out.read();
            broadcast_val = mult_result.read();
            broadcast_rob_id = reservation_stations[broadcast_rs_id].dest_rob;
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
            broadcast_rob_id = reservation_stations[broadcast_rs_id].dest_rob;
            for (auto& inst : inst_history) {
                if (inst.id == reservation_stations[broadcast_rs_id].inst_id) {
                    inst.cycle_execute_end = current_cycle - 1;
                    inst.cycle_writeback = current_cycle;
                }
            }
            reservation_stations[broadcast_rs_id].busy = false;
        }

        if (broadcast_rob_id != -1) {
            broadcast_cdb(broadcast_rob_id, broadcast_val);
        }

        // --- 3. EXECUTE ---
        // Execute LSU
        for (auto& lsu : load_store_units) {
            if (lsu.busy) {
                if (!lsu.is_store) { // Load
                    if (lsu.ready_cycles == 2) {
                        for (auto& inst : inst_history) {
                            if (inst.id == lsu.inst_id) inst.cycle_execute_start = current_cycle;
                        }
                        reorder_buffer[lsu.dest_rob].state = "Execute";
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
                        broadcast_cdb(lsu.dest_rob, val);
                        lsu.busy = false;
                    }
                    if (lsu.ready_cycles > 0) lsu.ready_cycles--;
                } else { // Store
                    if (lsu.qj_rob == -1) { // Data is ready
                        if (lsu.ready_cycles == 2) {
                            for (auto& inst : inst_history) {
                                if (inst.id == lsu.inst_id) inst.cycle_execute_start = current_cycle;
                            }
                            reorder_buffer[lsu.dest_rob].state = "Execute";
                        }
                        if (lsu.ready_cycles == 1) {
                            for (auto& inst : inst_history) {
                                if (inst.id == lsu.inst_id) inst.cycle_execute_end = current_cycle;
                            }
                        }
                        if (lsu.ready_cycles == 0) {
                            int rob_id = lsu.dest_rob;
                            reorder_buffer[rob_id].value = lsu.vj;
                            reorder_buffer[rob_id].address = lsu.address;
                            reorder_buffer[rob_id].ready = true;
                            reorder_buffer[rob_id].state = "Writeback";

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
            if (rs.busy && rs.qj_rob == -1 && rs.qk_rob == -1 && rs.ready_cycles > 0) {
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
            if (rs.busy && rs.qj_rob == -1 && rs.qk_rob == -1 && rs.ready_cycles == 0 && rs.name.find("Add") != std::string::npos) {
                if (!adder_dispatched) {
                    add_rs_id_in.write(i);
                    add_op1.write(rs.vj);
                    add_op2.write(rs.vk);
                    add_opcode.write((rs.op == "add" || rs.op == "addi") ? 0 : 1);
                    add_start.write(true);
                    
                    for (auto& inst : inst_history) {
                        if (inst.id == rs.inst_id) inst.cycle_execute_start = current_cycle + 1;
                    }
                    reorder_buffer[rs.dest_rob].state = "Execute";
                    rs.ready_cycles = 2; // Inicia o countdown (Latência do Adder)
                    adder_dispatched = true;
                }
            }
        }

        // Dispatch to Multiplier
        bool mult_dispatched = false;
        for (size_t i = 0; i < reservation_stations.size(); i++) {
            auto& rs = reservation_stations[i];
            if (rs.busy && rs.qj_rob == -1 && rs.qk_rob == -1 && rs.ready_cycles == 0 && rs.name.find("Mult") != std::string::npos) {
                if (!mult_dispatched) {
                    mult_rs_id_in.write(i);
                    mult_op1.write(rs.vj);
                    mult_op2.write(rs.vk);
                    mult_opcode.write((rs.op == "mul") ? 0 : 1);
                    mult_start.write(true);
                    
                    for (auto& inst : inst_history) {
                        if (inst.id == rs.inst_id) inst.cycle_execute_start = current_cycle + 1;
                    }
                    reorder_buffer[rs.dest_rob].state = "Execute";
                    rs.ready_cycles = (rs.op == "mul") ? 10 : 40; // Inicia o countdown (Latência do Multiplier)
                    mult_dispatched = true;
                }
            }
        }

        // --- 4. ISSUE ---
        if (!instruction_queue.isEmpty()) {
            bool rob_full = reorder_buffer[rob_tail].busy;
            
            if (!rob_full) {
                Instruction inst = instruction_queue.front();

            if (inst.type == OP_LOGICAL || inst.type == OP_IMMEDIATE) {
                bool needs_mult = (inst.name == "mul" || inst.name == "div");
                for (size_t i = 0; i < reservation_stations.size(); i++) {
                    auto& rs = reservation_stations[i];
                    bool is_mult_rs = (rs.name.find("Mult") != std::string::npos);
                    
                    if (!rs.busy && (needs_mult == is_mult_rs)) {
                        rs.busy = true;
                        rs.op = inst.name;
                        rs.inst_id = inst.id;
                        rs.dest_rob = rob_tail;
                        rs.ready_cycles = 0; 

                        if (registers[inst.rs].rob_id == -1) {
                            rs.vj = registers[inst.rs].value;
                            rs.qj_rob = -1;
                        } else {
                            if (reorder_buffer[registers[inst.rs].rob_id].ready) {
                                rs.vj = reorder_buffer[registers[inst.rs].rob_id].value;
                                rs.qj_rob = -1;
                            } else {
                                rs.qj_rob = registers[inst.rs].rob_id;
                            }
                        }

                        if (inst.type == OP_IMMEDIATE) {
                            rs.vk = inst.imm;
                            rs.qk_rob = -1;
                        } else {
                            if (registers[inst.rt].rob_id == -1) {
                                rs.vk = registers[inst.rt].value;
                                rs.qk_rob = -1;
                            } else {
                                if (reorder_buffer[registers[inst.rt].rob_id].ready) {
                                    rs.vk = reorder_buffer[registers[inst.rt].rob_id].value;
                                    rs.qk_rob = -1;
                                } else {
                                    rs.qk_rob = registers[inst.rt].rob_id;
                                }
                            }
                        }

                        int dest_reg = (inst.type == OP_IMMEDIATE) ? inst.rt : inst.rd;

                        reorder_buffer[rob_tail].busy = true;
                        reorder_buffer[rob_tail].instruction = inst.name;
                        reorder_buffer[rob_tail].state = "Issue";
                        reorder_buffer[rob_tail].destination = "F" + std::to_string(dest_reg);
                        reorder_buffer[rob_tail].dest_reg = dest_reg;
                        reorder_buffer[rob_tail].ready = false;
                        reorder_buffer[rob_tail].inst_id = inst.id;
                        reorder_buffer[rob_tail].is_store = false;

                        registers[dest_reg].rob_id = rob_tail;

                        inst.cycle_issue = current_cycle;
                        inst_history.push_back(inst);
                        instruction_queue.pop_front();
                        rob_tail = (rob_tail + 1) % rob_capacity;
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
                        lsu.dest_rob = rob_tail;
                        lsu.ready_cycles = 2; // Tempo de latência da memória
                        
                        // rs é o registrador de endereço base
                        lsu.address = registers[inst.rs].value + inst.imm;

                        if (is_inst_store) {
                            if (registers[inst.rt].rob_id == -1) {
                                lsu.vj = registers[inst.rt].value;
                                lsu.qj_rob = -1;
                            } else {
                                if (reorder_buffer[registers[inst.rt].rob_id].ready) {
                                    lsu.vj = reorder_buffer[registers[inst.rt].rob_id].value;
                                    lsu.qj_rob = -1;
                                } else {
                                    lsu.qj_rob = registers[inst.rt].rob_id;
                                }
                            }
                        } else {
                            lsu.qj_rob = -1;
                            registers[inst.rt].rob_id = rob_tail;
                        }

                        reorder_buffer[rob_tail].busy = true;
                        reorder_buffer[rob_tail].instruction = inst.name;
                        reorder_buffer[rob_tail].state = "Issue";
                        reorder_buffer[rob_tail].ready = false;
                        reorder_buffer[rob_tail].inst_id = inst.id;

                        if (is_inst_store) {
                            reorder_buffer[rob_tail].is_store = true;
                            reorder_buffer[rob_tail].destination = "Mem[" + std::to_string(lsu.address) + "]";
                            reorder_buffer[rob_tail].dest_reg = -1;
                        } else {
                            reorder_buffer[rob_tail].is_store = false;
                            reorder_buffer[rob_tail].destination = "F" + std::to_string(inst.rt);
                            reorder_buffer[rob_tail].dest_reg = inst.rt;
                        }

                        inst.cycle_issue = current_cycle;
                        inst_history.push_back(inst);
                        instruction_queue.pop_front();
                        rob_tail = (rob_tail + 1) % rob_capacity;
                        break;
                    }
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
            for (const auto& rob : reorder_buffer) {
                if (rob.busy) { all_done = false; break; }
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
              << std::setw(8) << "Dest" << "\n";
    for (const auto& rs : reservation_stations) {
        std::string time_str = "";
        if (!rs.busy) {
            time_str = "";
        } else if (rs.qj_rob != -1 || rs.qk_rob != -1) {
            time_str = "-"; // Aguardando operandos
        } else if (rs.ready_cycles > 0) {
            time_str = std::to_string(rs.ready_cycles); // Mostra o countdown regressivo
        } else if (rs.ready_cycles == -1) {
            time_str = "0"; // Execução finalizada, aguardando Barramento (Writeback)
        } else {
            time_str = "-"; // Pronto para despachar neste ciclo
        }

        std::string qj_str = (rs.qj_rob != -1) ? std::to_string(rs.qj_rob) : "";
        std::string qk_str = (rs.qk_rob != -1) ? std::to_string(rs.qk_rob) : "";
        std::string dest_str = (rs.dest_rob != -1) ? std::to_string(rs.dest_rob) : "";

        std::cout << std::left 
                  << std::setw(8) << time_str
                  << std::setw(8) << rs.name 
                  << std::setw(6) << (rs.busy ? "yes" : "no") 
                  << std::setw(8) << (rs.busy ? rs.op : "") 
                  << std::setw(8) << (rs.busy && rs.qj_rob == -1 ? std::to_string(rs.vj.to_int()) : "") 
                  << std::setw(8) << (rs.busy && rs.qk_rob == -1 ? std::to_string(rs.vk.to_int()) : "") 
                  << std::setw(8) << (rs.busy ? qj_str : "") 
                  << std::setw(8) << (rs.busy ? qk_str : "") 
                  << std::setw(8) << (rs.busy ? dest_str : "") << "\n";
    }

    std::cout << "\n--- LOAD/STORE UNITS ---\n";
    std::cout << std::left 
              << std::setw(8) << "Time" 
              << std::setw(8) << "Name" 
              << std::setw(6) << "Busy" 
              << std::setw(10) << "Address" 
              << std::setw(8) << "Dest" << "\n";
    for (const auto& lsu : load_store_units) {
        std::string time_str = "";
        if (!lsu.busy) {
            time_str = "";
        } else if (lsu.is_store && lsu.qj_rob != -1) {
            time_str = "-"; // Store aguardando valor do registrador para salvar
        } else {
            time_str = std::to_string(lsu.ready_cycles);
        }

        std::string dest_str = (lsu.dest_rob != -1) ? std::to_string(lsu.dest_rob) : "";

        std::cout << std::left 
                  << std::setw(8) << time_str
                  << std::setw(8) << lsu.name 
                  << std::setw(6) << (lsu.busy ? "yes" : "no") 
                  << std::setw(10) << (lsu.busy ? std::to_string(lsu.address.to_uint()) : "") 
                  << std::setw(8) << (lsu.busy ? dest_str : "") << "\n";
    }

    std::cout << "\n--- REORDER BUFFER ---\n";
    std::cout << std::left 
              << std::setw(6) << "ID" 
              << std::setw(6) << "Busy" 
              << std::setw(10) << "Inst" 
              << std::setw(12) << "State" 
              << std::setw(10) << "Dest" 
              << std::setw(8) << "Value" << "\n";
    for (int i = 0; i < rob_capacity; i++) {
        const auto& b = reorder_buffer[i];
        std::cout << std::left 
                  << std::setw(6) << i
                  << std::setw(6) << (b.busy ? "yes" : "no") 
                  << std::setw(10) << (b.busy ? b.instruction : "") 
                  << std::setw(12) << (b.busy ? b.state : "") 
                  << std::setw(10) << (b.busy ? b.destination : "") 
                  << std::setw(8) << (b.busy && b.ready ? std::to_string(b.value.to_int()) : "") << "\n";
    }

    std::cout << "\n--- REGISTERS ---\n";
    for (size_t i = 0; i < registers.size(); i += 2) {
        std::cout << "F" << i << ": ";
        if (registers[i].rob_id != -1) {
            std::cout << std::left << std::setw(6) << ("ROB" + std::to_string(registers[i].rob_id));
        } else {
            std::cout << std::left << std::setw(6) << registers[i].value.to_int();
        }
        if (((i / 2) + 1) % 8 == 0) std::cout << "\n";
    }
}

void TomasuloProcessor::print_timing_diagram() {
    std::cout << "\n========================================================================\n";
    std::cout << " FINAL TIMING DIAGRAM (ISSUE | EXECUTE | WRITEBACK | COMMIT)\n";
    std::cout << "========================================================================\n";
    std::cout << std::left 
              << std::setw(10) << "Inst ID" 
              << std::setw(10) << "Name" 
              << std::setw(10) << "Issue" 
              << std::setw(15) << "Execute" 
              << std::setw(12) << "Writeback" 
              << std::setw(10) << "Commit" << "\n";
              
    for (const auto& inst : inst_history) {
        std::string issue_time = (inst.cycle_issue != -1) ? std::to_string(inst.cycle_issue) : "Pending";
        std::string exec_time = (inst.cycle_execute_start != -1 && inst.cycle_execute_end != -1) ? 
                                (std::to_string(inst.cycle_execute_start) + "-" + std::to_string(inst.cycle_execute_end)) : "Pending";
        std::string wb_time = (inst.cycle_writeback != -1) ? std::to_string(inst.cycle_writeback) : "Pending";
        std::string commit_time = (inst.cycle_commit != -1) ? std::to_string(inst.cycle_commit) : "Pending";

        std::cout << std::left 
                  << std::setw(10) << inst.id 
                  << std::setw(10) << inst.name 
                  << std::setw(10) << issue_time 
                  << std::setw(15) << exec_time 
                  << std::setw(12) << wb_time 
                  << std::setw(10) << commit_time << "\n";
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