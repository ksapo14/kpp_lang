#ifndef kpp_chunk_h
#define kpp_chunk_h

#include <stdint.h>
#include "value.h"

typedef enum
{
    OP_PUSH,
    OP_POP,
    OP_NULL,
    OP_TRUE,
    OP_FALSE,

    // arithmetic
    OP_ADD,
    OP_SUB,
    OP_MUL,
    OP_DIV,
    OP_MOD,
    OP_NEGATE,

    // comparison
    OP_EQ,
    OP_NEQ,
    OP_LT,
    OP_GT,
    OP_LTE,
    OP_GTE,

    // logical
    OP_NOT,

    // variables
    OP_DEFINE_GLOBAL,
    OP_GET_GLOBAL,
    OP_SET_GLOBAL,

    // control flow
    OP_JUMP,
    OP_JUMP_IF_FALSE,
    OP_LOOP,

    // functions
    OP_CALL,
    OP_RETURN,

    // lists
    OP_BUILD_LIST,
    OP_INDEX_GET,
    OP_INDEX_SET,

    // misc
    OP_PRINT,
    OP_HALT,
} OpCode;

typedef struct
{
    uint8_t *code; // raw bytecode
    int *lines;    // line number per instruction for errors
    int count;
    int capacity;

    Value *constants; // constant pool
    int const_count;
    int const_capacity;
} Chunk;

void chunk_init(Chunk *chunk);
void chunk_free(Chunk *chunk);
void chunk_write(Chunk *chunk, uint8_t byte, int line);
int chunk_add_constant(Chunk *chunk, Value value);
void chunk_disassemble(Chunk *chunk, const char *name);      // debug helper
int chunk_disassemble_instruction(Chunk *chunk, int offset); // debug helper

#endif