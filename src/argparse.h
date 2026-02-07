#pragma once

// Parse register name to number (e.g., "r5" -> 5, "r31" -> 31)
int parseRegister(char * reg);

// Parse a literal value (handles hex 0x... and decimal)
// NOTE: Labels must be resolved BEFORE calling this function
unsigned long long parseLiteral(char * lit);

// Check if a string is a label reference (starts with :)
int isLabel(char * str);

// Parse three register arguments: "r0, r1, r3" -> rd=0, rs=1, rt=3
void parseThreeReg(char * args, int * rd, int * rs, int * rt);

// Parse two register arguments: "r5, r6" -> rd=5, rs=6
void parseTwoReg(char * args, int * rd, int * rs);

// Parse register and literal: "r5, 10" -> rd=5, L=10
void parseRegLit(char * args, int * rd, int * L);

// Parse memory reference: "r7, (r6)(0)" -> rd=7, rs=6, offset=0
void parseMemoryLoad(char * args, int * rd, int * rs, int * offset);

// Parse memory store: "(r5)(8), r3" -> rd=5, rs=3, offset=8
void parseMemoryStore(char * args, int * rd, int * rs, int * offset);

// Parse single register: "r5" -> 5
int parseSingleReg(char * args);

// Extract command name from instruction line
// e.g., "add r0, r1, r3" -> "add"
char * extractCommandName(char * line);

// Extract arguments from instruction line
// e.g., "add r0, r1, r3" -> "r0, r1, r3"
char * extractArguments(char * line);
