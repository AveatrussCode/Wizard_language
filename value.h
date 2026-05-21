#ifndef WIZARD_VALUE_H
#define WIZARD_VALUE_H

#include "common.h"

typedef double Valux;

typedef struct {
  int capacity;
  int count;
  Valux* values;
} ValuxArray;

void initValuxArray(ValuxArray* array);
void writeValuxArray(ValuxArray* array, Valux value);
void freeValuxArray(ValuxArray* array);
void printValue(Valux value);

#endif