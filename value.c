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
  printf("%g", value);
}