#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include "table.h"
#include "value.h"

#define TABLE_MAX_LOAD 0.75

void table_init(Table *table)
{
    table->entries = NULL;
    table->count = 0;
    table->capacity = 0;
}

void table_free(Table *table)
{
    for (int i = 0; i < table->capacity; i++)
    {
        if (table->entries[i].occupied)
        {
            free(table->entries[i].key);
        }
    }
    free(table->entries);
    table_init(table);
}

static uint32_t hash_string(const char *key)
{
    uint32_t hash = 2166136261u;
    for (int i = 0; key[i] != '\0'; i++)
    {
        hash ^= (uint8_t)key[i];
        hash *= 16777619;
    }
    return hash;
}

static Entry *find_entry(Entry *entries, int capacity, const char *key)
{
    uint32_t index = hash_string(key) % capacity;

    while (true)
    {
        Entry *entry = &entries[index];
        if (!entry->occupied || strcmp(entry->key, key) == 0)
        {
            return entry;
        }
        index = (index + 1) % capacity; // linear probe
    }
}

static void grow_table(Table *table)
{
    int new_capacity = table->capacity == 0 ? 8 : table->capacity * 2;
    Entry *new_entries = calloc(new_capacity, sizeof(Entry));

    // rehash all existing entries
    for (int i = 0; i < table->capacity; i++)
    {
        Entry *src = &table->entries[i];
        if (!src->occupied)
            continue;

        Entry *dst = find_entry(new_entries, new_capacity, src->key);
        dst->key = src->key;
        dst->value = src->value;
        dst->occupied = true;
    }

    free(table->entries);
    table->entries = new_entries;
    table->capacity = new_capacity;
}

void table_set(Table *table, const char *key, Value value)
{
    if (table->count + 1 > table->capacity * TABLE_MAX_LOAD)
    {
        grow_table(table);
    }

    Entry *entry = find_entry(table->entries, table->capacity, key);

    if (!entry->occupied)
    {
        entry->key = strdup(key);
        table->count++;
        entry->occupied = true;
    }

    entry->value = value;
}

bool table_get(Table *table, const char *key, Value *out)
{
    if (table->count == 0)
        return false;

    Entry *entry = find_entry(table->entries, table->capacity, key);
    if (!entry->occupied)
        return false;

    *out = entry->value;
    return true;
}

bool table_has(Table *table, const char *key)
{
    Value dummy;
    return table_get(table, key, &dummy);
}

void table_delete(Table *table, const char *key)
{
    if (table->count == 0)
        return;
    Entry *entry = find_entry(table->entries, table->capacity, key);
    if (!entry->occupied)
        return;
    free(entry->key);
    entry->key = NULL;
    entry->occupied = false;
    table->count--;
}