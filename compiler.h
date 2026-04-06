#ifndef kpp_compiler_h
#define kpp_compiler_h

#include "ast.h"
#include "chunk.h"
#include "value.h"

typedef struct
{
    Chunk *chunk;
    bool had_error;
} Compiler;

Chunk *compiler_compile(ASTNode *program);

#endif