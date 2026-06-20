#include <stdlib.h>
#include <string.h>

#include "table.h"

#define TABLE_MAX_LOAD 0.75

static bool keysEqual(ObjString *a, ObjString *b)
{
    return a->length == b->length &&
           a->hash == b->hash &&
           memcmp(a->chars, b->chars, a->length) == 0;
}

static Entry *findEntry(Entry *entries, int capacity, ObjString *key)
{
    uint32_t index = key->hash & (uint32_t)(capacity - 1);
    for (;;)
    {
        Entry *entry = &entries[index];
        if (entry->key == NULL)
            return entry;

        if (keysEqual(entry->key, key))
            return entry;

        index = (index + 1) & (uint32_t)(capacity - 1);
    }
}

static void adjustCapacity(Table *table, int capacity)
{
    Entry *entries = (Entry *)malloc(sizeof(Entry) * (size_t)capacity);
    if (entries == NULL)
        exit(1);

    for (int i = 0; i < capacity; i++)
    {
        entries[i].key = NULL;
        entries[i].value = NIL_VAL;
    }

    table->count = 0;
    for (int i = 0; i < table->capacity; i++)
    {
        Entry *entry = &table->entries[i];
        if (entry->key == NULL)
            continue;

        Entry *dest = findEntry(entries, capacity, entry->key);
        dest->key = entry->key;
        dest->value = entry->value;
        table->count++;
    }

    free(table->entries);
    table->entries = entries;
    table->capacity = capacity;
}

void initTable(Table *table)
{
    table->count = 0;
    table->capacity = 0;
    table->entries = NULL;
}

void freeTable(Table *table)
{
    free(table->entries);
    initTable(table);
}

bool tableGet(Table *table, ObjString *key, Value *value)
{
    if (table->count == 0)
        return false;

    Entry *entry = findEntry(table->entries, table->capacity, key);
    if (entry->key == NULL)
        return false;

    *value = entry->value;
    return true;
}

bool tableSet(Table *table, ObjString *key, Value value)
{
    if (table->count + 1 > table->capacity * TABLE_MAX_LOAD)
    {
        int capacity = table->capacity < 8 ? 8 : table->capacity * 2;
        adjustCapacity(table, capacity);
    }

    Entry *entry = findEntry(table->entries, table->capacity, key);
    bool isNewKey = entry->key == NULL;
    if (isNewKey)
        table->count++;

    entry->key = key;
    entry->value = value;
    return isNewKey;
}
