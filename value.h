#ifndef WIZARD_VALUE_H
#define WIZARD_VALUE_H

#include "common.h"

typedef struct Obj Obj;
typedef struct ObjString ObjString;



typedef enum {
  VAL_BOOL,
  VAL_NIL, 
  VAL_NUMBER,
  VAL_OBJ,
} ValueType;


typedef struct {
  ValueType type;
  union {
    bool boolean;
    double number;
    Obj* obj;
  } as; 
} Valux;

#define IS_BOOL(value)    ((value).type == VAL_BOOL)
#define IS_NIL(value)     ((value).type == VAL_NIL)
#define IS_NUMBER(value)  ((value).type == VAL_NUMBER)
#define IS_OBJ(value)     ((value).type == VAL_OBJ)

#define AS_OBJ(value)     ((value).as.obj)
#define AS_BOOL(value)    ((value).as.boolean)
#define AS_NUMBER(value)  ((value).as.number)


#define BOOL_VAL(value)   ((Valux){VAL_BOOL, {.boolean = value}})
#define NIL_VAL           ((Valux){VAL_NIL, {.number = 0}})
#define NUMBER_VAL(value) ((Valux){VAL_NUMBER, {.number = value}})
#define OBJ_VAL(object)   ((Valux){VAL_OBJ, {.obj = (Obj*)object}})

typedef struct {
  int capacity;
  int count;
  Valux* values;
} ValuxArray;

void initValuxArray(ValuxArray* array);
void writeValuxArray(ValuxArray* array, Valux value);
void freeValuxArray(ValuxArray* array);
void printValue(Valux value);
bool valuesEqual(Valux a, Valux b);

#endif