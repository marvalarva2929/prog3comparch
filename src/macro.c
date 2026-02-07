#include "macro.h"
#include "parse.h"
#include "argparse.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

// Forward declare trimWhitespace (from argparse.c)
extern char * trimWhitespace(char * str);

// Check if a command type is a macro
int isMacro(CommandType type) {
    return (type == CLR || type == IN || type == OUT || 
            type == LD || type == PUSH || type == POP || 
			type == HALT);
}



// Helper to create an entry for an expanded instruction
Entry createExpandedEntry(Entry * original, const char * instruction, int addressOffset) {
    Entry entry;
    entry.address = original->address + addressOffset;
    entry.size = 4;  // Instructions are 4 bytes
    entry.type = 0;  // 0 = instruction
    entry.str = strdup(instruction);
   	
    // Parse the instruction to set cmd.type
    char * cmd = extractCommandName((char*)instruction);
    char * args = extractArguments((char*)instruction);
    
    entry.str = args;
    
    // Determine command type
    if (strcmp(cmd, "xor") == 0) entry.cmd.type = XOR;
    else if (strcmp(cmd, "priv") == 0) entry.cmd.type = PRIV;
    else if (strcmp(cmd, "subi") == 0) entry.cmd.type = SUBI;
    else if (strcmp(cmd, "addi") == 0) entry.cmd.type = ADDI;
    else if (strcmp(cmd, "mov") == 0) entry.cmd.type = MOV;
    else if (strcmp(cmd, "shftli") == 0) entry.cmd.type = SHFTLI;
    else {
        fprintf(stderr, "Error: Unknown command in macro expansion: %s\n", cmd);
        exit(1);
    }
    
    free(cmd);
    return entry;
}

// clr rd -> xor rd, rd, rd
int expandClr(Entry * original, Entry * output) {
    // Parse the register from original args
    int rd = parseSingleReg(original->str);
    
    char instruction[64];
    snprintf(instruction, sizeof(instruction), "xor r%d, r%d, r%d", rd, rd, rd);
    
    output[0] = createExpandedEntry(original, instruction, 0);
    
    return 1;  // Returns 1 instruction
}

// halt -> priv 0, 0, 0, 0x0
int expandHalt(Entry * original, Entry * output) {
    output[0] = createExpandedEntry(original, "priv r0, r0, r0, 0", 0);
    return 1;
}

// in rd, rs -> priv rd, rs, 0, 0x3
int expandIn(Entry * original, Entry * output) {
    int rd, rs;
    parseTwoReg(original->str, &rd, &rs);
    
    char instruction[64];
    snprintf(instruction, sizeof(instruction), "priv r%d, r%d, r0, 3", rd, rs);
    
    output[0] = createExpandedEntry(original, instruction, 0);
    return 1;
}

// out rd, rs -> priv rd, rs, 0, 0x4
int expandOut(Entry * original, Entry * output) {
    int rd, rs;
    parseTwoReg(original->str, &rd, &rs);
    
    char instruction[64];
    snprintf(instruction, sizeof(instruction), "priv r%d, r%d, r0, 4", rd, rs);
    
    output[0] = createExpandedEntry(original, instruction, 0);
    return 1;
}

// ld rd, L -> Expands to multiple instructions to load full 64-bit value
int expandLd(Entry * original, Entry * output, uint64_t addr) {
	    fprintf(stderr, "DEBUG expandLd: address=%llu (0x%llu)\n", addr, addr);
// Parse register from args (format: "r5, :label" or "r5, 0x1000")
    char * argsCopy = strdup(original->str);
    char * reg = strtok(argsCopy, ",");
    int rd = parseRegister(reg);
    free(argsCopy);
    
    int count = 0;
    char instruction[64];
    
    // Start with xor to clear register
    snprintf(instruction, sizeof(instruction), "xor r%d, r%d, r%d", rd, rd, rd);
    output[count] = createExpandedEntry(original, instruction, count * 4);
    count++;
    
    // Extract 12-bit chunks from MSB to LSB
    
    uint64_t chunks[6];
    chunks[0] = (addr >> 52) & 0xFFF;  // Bits 52-63
    chunks[1] = (addr >> 40) & 0xFFF;  // Bits 40-51
    chunks[2] = (addr >> 28) & 0xFFF;  // Bits 28-39
    chunks[3] = (addr >> 16) & 0xFFF;  // Bits 16-27
    chunks[4] = (addr >> 4) & 0xFFF;   // Bits 4-15
    chunks[5] = addr & 0xF;            // Bits 0-3
    
    // Build from first non-zero chunk down
    for (int i = 0; i < 6; i++) {
        if (i == 5) {
            // Last 4 bits
			snprintf(instruction, sizeof(instruction), "addi r%d, %llu", rd, chunks[i]);
			output[count] = createExpandedEntry(original, instruction, 0);
			count++;
        } else {
            // 12-bit chunks
			snprintf(instruction, sizeof(instruction), "addi r%d, %llu", rd, chunks[i]);
			output[count] = createExpandedEntry(original, instruction, 0);
			count++;
			snprintf(instruction, sizeof(instruction), "shftli r%d, %d", rd, (i == 4) ? 4 : 12);
			output[count] = createExpandedEntry(original, instruction, 0);
			count++;
		}
    }
    // 4148
	// 4160
    return count;
}

// push rd -> subi r31, 8; mov (r31)(0), rd
int expandPush(Entry * original, Entry * output) {
    int rd = parseSingleReg(original->str);
    
    char instruction[64];
        snprintf(instruction, sizeof(instruction), "mov (r31)(-8), r%d", rd);
    output[0] = createExpandedEntry(original, instruction, 4);

    snprintf(instruction, sizeof(instruction), "subi r31, 8");
    output[1] = createExpandedEntry(original, instruction, 0);
    


    return 2;
}

// pop rd -> mov rd, (r31)(0); addi r31, 8
int expandPop(Entry * original, Entry * output) {
    int rd = parseSingleReg(original->str);
    
    char instruction[64];
    
    snprintf(instruction, sizeof(instruction), "mov r%d, (r31)(0)", rd);
    output[0] = createExpandedEntry(original, instruction, 0);
    
    snprintf(instruction, sizeof(instruction), "addi r31, 8");
    output[1] = createExpandedEntry(original, instruction, 4);
    
    return 2;
}

// Master expansion function
int expandMacro(Entry * original, Entry * output, ltable * table) {
    if (original->type == 1 || !isMacro(original->cmd.type)) {
        // Not a macro, just copy it
        output[0] = *original;
        return 1;
    }
    
    switch (original->cmd.type) {
        case CLR:
            return expandClr(original, output);
        case IN:
            return expandIn(original, output);
        case OUT:
            return expandOut(original, output);
        case PUSH:
            return expandPush(original, output);
        case POP:
            return expandPop(original, output);
        case LD: {
            // Need to resolve label if present
            char * argsCopy = strdup(original->str);
            char * comma = strchr(argsCopy, ',');
            if (comma) {
                char * labelOrAddr = comma + 1;
                labelOrAddr = trimWhitespace(labelOrAddr);
				uint64_t address;

				if (isLabelReference(labelOrAddr)) {
					address = getintAddress(labelOrAddr, table);
				} else {
					address = parseLiteral(labelOrAddr);
				}
                free(argsCopy);
                return expandLd(original, output, address);
            }
            fprintf(stderr, "Error: Invalid ld macro format\n");
            exit(1);
        }
		case HALT: 
			return expandHalt(original, output);
		default:
            fprintf(stderr, "Error: Unknown macro type\n");
            exit(1);
    }
}

// Check if a string is a label reference
int isLabelReference(char * str) {
    if (str == NULL || strlen(str) == 0) return 0;
    return str[0] == ':';
}

// Extract label name (remove the ':')
char * extractLabelName(char * labelRef) {
    if (!isLabelReference(labelRef)) return NULL;
    return strdup(labelRef + 1);  // Skip the ':'
}
