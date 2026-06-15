struct Instruction {
    int id;
    string name;
    OpType type;
    int rd, rs, rt;
    int imm;
    
    // Controle do Diagrama de Tempos
    int cycle_issue = -1;
    int cycle_execute_start = -1;
    int cycle_execute_end = -1;
    int cycle_writeback = -1;
};