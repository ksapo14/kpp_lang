#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "value.h"

// list operations
ObjList *list_new()
{
    ObjList *list = malloc(sizeof(ObjList));
    list->elements = NULL;
    list->count = 0;
    list->capacity = 0;
    return list;
}

void list_push(ObjList *list, Value value)
{
    if (list->count >= list->capacity)
    {
        list->capacity = list->capacity == 0 ? 8 : list->capacity * 2;
        list->elements = realloc(list->elements, sizeof(Value) * list->capacity);
    }
    list->elements[list->count++] = value;
}

void list_free(ObjList *list)
{
    for (int i = 0; i < list->count; i++)
    {
        value_free(list->elements[i]);
    }
    free(list->elements);
    free(list);
}

// function operations
ObjFunction *function_new(const char *name, char **params, int param_count)
{
    ObjFunction *fn = malloc(sizeof(ObjFunction));
    fn->name = strdup(name);
    fn->params = params;
    fn->param_count = param_count;
    fn->chunk = NULL; // set by compiler
    return fn;
}

void function_free(ObjFunction *fn)
{
    free(fn->name);
    for (int i = 0; i < fn->param_count; i++)
    {
        free(fn->params[i]);
    }
    free(fn->params);
    // chunk freed separately
    free(fn);
}

// value operations
bool value_truthy(Value value)
{
    switch (value.type)
    {
    case VAL_NULL:
        return false;
    case VAL_BOOL:
        return AS_BOOL(value);
    case VAL_NUMBER:
        return AS_NUMBER(value) != 0;
    case VAL_STRING:
        return AS_STRING(value)[0] != '\0';
    case VAL_LIST:
        return AS_LIST(value)->count > 0;
    default:
        return true;
    }
}

bool value_equal(Value a, Value b)
{
    if (a.type != b.type)
        return false;
    switch (a.type)
    {
    case VAL_NULL:
        return true;
    case VAL_BOOL:
        return AS_BOOL(a) == AS_BOOL(b);
    case VAL_NUMBER:
        return AS_NUMBER(a) == AS_NUMBER(b);
    case VAL_STRING:
        return strcmp(AS_STRING(a), AS_STRING(b)) == 0;
    default:
        return false;
    }
}

char *value_to_string(Value value)
{
    char buffer[64];
    switch (value.type)
    {
    case VAL_NULL:
        return strdup("null");
    case VAL_BOOL:
        return strdup(AS_BOOL(value) ? "true" : "false");
    case VAL_NUMBER:
        // strip trailing zeros e.g. 1.0 -> 1
        snprintf(buffer, sizeof(buffer), "%g", AS_NUMBER(value));
        return strdup(buffer);
    case VAL_STRING:
        return strdup(AS_STRING(value));
    case VAL_LIST:
    {
        // build [1, 2, 3] style string
        ObjList *list = AS_LIST(value);
        char *result = malloc(2);
        strcpy(result, "[");
        for (int i = 0; i < list->count; i++)
        {
            char *elem = value_to_string(list->elements[i]);
            size_t new_len = strlen(result) + strlen(elem) + 3;
            result = realloc(result, new_len);
            strcat(result, elem);
            free(elem);
            if (i < list->count - 1)
                strcat(result, ", ");
        }
        result = realloc(result, strlen(result) + 2);
        strcat(result, "]");
        return result;
    }
    case VAL_FUNCTION:
        snprintf(buffer, sizeof(buffer), "<fn %s>", AS_FUNCTION(value)->name);
        return strdup(buffer);
    case VAL_NATIVE:
        return strdup("<native fn>");
    default:
        return strdup("unknown");
    }
}

void value_print(Value value)
{
    char *str = value_to_string(value);
    printf("%s", str);
    free(str);
}

void value_free(Value value)
{
    switch (value.type)
    {
    case VAL_STRING:
        free(AS_STRING(value));
        break;
    case VAL_LIST:
        list_free(AS_LIST(value));
        break;
    case VAL_FUNCTION:
        function_free(AS_FUNCTION(value));
        break;
    default:
        break; // null, bool, number are stack allocated
    }
}