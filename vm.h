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

  /* Keeps the UI's most recently compiled program alive across restarts. */
  ObjFunction* preparedFunction;

} VM;

typedef void (*VMOutputFn)(const char* text);
typedef void (*VMErrorFn)(const char* text);

typedef enum {
  INTERPRET_RUNNING,
  INTERPRET_OK,
  INTERPRET_COMPILE_ERROR,
  INTERPRET_RUNTIME_ERROR
} InterpretResult;


extern VM vm;

void initVM();
void freeVM();
InterpretResult interpret(const char* source);
/* Starts and advances an execution without blocking the UI event loop. */
InterpretResult beginInterpret(const char* source);
InterpretResult stepVM(void);
bool vmIsRunning(void);
/* UI-oriented API: compile once, then restart the retained bytecode. */
InterpretResult vmCompileSource(const char* source);
InterpretResult vmRestartPrepared(void);
bool vmHasPreparedProgram(void);
void vmSetOutputCallback(VMOutputFn callback);
void vmSetErrorCallback(VMErrorFn callback);
void vmReportError(const char* format, ...);
void push(Valux value);
Valux pop();

#endif
