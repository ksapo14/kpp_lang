#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "object.h"
#include "vm.h"

static uint32_t hashString(const char *key, int length)
{
    uint32_t hash = 2166136261u;
    for (int i = 0; i < length; i++)
    {
        hash ^= (uint8_t)key[i];
        hash *= 16777619;
    }
    return hash;
}

static Obj *allocateObject(size_t size, ObjType type)
{
    Obj *object = (Obj *)malloc(size);
    if (object == NULL)
    {
        fprintf(stderr, "Out of memory.\n");
        exit(1);
    }

    object->type = type;
    object->next = vm.objects;
    vm.objects = object;
    return object;
}

ObjFunction *newFunction(void)
{
    ObjFunction *function = (ObjFunction *)allocateObject(sizeof(ObjFunction), OBJ_FUNCTION);
    function->arity = 0;
    function->name = NULL;
    initChunk(&function->chunk);
    return function;
}

static ObjString *allocateString(char *chars, int length, uint32_t hash)
{
    ObjString *string = (ObjString *)allocateObject(sizeof(ObjString), OBJ_STRING);
    string->length = length;
    string->chars = chars;
    string->hash = hash;
    return string;
}

ObjString *takeString(char *chars, int length)
{
    return allocateString(chars, length, hashString(chars, length));
}

ObjString *copyString(const char *chars, int length)
{
    char *heapChars = (char *)malloc((size_t)length + 1);
    if (heapChars == NULL)
    {
        fprintf(stderr, "Out of memory.\n");
        exit(1);
    }

    memcpy(heapChars, chars, (size_t)length);
    heapChars[length] = '\0';
    return takeString(heapChars, length);
}

void printObject(Value value)
{
    switch (OBJ_TYPE(value))
    {
    case OBJ_FUNCTION:
    {
        ObjFunction *function = AS_FUNCTION(value);
        if (function->name == NULL)
            printf("<script>");
        else
            printf("<fun %s>", function->name->chars);
        break;
    }
    case OBJ_STRING:
        printf("%s", AS_CSTRING(value));
        break;
    }
}

void freeObjects(void)
{
    Obj *object = vm.objects;
    while (object != NULL)
    {
        Obj *next = object->next;
        switch (object->type)
        {
        case OBJ_FUNCTION:
        {
            ObjFunction *function = (ObjFunction *)object;
            freeChunk(&function->chunk);
            free(function);
            break;
        }
        case OBJ_STRING:
        {
            ObjString *string = (ObjString *)object;
            free(string->chars);
            free(string);
            break;
        }
        }
        object = next;
    }
    vm.objects = NULL;
}
