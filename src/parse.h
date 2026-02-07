#pragma once
typedef struct Script Script;
typedef struct Entry Entry;
typedef struct Command Command;

#define null NULL
#include <stdio.h>
#include "labletable.h"
#include "argparse.h"

char * trim(char * totrim);

typedef enum CommandType {
	ADD,
	ADDI,
	SUB,
	SUBI,
	MUL,
	DIV,
	AND,
	OR,
	XOR,
	NOT,
	SHFTR,
	SHFTRI,
	SHFTL,
	SHFTLI,
	BR,
	BRR,
	BRNZ,
	CALL,
	RETURN,
	BRGT,
	PRIV,
	MOV,
	ADDF,
	SUBF,
	MULF,
	DIVF,
// MACROS
	IN,
	OUT,
	CLR,
	LD,
	PUSH,
	POP,
// data
	DATA,
	HALT,
} CommandType;

struct Command {
	char * args;
	int argl;
	CommandType type;
};

struct Entry {
	unsigned long long value;
	int address;
	int size;
	int type;
	
	int numArgs;
	char * str;
	char * lbl;
	Command cmd;
};

struct Script {
	Entry * entries;
	ltable * ltable;
	int numEntries;
	int byteSize;
};

typedef struct {
    const char *name;
	CommandType type;
	int cnt;
	int opcode;
} CmdMap;

static CmdMap cmdTable[] = {
    {"add", ADD, 1,   0x18}, 
	{"addi", ADDI, 1, 0x19},
    {"sub", SUB, 1,   0x1a}, 
	{"subi", SUBI, 1, 0x1b},
    {"mul", MUL, 1, 0x1c}, 
	{"div", DIV, 1, 0x1d},
    {"and", AND, 1, 0x0}, 
	{"or", OR, 1, 0x1}, 
	{"xor", XOR, 1, 0x2}, 
	{"not", NOT, 1, 0x3},
    {"shftr", SHFTR, 1, 0x4}, 
	{"shftri", SHFTRI, 1, 0x5},
    {"shftl", SHFTL, 1, 0x6},
	{"shftli", SHFTLI, 1, 0x7},
    {"br", BR, 1, 0x8},
	{"brr", BRR, 1, 0x9}, 
	{"brnz", BRNZ, 1, 0xb}, 
    {"call", CALL, 1, 0xc},
	{"return", RETURN, 1, 0xd},
	{"brgt", BRGT, 1, 0xe},
    {"priv", PRIV, 1, 0xf},
    {"mov", MOV, 1, 0x10},
    {"addf", ADDF, 1, 0x14}, 
	{"subf", SUBF, 1, 0x15},
    {"mulf", MULF, 1, 0x16}, 
	{"divf", DIVF, 1, 0x17},
    {"in", IN, 1, -1},
	{"out", OUT, 1, -1},
    {"clr", CLR, 1, -1}, 
	{"ld", LD, 12, -1},
    {"push", PUSH, 2, -1}, 
	{"pop", POP, 2, -1},
    {"data", DATA, 1, -1},
	{"halt", HALT, 1, -1}
};

Script * getScript(char * filename);
