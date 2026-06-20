#ifndef KPP_TABLE_H
#define KPP_TABLE_H

#include "common.h"
#include "object.h"
#include "value.h"

typedef struct
{
    ObjString *key;
    Value value;
} Entry;

typedef struct
{
    int count;
    int capacity;
    Entry *entries;
} Table;

void initTable(Table *table);
void freeTable(Table *table);
bool tableGet(Table *table, ObjString *key, Value *value);
bool tableSet(Table *table, ObjString *key, Value value);

#endif
