#ifndef WIZARD_VM_H
#define WIZARD_VM_H

#include "chunk.h"

typedef struct{
    Chunk* Chunk;
} VM;

void initVM();
void freeVM();

#endif