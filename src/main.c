#include "parse.h"
#include "string.h"
#include "macro.h"
#include "encode.h"
#include <stdlib.h>
#include <string.h>

// Steps:
// 1: Read the given file and get a script object
// 2: go through the commands and turn each entry into machine bytecode
// 3: output the binary into a new file

void expandMacros(Script * script) {
	int newNumEntries = 0;
	Entry * newEntries = malloc(10000000 * sizeof(Entry));
	for (int i = 0; i < script->numEntries; i++) {
		if (script->entries[i].type == 1) {
			newEntries[newNumEntries++] = script->entries[i];
			continue;
		}

		Entry * add = malloc(sizeof(Entry) * 5);
		int toAdd = expandMacro(&script->entries[i], add, script->ltable);

		for (int j = 0; j < toAdd; j++)
			newEntries[newNumEntries++] = add[j];
	}

	script->entries = newEntries;
	script->numEntries = newNumEntries;
}

void fillLabelTable(Script * script) {
	int address = 0x1000;
	for (int i = 0; i < script->numEntries; i++) {
		if (script->entries[i].type != 2) script->entries[i].address = address;
		if (script->entries[i].type == 2) {
			script->entries[i].address = address;
			insertLabel(script->entries[i].lbl, address, script->ltable);
		} else if (script->entries[i].type == 1) { 
			address += 8;
		}
		else if (script->entries[i].type == 0) {
			address += 4 * cmdTable[script->entries[i].cmd.type].cnt;
		}
	}
}

void replaceLabels(Script * script) {
	for (int i = 0; i < script->numEntries; i++) {
		Entry * entry = &script->entries[i];
		char * modified = malloc(50000 * sizeof(char));
		int len = 0;
		if (entry->str == null) continue;
		for (int j = 0; j < strlen(entry->str); j++)
			if (entry->str[j] == ':') {
				char buf[500];
				int k = j;
				while (entry->str[k] != ' ')
					buf[k - j] = entry->str[k], k++;
				buf[k] = '\0';
				uint64_t address = getintAddress(buf, script->ltable);
				char str[20];
				sprintf(str, "%llu", address);
				for (int m = 0; m < strlen(str); m++)
					modified[len++] = str[m];
				j = k-1;
			} else modified[len++] = entry->str[j];

		modified[len] = '\0';
		entry->str = modified;
	}
}

void printToIntermediate(Script * script, char * filename) {
	FILE * file = fopen(filename, "w");
	int mode = -1;
	int nmode = mode;

	int b = 0;
	for (int i = 0; i < script->numEntries; i++) {
		b |= (script->entries[i].type == 3);
	}
	if (!b) fprintf(file, ".code\n"), mode = 3;
	
	for (int i = 0; i < script->numEntries; i++) {
		Entry entry = script->entries[i];
		
		if (entry.type == 2) continue;
		
		if (entry.type == 1) { // data
			fprintf(file, "\t%llu\n", entry.value);
		} else if (entry.type == 0){ // code
			fprintf(file, "\t%s %s\n", cmdTable[entry.cmd.type].name, entry.str);
		} else if (entry.type == 3 && mode != 3) {
			fprintf(file, ".code\n");
			mode = entry.type;
		} else if (entry.type == 4 && mode != 4) {
			fprintf(file, ".data\n");
			mode = entry.type;
		}

	}
}

int * binNum(int num, int sz) {
	int * bin = malloc(sizeof(int)*(sz+1));
	bin[sz] = '\0';
	for (int i = 0; i < sz; i++) {
		bin[i] = (num%2);
		num>>=1;
	}
	return bin;
}

char * charNum(int num, int sz) {
	char * bin = malloc(sizeof(int)*(sz+1));
	bin[sz] = '\0';
	for (int i = 0; i < sz; i++) {
		bin[sz-i-1] = (num%2) ? '1' : '0';
		num>>=1;
	}
	return bin;
}

void printToBinary(Script * script, char * filename) {
	FILE * file = fopen(filename, "wb");
	for (int i = 0; i < script->numEntries; i++) {
		Entry entry = script->entries[i];
		if (entry.type == 2) continue;
		if (entry.type == 1) { // data
			fwrite(&entry.value, sizeof(long long), 1, file);
		} else if (entry.type == 0) { // instruction
									  //
			uint32_t x = getInstruction(&entry);
			fwrite(&x, sizeof(uint32_t), 1, file);
		}
	}
}

int main(int argc, char * argv[]) {
	Script * script = getScript(argv[1]);
	
	// 1: Intermediate file created
	fillLabelTable(script);

	for (int i = 0; i < script->numEntries; i++) {
		Entry entry = script->entries[i];


	}
	expandMacros(script);

	replaceLabels(script);

	printToIntermediate(script, argv[2]);
	printToBinary(script, argv[3]);
}
