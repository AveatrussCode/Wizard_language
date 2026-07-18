#ifndef WIZARD_TABLE_H
#define WIZARD_TABLE_H 

#include "common.h"
#include "value.h"

typedef struct {
  ObjString* key;
  Valux value;
} Entry;

typedef struct {
  int count;
  int capacity;
  Entry* entries;
} Table;

void initTable(Table* table);
void freeTable(Table* table);
bool tableGet(Table* table, ObjString* key, Valux* value);
bool tableSet(Table* table, ObjString* key, Valux value);
bool tableDelete(Table* table, ObjString* key);
void tableAddAll(Table* from, Table* to);
ObjString* tableFindString(Table* table, const char* chars, int length, uint32_t hash);
void tableRemoveWhite(Table* table);
void markTable(Table* table);
#endif
