#pragma once
#include <stdio.h>
#include <inttypes.h>

#define MAX_LABELS 100000

typedef struct ltable {
    char labels[MAX_LABELS][64];  // Array of label strings
    uint64_t addresses[MAX_LABELS];     // Array of addresses
    int count;                     // Number of labels
} ltable;

uint64_t getintAddress(char * label, ltable *table);
void insertLabel(char * label, uint64_t address, ltable *table);


