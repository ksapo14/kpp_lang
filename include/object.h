#ifndef KPP_OBJECT_H
#define KPP_OBJECT_H

#include "chunk.h"
#include "common.h"
#include "value.h"

typedef enum
{
    OBJ_FUNCTION,
    OBJ_STRING
} ObjType;

struct Obj
{
    ObjType type;
    struct Obj *next;
};

struct ObjString
{
    Obj obj;
    int length;
    char *chars;
    uint32_t hash;
};

struct ObjFunction
{
    Obj obj;
    int arity;
    Chunk chunk;
    ObjString *name;
};

#define OBJ_TYPE(value) (AS_OBJ(value)->type)

static inline bool isObjType(Value value, ObjType type)
{
    return IS_OBJ(value) && AS_OBJ(value)->type == type;
}

#define IS_FUNCTION(value) isObjType(value, OBJ_FUNCTION)
#define IS_STRING(value) isObjType(value, OBJ_STRING)

#define AS_FUNCTION(value) ((ObjFunction *)AS_OBJ(value))
#define AS_STRING(value) ((ObjString *)AS_OBJ(value))
#define AS_CSTRING(value) (((ObjString *)AS_OBJ(value))->chars)

ObjFunction *newFunction(void);
ObjString *takeString(char *chars, int length);
ObjString *copyString(const char *chars, int length);
void printObject(Value value);
void freeObjects(void);

#endif
