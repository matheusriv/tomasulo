#include "InstructionQueue.hpp"
#include <stdexcept>

InstructionQueue::InstructionQueue() {}

void InstructionQueue::push_back(const Instruction& inst) {
    queue.push_back(inst);
}

Instruction InstructionQueue::pop_front() {
    if (isEmpty()) {
        throw std::out_of_range("Instruction queue is empty!");
    }
    Instruction first = queue.front();
    queue.erase(queue.begin());
    return first;
}

Instruction InstructionQueue::front() const {
    if (isEmpty()) {
        throw std::out_of_range("Instruction queue is empty!");
    }
    return queue.front();
}

bool InstructionQueue::isEmpty() const {
    return queue.empty();
}

int InstructionQueue::size() const {
    return queue.size();
}

void InstructionQueue::clear() {
    queue.clear();
}