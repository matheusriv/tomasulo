#include<fstream>
#include<iostream>
#include<string>
#include<sstream>
#include<vector>
#include "loader.hpp"
using namespace std;

vector<string> logical_instructions = {
    "and", "or", "xor", "not", "add", "sub", "mul", "div"
};

vector<string> jump_instructions = {
    "j", "beq", "bne"
};

vector<string> memory_instructions = {
    "ld", "st"
};

vector<string> immediate_instructions = {
    "addi", "andi", "ori", "xori"
};

bool is_logical_instruction(const string& instruction) {
    for(const string &instr : logical_instructions) {
        if(instruction == instr) {
            return true;
        }
    }
    return false;
}

bool is_jump_instruction(const string& instruction) {
    for(const string &instr : jump_instructions) {
        if(instruction == instr) {
            return true;
        }
    }
    return false;
}

bool is_memory_instruction(const string& instruction) {
    for(const string &instr : memory_instructions) {
        if(instruction == instr) {
            return true;
        }
    }
    return false;
}

bool is_immediate_instruction(const string& instruction) {
    for(const string &instr : immediate_instructions) {
        if(instruction == instr) {
            return true;
        }
    }
    return false;
}

int parse_operand(const string& op_str) {
    if (op_str.empty()) return 0;
    
    string clean_str = op_str;
    
    // remove a vírgula se ela estiver colada no operando (ex: "f6,")
    if (clean_str.back() == ',') {
        clean_str.pop_back();
    }

    // pula os prefixos tradicionais '$', 'f', 'F', 'r', ou 'R'
    if (clean_str[0] == '$' || clean_str[0] == 'f' || clean_str[0] == 'F' || clean_str[0] == 'r' || clean_str[0] == 'R') {
        return stoi(clean_str.substr(1)); 
    }
    return stoi(clean_str); 
}

void load_program(const string& filepath, InstructionQueue& instruction_queue, sc_int<32>* mem_dados) {
    ifstream file(filepath);
    if (!file.is_open()) {
        cerr << "Error: Could not open file '" << filepath << "'!" << endl;
        return;
    }

    string token;
    bool isData = true;
    int inst_id_counter = 0;

    while(getline(file, token)) {
        if (!token.empty() && token.back() == '\r') {
            token.pop_back();
        }

        if(token == "--") {
            isData = false;
            continue;
        }
        if(token.empty()) continue; // evita linhas vazias

        istringstream iss(token);

        if(isData) {
            int address, value;
            iss >> address >> value;
            mem_dados[address] = value;
        } else {
            string instruc;
            iss >> instruc;

            Instruction inst;
            inst.id = inst_id_counter++;
            inst.name = instruc;
            inst.rd = -1;
            inst.rs = -1;
            inst.rt = -1;
            inst.imm = 0;

            if(is_logical_instruction(instruc)) {
                inst.type = OP_LOGICAL;
                string s_rd, s_rs, s_rt;
                iss >> s_rd >> s_rs >> s_rt;
                inst.rd = parse_operand(s_rd);
                inst.rs = parse_operand(s_rs);
                inst.rt = parse_operand(s_rt);
            } else if (is_jump_instruction(instruc)) {
                inst.type = OP_JUMP;
                if (instruc == "j") {
                    string s_imm;
                    iss >> s_imm;
                    inst.imm = parse_operand(s_imm);
                } else if (instruc == "beq" || instruc == "bne") {
                    string s_rs, s_rt, s_imm;
                    iss >> s_rs >> s_rt >> s_imm;
                    inst.rs = parse_operand(s_rs);
                    inst.rt = parse_operand(s_rt);
                    inst.imm = parse_operand(s_imm);
                }
            } else if (is_memory_instruction(instruc)) {
                inst.type = OP_MEMORY;
                string s_rt, s_rs, s_imm;
                iss >> s_rt >> s_rs >> s_imm;
                inst.rt = parse_operand(s_rt);
                inst.rs = parse_operand(s_rs);
                inst.imm = parse_operand(s_imm);
            } else if (is_immediate_instruction(instruc)) {
                inst.type = OP_IMMEDIATE;
                string s_rt, s_rs, s_imm;
                iss >> s_rt >> s_rs >> s_imm;
                inst.rt = parse_operand(s_rt);
                inst.rs = parse_operand(s_rs);
                inst.imm = parse_operand(s_imm);
            }
            
            instruction_queue.push_back(inst);
        }
    }
}