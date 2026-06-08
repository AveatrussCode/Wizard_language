#include <stdio.h>
#include "memory.h"
#include "value.h"

void initValuxArray(ValuxArray* array){
    array->values = NULL;
    array->capacity = 0;
    array->count = 0;
}
void writeValuxArray(ValuxArray* array, Valux value){
    if(array->capacity < array->count +1){
        int oldCapacity = array->capacity;
        array->capacity = GROW_CAPACITY(oldCapacity);
        array->values = GROW_ARRAY(Valux, array->values, oldCapacity, array->capacity);
    }
    array->values[array->count] = value;
    array->count++;
}
void freeValuxArray(ValuxArray* array){
    FREE_ARRAY(Valux, array->values, array->capacity);
    initValuxArray(array);
}
void printValue(Valux value) {
    switch (value.type) {
        case VAL_BOOL:
            printf(AS_BOOL(value) ? "true" : "false");
            break;
        case VAL_NIL: printf("nil"); break;
        case VAL_NUMBER: printf("%g", AS_NUMBER(value)); break;
    }
}
bool valuesEqual(Valux a, Valux b) {
  if (a.type != b.type) return false;
  switch (a.type) {
    case VAL_BOOL:   return AS_BOOL(a) == AS_BOOL(b);
    case VAL_NIL:    return true;
    case VAL_NUMBER: return AS_NUMBER(a) == AS_NUMBER(b);
    default:         return false; // Unreachable.
  }
}