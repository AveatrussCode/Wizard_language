#ifndef WIZARD_VM_H
#define WIZARD_VM_H

#include "chunk.h"
#include "value.h"
#define STACK_MAX 256

typedef struct{
    Chunk* chunk;
    uint8_t* ip;
    Valux stack[STACK_MAX];
    Valux* stackTop;
} VM;

typedef enum {
  INTERPRET_OK,
  INTERPRET_COMPILE_ERROR,
  INTERPRET_RUNTIME_ERROR
} InterpretResult;

void initVM();
void freeVM();
InterpretResult interpret(Chunk* chunk);
void push(Valux value);
Valux pop();

#endif