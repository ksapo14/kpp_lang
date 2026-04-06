#include <stdio.h>
#include <stdlib.h>
#include "chunk.h"

void chunk_init(Chunk *chunk)
{
    chunk->code = NULL;
    chunk->lines = NULL;
    chunk->count = 0;
    chunk->capacity = 0;
    chunk->constants = NULL;
    chunk->const_count = 0;
    chunk->const_capacity = 0;
}

void chunk_free(Chunk *chunk)
{
    free(chunk->code);
    free(chunk->lines);
    for (int i = 0; i < chunk->const_count; i++)
    {
        value_free(chunk->constants[i]);
    }
    free(chunk->constants);
    chunk_init(chunk); // reset to clean state
}

void chunk_write(Chunk *chunk, uint8_t byte, int line)
{
    if (chunk->count >= chunk->capacity)
    {
        chunk->capacity = chunk->capacity == 0 ? 8 : chunk->capacity * 2;
        chunk->code = realloc(chunk->code, sizeof(uint8_t) * chunk->capacity);
        chunk->lines = realloc(chunk->lines, sizeof(int) * chunk->capacity);
    }
    chunk->code[chunk->count] = byte;
    chunk->lines[chunk->count] = line;
    chunk->count++;
}

int chunk_add_constant(Chunk *chunk, Value value)
{
    if (chunk->const_count >= chunk->const_capacity)
    {
        chunk->const_capacity = chunk->const_capacity == 0 ? 8 : chunk->const_capacity * 2;
        chunk->constants = realloc(chunk->constants, sizeof(Value) * chunk->const_capacity);
    }
    chunk->constants[chunk->const_count++] = value;
    return chunk->const_count - 1; // return index
}

// debug helpers
static const char *opcode_name(uint8_t op)
{
    switch (op)
    {
    case OP_PUSH:
        return "OP_PUSH";
    case OP_POP:
        return "OP_POP";
    case OP_NULL:
        return "OP_NULL";
    case OP_TRUE:
        return "OP_TRUE";
    case OP_FALSE:
        return "OP_FALSE";
    case OP_ADD:
        return "OP_ADD";
    case OP_SUB:
        return "OP_SUB";
    case OP_MUL:
        return "OP_MUL";
    case OP_DIV:
        return "OP_DIV";
    case OP_MOD:
        return "OP_MOD";
    case OP_NEGATE:
        return "OP_NEGATE";
    case OP_EQ:
        return "OP_EQ";
    case OP_NEQ:
        return "OP_NEQ";
    case OP_LT:
        return "OP_LT";
    case OP_GT:
        return "OP_GT";
    case OP_LTE:
        return "OP_LTE";
    case OP_GTE:
        return "OP_GTE";
    case OP_NOT:
        return "OP_NOT";
    case OP_DEFINE_GLOBAL:
        return "OP_DEFINE_GLOBAL";
    case OP_GET_GLOBAL:
        return "OP_GET_GLOBAL";
    case OP_SET_GLOBAL:
        return "OP_SET_GLOBAL";
    case OP_JUMP:
        return "OP_JUMP";
    case OP_JUMP_IF_FALSE:
        return "OP_JUMP_IF_FALSE";
    case OP_LOOP:
        return "OP_LOOP";
    case OP_CALL:
        return "OP_CALL";
    case OP_RETURN:
        return "OP_RETURN";
    case OP_BUILD_LIST:
        return "OP_BUILD_LIST";
    case OP_INDEX_GET:
        return "OP_INDEX_GET";
    case OP_INDEX_SET:
        return "OP_INDEX_SET";
    case OP_PRINT:
        return "OP_PRINT";
    case OP_HALT:
        return "OP_HALT";
    default:
        return "UNKNOWN";
    }
}

int chunk_disassemble_instruction(Chunk *chunk, int offset)
{
    printf("%04d ", offset);

    // print line number
    if (offset > 0 && chunk->lines[offset] == chunk->lines[offset - 1])
    {
        printf("   | ");
    }
    else
    {
        printf("%4d ", chunk->lines[offset]);
    }

    uint8_t op = chunk->code[offset];
    printf("%-20s", opcode_name(op));

    switch (op)
    {
    // instructions with a constant index operand
    case OP_PUSH:
    case OP_DEFINE_GLOBAL:
    case OP_GET_GLOBAL:
    case OP_SET_GLOBAL:
    {
        int idx = chunk->code[offset + 1];
        printf(" %4d '", idx);
        value_print(chunk->constants[idx]);
        printf("'\n");
        return offset + 2;
    }

    // instructions with a jump offset operand
    case OP_JUMP:
    case OP_JUMP_IF_FALSE:
    case OP_LOOP:
    {
        int jump = (chunk->code[offset + 1] << 8) | chunk->code[offset + 2];
        printf(" %4d\n", jump);
        return offset + 3;
    }

    // instructions with a count operand
    case OP_CALL:
    case OP_BUILD_LIST:
    {
        printf(" %4d\n", chunk->code[offset + 1]);
        return offset + 2;
    }

    // simple instructions
    default:
        printf("\n");
        return offset + 1;
    }
}

void chunk_disassemble(Chunk *chunk, const char *name)
{
    printf("=== %s ===\n", name);
    int offset = 0;
    while (offset < chunk->count)
    {
        offset = chunk_disassemble_instruction(chunk, offset);
    }
}