#pragma once
#include "parse.h"
#include "labletable.h"

// Macro detection
int isMacro(CommandType type);

// Macro expansion functions
// These return the number of entries (instructions) the macro expands to
// and fill the output array with the expanded Entry objects

// clr rd -> xor rd, rd, rd
int expandClr(Entry * original, Entry * output);

// halt -> priv 0, 0, 0, 0x0
int expandHalt(Entry * original, Entry * output);

// in rd, rs -> priv rd, rs, 0, 0x3
int expandIn(Entry * original, Entry * output);

// out rd, rs -> priv rd, rs, 0, 0x4
int expandOut(Entry * original, Entry * output);

// ld rd, L -> multiple instructions to build 64-bit value
// NOTE: L should be the resolved address (label already looked up)
int expandLd(Entry * original, Entry * output, uint64_t address);

// push rd -> subi r31, 8; mov (r31)(0), rd
int expandPush(Entry * original, Entry * output);

// pop rd -> mov rd, (r31)(0); addi r31, 8
int expandPop(Entry * original, Entry * output);

// Master expansion function - detects macro type and calls appropriate function
// Returns number of entries created
int expandMacro(Entry * original, Entry * output, ltable * table);

// Helper: Check if a string is a label reference (starts with :)
int isLabelReference(char * str);

// Helper: Extract label name from reference (removes :)
char * extractLabelName(char * labelRef);
