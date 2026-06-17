#ifndef INSTRUCTION_QUEUE_HPP
#define INSTRUCTION_QUEUE_HPP

#include <vector>
#include "instruction.hpp"

class InstructionQueue {
private:
    std::vector<Instruction> queue;

public:
    InstructionQueue();

    // Adiciona uma instrução ao final da fila
    void push_back(const Instruction& inst);
    
    // Retorna e remove a primeira instrução da fila
    Instruction pop_front();
    
    // Retorna a primeira instrução sem removê-la
    Instruction front() const;
    
    // Verifica se a fila está vazia
    bool isEmpty() const;
    
    // Retorna a quantidade de instruções na fila
    int size() const;
    
    // Limpa todas as instruções da fila
    void clear();
};

#endif // INSTRUCTION_QUEUE_HPP