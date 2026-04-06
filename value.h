#ifndef kpp_value_h
#define kpp_value_h

#include <stdbool.h>
#include <stddef.h>

typedef enum
{
    VAL_NULL,
    VAL_BOOL,
    VAL_NUMBER,
    VAL_STRING,
    VAL_LIST,
    VAL_FUNCTION,
    VAL_NATIVE,
} ValueType;

// forward declare so Value can reference ObjFunction
typedef struct ObjFunction ObjFunction;
typedef struct ObjList ObjList;

typedef struct
{
    ValueType type;
    union
    {
        bool boolean;
        double number;
        char *string;
        ObjList *list;
        ObjFunction *function;
        void *native; // function pointer for native fns
    } as;
} Value;

// heap allocated list
struct ObjList
{
    Value *elements;
    int count;
    int capacity;
};

// heap allocated function
struct ObjFunction
{
    char *name;
    char **params;
    int param_count;
    void *chunk; // forward declare, chunk.h will fill this in
};

// convenience macros for building values
#define NULL_VAL() ((Value){VAL_NULL, {.number = 0}})
#define BOOL_VAL(v) ((Value){VAL_BOOL, {.boolean = v}})
#define NUMBER_VAL(v) ((Value){VAL_NUMBER, {.number = v}})
#define STRING_VAL(v) ((Value){VAL_STRING, {.string = v}})
#define LIST_VAL(v) ((Value){VAL_LIST, {.list = v}})
#define FUNCTION_VAL(v) ((Value){VAL_FUNCTION, {.function = v}})
#define NATIVE_VAL(v) ((Value){VAL_NATIVE, {.native = v}})

// convenience macros for reading values
#define AS_BOOL(v) ((v).as.boolean)
#define AS_NUMBER(v) ((v).as.number)
#define AS_STRING(v) ((v).as.string)
#define AS_LIST(v) ((v).as.list)
#define AS_FUNCTION(v) ((v).as.function)
#define AS_NATIVE(v) ((v).as.native)

// type checking macros
#define IS_NULL(v) ((v).type == VAL_NULL)
#define IS_BOOL(v) ((v).type == VAL_BOOL)
#define IS_NUMBER(v) ((v).type == VAL_NUMBER)
#define IS_STRING(v) ((v).type == VAL_STRING)
#define IS_LIST(v) ((v).type == VAL_LIST)
#define IS_FUNCTION(v) ((v).type == VAL_FUNCTION)
#define IS_NATIVE(v) ((v).type == VAL_NATIVE)

// native function signature
typedef Value (*NativeFn)(int arg_count, Value *args);

// list operations
ObjList *list_new();
void list_push(ObjList *list, Value value);
void list_free(ObjList *list);

// function operations
ObjFunction *function_new(const char *name, char **params, int param_count);
void function_free(ObjFunction *fn);

// value operations
void value_print(Value value);
bool value_truthy(Value value);
bool value_equal(Value a, Value b);
void value_free(Value value);
char *value_to_string(Value value);

#endif