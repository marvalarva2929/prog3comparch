#include "argparse.h"
#include "parse.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

// Helper: trim whitespace (modifies in-place, returns adjusted pointer)
char * trimWhitespace(char * str) {
    if (str == NULL) return str;
    
    // Skip leading whitespace
    while (*str && isspace((unsigned char)*str)) {
        str++;
    }
    
    // If entire string is whitespace, return empty string marker
    if (*str == '\0') {
        return str;
    }
    
    // Find the end of the string
    char * end = str + strlen(str) - 1;
    
    // Trim trailing whitespace
    while (end > str && isspace((unsigned char)*end)) {
        end--;
    }
    
    // Write null terminator after last non-whitespace character
    *(end + 1) = '\0';
    
    return str;
}

// Helper: trim whitespace (allocates new string)
char * trimWhitespaceAlloc(char * str) {
    if (str == NULL) return NULL;
    
    // Find first non-whitespace character
    int start = 0;
    while (str[start] && isspace((unsigned char)str[start])) {
        start++;
    }
    
    // If entire string is whitespace, return empty string
    if (str[start] == '\0') {
        char * result = malloc(1);
        result[0] = '\0';
        return result;
    }
    
    // Find last non-whitespace character
    int end = strlen(str) - 1;
    while (end > start && isspace((unsigned char)str[end])) {
        end--;
    }
    
    // Allocate and copy trimmed content
    int len = end - start + 1;
    char * result = malloc((len + 1) * sizeof(char));
    strncpy(result, str + start, len);
    result[len] = '\0';
    
    return result;
}

// Parse register name to number
int parseRegister(char * reg) {
    reg = trimWhitespace(reg);
    if (reg[0] != 'r') {
        fprintf(stderr, "Error: Invalid register '%s'\n", reg);
        exit(1);
    }
    int regNum = atoi(reg + 1);  // Skip 'r'
    if (regNum < 0 || regNum > 31) {
        fprintf(stderr, "Error: Register out of range (0-31): r%d\n", regNum);
        exit(1);
    }
    return regNum;
}

// Parse a literal value or label reference
// Returns: the numeric value
// Note: Caller should check if this is a label (starts with :) BEFORE calling
// If it's a label, you need to look it up in the label table first
unsigned long long parseLiteral(char * lit) {
    lit = trimWhitespace(lit);
    
    // Check if it's a label reference
    if (lit[0] == ':') {
        fprintf(stderr, "Error: parseLiteral called with unresolved label '%s'\n", lit);
        fprintf(stderr, "Labels must be resolved before parsing\n");
        exit(1);
    }
    
    // Handle hex (0x...)
    if (lit[0] == '0' && (lit[1] == 'x' || lit[1] == 'X')) {
        return strtol(lit, NULL, 16);
    }
    
    // Handle negative numbers
    if (lit[0] == '-') {
        return strtoull(lit, NULL, 0);
    }
    
    // Handle decimal
    return strtoull(lit, NULL, 0);
}

// Check if a string is a label reference
int isLabel(char * str) {
    if (str == NULL) return 0;
    str = trimWhitespace(str);
    return str[0] == ':';
}

// Parse three registers: "r0, r1, r3"
void parseThreeReg(char * args, int * rd, int * rs, int * rt) {
    char * argsCopy = strdup(args);
    char * token;
    
    token = strtok(argsCopy, ",");
    if (token) *rd = parseRegister(token);
    
    token = strtok(NULL, ",");
    if (token) *rs = parseRegister(token);
    
    token = strtok(NULL, ",");
    if (token) *rt = parseRegister(token);
    
    free(argsCopy);
}

// Parse two registers: "r5, r6"
void parseTwoReg(char * args, int * rd, int * rs) {
    char * argsCopy = strdup(args);
    char * token;
    
    token = strtok(argsCopy, ",");
    if (token) *rd = parseRegister(token);
    
    token = strtok(NULL, ",");
    if (token) *rs = parseRegister(token);
    
    free(argsCopy);
}

// Parse register and literal: "r5, 10"
void parseRegLit(char * args, int * rd, int * L) {
    char * argsCopy = strdup(args);
    char * token;
    
    token = strtok(argsCopy, ",");
    if (token) *rd = parseRegister(token);
    
    token = strtok(NULL, ",");
    if (token) *L = parseLiteral(token);
    
    free(argsCopy);
}

// Parse memory load: "r7, (r6)(0)"
void parseMemoryLoad(char * args, int * rd, int * rs, int * offset) {
    char * argsCopy = strdup(args);
    
    // Find the comma
    char * comma = strchr(argsCopy, ',');
    if (!comma) {
        fprintf(stderr, "Error: Invalid memory load format\n");
        exit(1);
    }
    
    *comma = '\0';
    *rd = parseRegister(argsCopy);
    
    // Now parse (rs)(offset)
    char * rest = comma + 1;
    rest = trimWhitespace(rest);
    
    // Find first (
    char * paren1 = strchr(rest, '(');
    if (!paren1) {
        fprintf(stderr, "Error: Invalid memory format\n");
        exit(1);
    }
    
    // Find first )
    char * paren2 = strchr(paren1, ')');
    if (!paren2) {
        fprintf(stderr, "Error: Invalid memory format\n");
        exit(1);
    }
    
    // Extract register between first ( and )
    *paren2 = '\0';
    *rs = parseRegister(paren1 + 1);
    
    // Find second (
    char * paren3 = strchr(paren2 + 1, '(');
    if (!paren3) {
        *offset = 0;
    } else {
        char * paren4 = strchr(paren3, ')');
        if (paren4) {
            *paren4 = '\0';
            *offset = parseLiteral(paren3 + 1);
        }
    }
    
    free(argsCopy);
}

// Parse memory store: "(r5)(8), r3"
void parseMemoryStore(char * args, int * rd, int * rs, int * offset) {
    char * argsCopy = strdup(args);
    
    // Find the comma
    char * comma = strchr(argsCopy, ',');
    if (!comma) {
        fprintf(stderr, "Error: Invalid memory store format\n");
        exit(1);
    }
    
    *comma = '\0';
    char * memPart = trimWhitespace(argsCopy);
    char * regPart = trimWhitespace(comma + 1);
    
    *rs = parseRegister(regPart);
    
    // Parse (rd)(offset)
    char * paren1 = strchr(memPart, '(');
    if (!paren1) {
        fprintf(stderr, "Error: Invalid memory format\n");
        exit(1);
    }
    
    char * paren2 = strchr(paren1, ')');
    if (!paren2) {
        fprintf(stderr, "Error: Invalid memory format\n");
        exit(1);
    }
    
    *paren2 = '\0';
    *rd = parseRegister(paren1 + 1);
    
    // Find second (
    char * paren3 = strchr(paren2 + 1, '(');
    if (!paren3) {
        *offset = 0;
    } else {
        char * paren4 = strchr(paren3, ')');
        if (paren4) {
            *paren4 = '\0';
            *offset = parseLiteral(paren3 + 1);
        }
    }
    
    free(argsCopy);
}

// Parse single register
int parseSingleReg(char * args) {
    return parseRegister(trimWhitespace(args));
}

// Extract command name from instruction line
char * extractCommandName(char * line) {
    char * lineCopy = strdup(line);
    char * space = strchr(lineCopy, ' ');
    
    if (space) {
        *space = '\0';
    }
    
    char * cmd = strdup(lineCopy);
    free(lineCopy);
    return cmd;
}

// Extract arguments from instruction line
char * extractArguments(char * line) {
    char * space = strchr(line, ' ');
    if (space) {
        return trimWhitespaceAlloc(space + 1);
    }
    // No arguments
    char * result = malloc(1);
    result[0] = '\0';
    return result;
}
