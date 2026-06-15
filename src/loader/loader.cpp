#include<fstream>
#include<iostream>
#include<string>
#include<sstream>
#include<vector>
#include<bitset>
using namespace std;

vector<string> logical_instructions = {
    "and", "or", "xor", "not", "add", "sub",
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

uint get_funct(string& instruction) {
    if(instruction == "add") return 0;
    if(instruction == "sub") return 1;
    if(instruction == "and") return 2;
    if(instruction == "or") return 3;
    if(instruction == "xor") return 4;
    if(instruction == "not") return 5;
    return -1;
}

int parse_operand(const string& op_str) {
    if (op_str.empty()) return 0;
    if (op_str[0] == '$') {
        return stoi(op_str.substr(1)); // Pula o '$' e converte
    }
    return stoi(op_str); // Converte direto se não tiver '$'
}

string output = 
"#ifndef LOAD_DATA_CPP\n"
"#define LOAD_DATA_CPP\n"
"\n"
"#include <systemc.h>\n"
"\n"
"void load (sc_uint<8> *mem_instrucao, sc_int<32> *mem_dados) {\n";

int main(int argc, char* argv[]) {
    ifstream file(argv[1]);
    if (!file.is_open()) {
        cerr << "Erro: Nao foi possivel abrir o arquivo '" << argv[1] << "'!" << endl;
        return 1;
    }

    string token;
    bool isData = true;
    bool isAddress = true;
    int offset = 0;
    while(getline(file, token)) {
        if(token == "--") {
            isData = false;
            continue;
        }

        std::istringstream iss(token);

        if(isData) {
            int address, value;
            iss >> address >> value;

            output.append("\tmem_dados[");
            output.append(to_string(address));
            output.append("] = ");
            output.append(to_string(value));
            output.append(";\n");
        } else {
            std::string instruc;
            iss >> instruc;

            string palavra;
            if(is_logical_instruction(instruc)) {
                string s_rd, s_rs, s_rt;
                iss >> s_rd >> s_rs >> s_rt;
                uint rd = parse_operand(s_rd);
                uint rs = parse_operand(s_rs);
                uint rt = parse_operand(s_rt);
                palavra.append(bitset<11>{get_funct(instruc)}.to_string());
                palavra.append(bitset<5>{rd}.to_string());
                palavra.append(bitset<5>{rs}.to_string());
                palavra.append(bitset<5>{rt}.to_string());
                palavra.append(bitset<6>{0b010000}.to_string());
            } else if (is_jump_instruction(instruc)) {
                if (instruc == "j") {
                    string s_imm;
                    iss >> s_imm;
                    uint immediate = parse_operand(s_imm);
                    palavra.append(bitset<26>{immediate}.to_string());
                    palavra.append(bitset<6>{0b110000}.to_string());
                } else if (instruc == "beq" || instruc == "bne") {
                    string s_rs, s_rt, s_imm;
                    iss >> s_rs >> s_rt >> s_imm;
                    uint rs = parse_operand(s_rs);
                    uint rt = parse_operand(s_rt);
                    uint immediate = parse_operand(s_imm);
                    palavra.append(bitset<16>{immediate}.to_string());
                    palavra.append(bitset<5>{rs}.to_string());
                    palavra.append(bitset<5>{rt}.to_string());
                    if (instruc == "beq") {
                        palavra.append(bitset<6>{0b110001}.to_string());
                    } else if (instruc == "bne") {
                        palavra.append(bitset<6>{0b110010}.to_string());
                    }
                }
            } else if (is_memory_instruction(instruc)) {
                string s_rt, s_rs, s_imm;
                iss >> s_rt >> s_rs >> s_imm;
                int rt = parse_operand(s_rt);
                int rs = parse_operand(s_rs);
                int immediate = parse_operand(s_imm);
                palavra.append(bitset<16>(immediate).to_string());
                palavra.append(bitset<5>(rs).to_string());
                palavra.append(bitset<5>(rt).to_string());
                if (instruc == "ld") {
                    palavra.append(bitset<6>{0b100000}.to_string());
                } else if (instruc == "st") {
                    palavra.append(bitset<6>{0b100001}.to_string());
                }
            } else if (is_immediate_instruction(instruc)) {
                string s_rt, s_rs, s_imm;
                iss >> s_rt >> s_rs >> s_imm;
                uint rt = parse_operand(s_rt);
                uint rs = parse_operand(s_rs);
                uint immediate = parse_operand(s_imm);
                palavra.append(bitset<16>{immediate}.to_string());
                palavra.append(bitset<5>{rs}.to_string());
                palavra.append(bitset<5>{rt}.to_string());
                if (instruc == "addi") {
                    palavra.append(bitset<6>{0b000001}.to_string());
                } else if (instruc == "andi") {
                    palavra.append(bitset<6>{0b000010}.to_string());
                } else if (instruc == "ori") {
                    palavra.append(bitset<6>{0b000011}.to_string());
                } else if (instruc == "xori") {
                    palavra.append(bitset<6>{0b000100}.to_string());
                }
            }

            for(int i=0; i<palavra.size(); i+=8) {
                string byte = palavra.substr(i, 8);
                bitset<8> b(byte);
                output.append("\tmem_instrucao[");
                output.append(to_string(offset + i/8));
                output.append("] = ");
                output.append("0b");
                output.append(b.to_string());
                output.append(";\n");
            }
            offset += 4;
        }
    }

    output.append("};\n\n#endif // !LOAD_DATA_CPP");

    ofstream outFile("src/load_data.cpp");
    outFile << output;
    outFile.close();
    cout << "\n\tArquivo src/load_data.cpp gerado com sucesso!\n" << endl;
}