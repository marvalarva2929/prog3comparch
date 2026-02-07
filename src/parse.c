#include "parse.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h>

// FIXED: Properly trim leading and trailing whitespace
char * trim(char * totrim) {
    if (totrim == NULL) return NULL;
    
    // Find first non-whitespace character
    int l = 0;
    while (totrim[l] && isspace((unsigned char)totrim[l])) {
        l++;
    }
    
    // If entire string is whitespace, return empty string
    if (totrim[l] == '\0') {
        char * result = malloc(1);
        result[0] = '\0';
        return result;
    }
    
    // Find last non-whitespace character
    int r = strlen(totrim) - 1;
    while (r > l && isspace((unsigned char)totrim[r])) {
        r--;
    }
    
    // Allocate memory for trimmed string
    int length = r - l + 1;
    char * ret = malloc((length + 1) * sizeof(char));
    
    // Copy trimmed content
    strncpy(ret, totrim + l, length);
    ret[length] = '\0';
    
    return ret;
}

static CommandType lookupCommand(const char *cmd) {
    size_t n = sizeof(cmdTable) / sizeof(cmdTable[0]);
    for (size_t i = 0; i < n; i++) {
        if (strcmp(cmd, cmdTable[i].name) == 0)
            return cmdTable[i].type;
    }
	fprintf(stderr, "unknown command %s\n", cmd);
	exit(1);
}

Entry * handleData(char * dataline, int address) {
	Entry * ret = malloc(sizeof(Entry));
	ret->address = address;
	ret->size = 8; 
	ret->type = 1;
	if (trim(dataline)[0] == '-') {
		fprintf(stderr, "no negatives allowed\n");
		exit(1);
	}
	char * ptr;

	ret->value = strtoull(dataline, &ptr, 10);
	if (*ptr != '\0' || ptr == dataline) {
		fprintf(stderr, "invalid data\n");
		exit(1);
	}
	if (errno == ERANGE) {
		fprintf(stderr, "data exceeds maximum limit\n");
		exit(1);
	}
	return ret;
}

Entry * handleCmd(char * line, int address) {
	Entry * newEntry = malloc(sizeof(Entry));
	char * cmd = extractCommandName(line);
	char * args = extractArguments(line);
	newEntry->str = args;
	newEntry->address = address;
	newEntry->cmd.type = lookupCommand(cmd);
	free(cmd);  // Free the command string since we don't need it anymore
	return newEntry;
}

Script * getScript(char * filename) {

	Script * ret = malloc(sizeof(Script));
	FILE * file = fopen(filename, "r");
	ltable * table = malloc(sizeof(ltable));
	ret->ltable = table;
	char line[5000];

	int numEntries = 0;
	Entry * allEntries = malloc(50000 * sizeof(Entry));	

	// first passthrough:
	// 1: Create label table
	// 2: Expand macros
	// 3: create a sequential list of the instructions/data
	// 4: return a script object that contains a list of all entries 
	
	// first passthrough, create label table and expand macros
	int mode = -1; // 0 for code, 1 for data
	int address = 0x1000;
	
	while (fgets(line, sizeof(line), file) != NULL) {
		int val;
		Entry * entry = NULL;
		switch (line[0]) {
			case '\t': // save either the data or instruction at the current address and increment counter
				entry = malloc(sizeof(Entry));
				if (mode) {
					entry = handleData(trim(line), address);
					address += 8;
				} else {
					entry = handleCmd(trim(line), address);
					address += 4;
				}
				break;
				
			case ':': // save this label as the current address, but don't increment current counter
				entry = malloc(sizeof(Entry));
				char * label = trim(line);
				entry->size = 0;
				entry->type = 2;
				entry->lbl = label;
				break;

			case '.': // switch modes
				if (line[1] == 'd') mode = 1;
				else mode = 0;
				entry = malloc(sizeof(Entry));
				entry->type = 3 + mode; // 3 for code, 4 for data
				break;
		}


		if (entry) allEntries[numEntries++] = *entry;
	}

	Entry * orderedEntries = malloc(numEntries * sizeof(Entry));
	for (int i = 0; i < numEntries; i++)
		orderedEntries[i] = allEntries[i];

	ret->numEntries = numEntries;
	ret->entries = orderedEntries;

	
	fclose(file);
	return ret;
}

void printScript(Script * script, char * outputFile) { // prints to a new file the parsed assembly code with macros expanded and labels replaced 
};
