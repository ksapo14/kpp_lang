#ifndef kpp_parser_h
#define kpp_parser_h

#include "ast.h"
#include "lexer.h"

typedef struct
{
    Token current;
    Token previous;
    bool had_error;
} Parser;

ASTNode *parser_parse(const char *source);

#endif