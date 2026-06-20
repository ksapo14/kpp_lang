#include <stdlib.h>

#include "chunk.h"

static void *resizeArray(void *pointer, size_t newCount, size_t size)
{
    if (newCount == 0)
    {
        free(pointer);
        return NULL;
    }
    return realloc(pointer, size * newCount);
}

void initChunk(Chunk *chunk)
{
    chunk->count = 0;
    chunk->capacity = 0;
    chunk->code = NULL;
    chunk->lines = NULL;
    chunk->columns = NULL;
    initValueArray(&chunk->constants);
}

void writeChunk(Chunk *chunk, uint8_t byte, int line, int column)
{
    if (chunk->capacity < chunk->count + 1)
    {
        int oldCapacity = chunk->capacity;
        chunk->capacity = oldCapacity < 8 ? 8 : oldCapacity * 2;
        chunk->code = resizeArray(chunk->code, chunk->capacity, sizeof(uint8_t));
        chunk->lines = resizeArray(chunk->lines, chunk->capacity, sizeof(int));
        chunk->columns = resizeArray(chunk->columns, chunk->capacity, sizeof(int));
    }

    chunk->code[chunk->count] = byte;
    chunk->lines[chunk->count] = line;
    chunk->columns[chunk->count] = column;
    chunk->count++;
}

int addConstant(Chunk *chunk, Value value)
{
    writeValueArray(&chunk->constants, value);
    return chunk->constants.count - 1;
}

void freeChunk(Chunk *chunk)
{
    resizeArray(chunk->code, 0, sizeof(uint8_t));
    resizeArray(chunk->lines, 0, sizeof(int));
    resizeArray(chunk->columns, 0, sizeof(int));
    freeValueArray(&chunk->constants);
    initChunk(chunk);
}
