#ifndef WIZARD_VM_H
#define WIZARD_VM_H

#include "object.h"
#include "table.h"
#include "value.h"

#define FRAMES_MAX 64
#define STACK_MAX (FRAMES_MAX * UINT8_COUNT)

typedef struct {
  ObjClosure* closure;
  uint8_t* ip;
  Valux* slots;
} CallFrame;

typedef struct{
  CallFrame frames[FRAMES_MAX];
  int frameCount;
  Valux stack[STACK_MAX];
  Valux* stackTop;
  Table globals;
  Table strings;
  ObjString* initString;
  ObjUpvalue* openUpvalues;

  size_t bytesAllocated;
  size_t nextGC;
  Obj* objects;
  int grayCount;
  int grayCapacity;
  Obj** grayStack;

} VM;

typedef enum {
  INTERPRET_OK,
  INTERPRET_COMPILE_ERROR,
  INTERPRET_RUNTIME_ERROR
} InterpretResult;


extern VM vm;

void initVM();
void freeVM();
InterpretResult interpret(const char* source);
void push(Valux value);
Valux pop();

#endif
