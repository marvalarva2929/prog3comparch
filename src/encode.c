#include "encode.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// Helper to build a 32-bit instruction
// Bits are filled from MSB (31) to LSB (0)
#include "argparse.h"
#include "encode.h"

int isLiteralArg(char * arg) {
	char new = trim(arg)[0];
	return new == 'r' ? 0 : new == '(' ? 0 : 1;
}

int isParArg(char * arg) {
	char new = trim(arg)[0];
	return new == '(';
}

int isRegArg(char * arg) {
	return !(isLiteralArg(arg) | isParArg(arg));
}

uint32_t build_instruction(uint32_t opcode, int rd, int rs, int rt, uint32_t imm) {
    uint32_t instr = 0;
	
	instr += opcode; instr <<= 5;
	instr += rd; 	 instr <<= 5;
	instr += rs; 	 instr <<= 5;
	instr += rt; 	 instr <<= 12;
	instr += (imm & 0xFFF);

    return instr;
}

int instructionList(char ** list, char * args) {
    if (args == NULL || strlen(args) == 0) return 0;
    
    char * argsCopy = strdup(args);
    int count = 0;
    char * token = strtok(argsCopy, ",");
    
    while (token != NULL && count < 4) {
        list[count] = trim(token);
        count++;
        token = strtok(NULL, ",");
    }
    
    free(argsCopy);
    return count;
}



uint32_t getMovInstruction(Entry * entry) {
	uint32_t opcode = cmdTable[entry->cmd.type].opcode;
	int r[3] = {0, 0, 0}; // rd rs rt
	int imm = 0;
	
	char * args[4];
	for (int i = 0; i < 4; i++) 
		args[i] = malloc(500*sizeof(char));
	
	char *rr = entry->str;
	int len = instructionList(args, rr);
	if (isRegArg(args[0]) && isParArg(args[1])) {
		parseMemoryLoad(rr, &r[0], &r[1], &imm);
	} else if (isRegArg(args[0]) && isRegArg(args[1])) {
		opcode += 1;
		r[0] = parseSingleReg(args[0]);
		r[1] = parseSingleReg(args[1]);
	} else if (isRegArg(args[0]) && isLiteralArg(args[1])) {
		opcode += 2;
		r[0] = parseSingleReg(args[0]);
		imm = parseLiteral(args[1]);
	} else if (isParArg(args[0]) && isRegArg(args[1])) {
		opcode += 3;
		parseMemoryStore(rr, &r[0], &r[1], &imm);
	}

	return build_instruction(opcode, r[0], r[1], r[2], imm);
}

uint32_t getBrrInstruction(Entry * entry) {
	uint32_t opcode = cmdTable[entry->cmd.type].opcode;
	char * arg = entry->str;
	
	if (isLiteralArg(arg))
		return build_instruction(opcode + 1, 0, 0, 0, parseLiteral(arg));
	return build_instruction(opcode, parseSingleReg(arg), 0, 0, 0);

}

uint32_t getInstruction(Entry * entry) {
	if (entry->cmd.type == MOV) return getMovInstruction(entry);
	if (entry->cmd.type == BRR) return getBrrInstruction(entry);
	uint32_t opcode = cmdTable[entry->cmd.type].opcode;
	int r[3] = {0, 0, 0}; // rd rs rt
	int imm = 0;
	
	char * args[4];
	for (int i = 0; i < 4; i++) 
		args[i] = malloc(500*sizeof(char));
	int len = instructionList(args, entry->str);
	for (int i = 0; i < len; i++) {
		if (isLiteralArg(args[i])) {
			imm = parseLiteral(args[i]);
			break;
		}
		r[i] = parseSingleReg(args[i]);
	}
	return build_instruction(opcode, r[0], r[1], r[2], imm);
}


