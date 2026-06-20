#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "object.h"
#include "value.h"

static void *growArray(void *pointer, size_t oldCount, size_t newCount, size_t size)
{
    (void)oldCount;
    if (newCount == 0)
    {
        free(pointer);
        return NULL;
    }
    return realloc(pointer, size * newCount);
}

void initValueArray(ValueArray *array)
{
    array->capacity = 0;
    array->count = 0;
    array->values = NULL;
}

void writeValueArray(ValueArray *array, Value value)
{
    if (array->capacity < array->count + 1)
    {
        int oldCapacity = array->capacity;
        array->capacity = oldCapacity < 8 ? 8 : oldCapacity * 2;
        array->values = growArray(array->values, oldCapacity, array->capacity, sizeof(Value));
    }

    array->values[array->count] = value;
    array->count++;
}

void freeValueArray(ValueArray *array)
{
    growArray(array->values, array->capacity, 0, sizeof(Value));
    initValueArray(array);
}

bool valuesEqual(Value a, Value b)
{
    if (a.type != b.type)
        return false;

    switch (a.type)
    {
    case VAL_BOOL:
        return AS_BOOL(a) == AS_BOOL(b);
    case VAL_NIL:
        return true;
    case VAL_NUMBER:
        return AS_NUMBER(a) == AS_NUMBER(b);
    case VAL_OBJ:
        if (OBJ_TYPE(a) != OBJ_TYPE(b))
            return false;
        if (IS_STRING(a))
        {
            ObjString *aString = AS_STRING(a);
            ObjString *bString = AS_STRING(b);
            return aString->length == bString->length &&
                   aString->hash == bString->hash &&
                   memcmp(aString->chars, bString->chars, aString->length) == 0;
        }
        return AS_OBJ(a) == AS_OBJ(b);
    }

    return false;
}

void printValue(Value value)
{
    switch (value.type)
    {
    case VAL_BOOL:
        printf(AS_BOOL(value) ? "true" : "false");
        break;
    case VAL_NIL:
        printf("nil");
        break;
    case VAL_NUMBER:
        printf("%g", AS_NUMBER(value));
        break;
    case VAL_OBJ:
        printObject(value);
        break;
    }
}
