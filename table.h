#ifndef kpp_table_h
#define kpp_table_h

#include "value.h"
#include <stdbool.h>

typedef struct
{
    char *key;
    Value value;
    bool occupied;
} Entry;

typedef struct
{
    Entry *entries;
    int count;
    int capacity;
} Table;

void table_init(Table *table);
void table_free(Table *table);
bool table_get(Table *table, const char *key, Value *out);
void table_set(Table *table, const char *key, Value value);
bool table_has(Table *table, const char *key);
void table_delete(Table *table, const char *key);

#endif