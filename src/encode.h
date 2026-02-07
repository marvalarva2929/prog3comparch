#include "parse.h"
#include <stdint.h> 

uint32_t getInstruction(Entry * entry);
uint32_t build_instruction(uint32_t opcode, int rd, int rs, int rt, uint32_t imm);
