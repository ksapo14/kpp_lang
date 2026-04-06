#ifndef kpp_vm_h
#define kpp_vm_h

#include "chunk.h"
#include "value.h"
#include "table.h"

#define STACK_MAX 256
#define FRAMES_MAX 64

typedef struct
{
    uint8_t *ip;   // instruction pointer
    Value *slots;  // pointer into vm stack for this frame
    Chunk *chunk;  // chunk being executed
    char **params; // parameter names for binding args
    int param_count;
} CallFrame;

typedef struct
{
    CallFrame frames[FRAMES_MAX];
    int frame_count;

    Value stack[STACK_MAX];
    Value *stack_top;

    Table globals; // global variable table
} VM;

typedef enum
{
    INTERPRET_OK,
    INTERPRET_COMPILE_ERROR,
    INTERPRET_RUNTIME_ERROR,
} InterpretResult;

void vm_init(VM *vm);
void vm_free(VM *vm);
InterpretResult vm_interpret(VM *vm, const char *source);

#endif