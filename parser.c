#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "parser.h"
#include "lexer.h"
#include "ast.h"

static Parser parser;

// helpers
static void error(const char *message)
{
    fprintf(stderr, "[line %d] Parse error: %s\n", parser.current.line, message);
    parser.had_error = true;
}

static Token advance()
{
    parser.previous = parser.current;
    parser.current = lexer_next_token();

    if (parser.current.type == TOKEN_ERROR)
    {
        error(parser.current.start);
    }

    return parser.previous;
}

static bool check(TokenType type)
{
    return parser.current.type == type;
}

static bool match(TokenType type)
{
    if (!check(type))
        return false;
    advance();
    return true;
}

static Token expect(TokenType type, const char *message)
{
    if (!check(type))
    {
        error(message);
    }
    return advance();
}

static void skip_newlines()
{
    while (check(TOKEN_NEWLINE))
        advance();
}

// forward declarations
static ASTNode *parse_statement();
static ASTNode *parse_expr();
static ASTNode *parse_primary();

// parse a block { ... }
static ASTNode *parse_block()
{
    ASTNode *block = ast_make_block(parser.previous.line);
    skip_newlines();

    while (!check(TOKEN_CLOSE_BRACE) && !check(TOKEN_EOF))
    {
        skip_newlines();
        if (check(TOKEN_CLOSE_BRACE))
            break;
        ASTNode *stmt = parse_statement();
        if (stmt)
            nodelist_push(&block->program.body, stmt);
        skip_newlines();
    }

    expect(TOKEN_CLOSE_BRACE, "Expected '}' after block.");
    return block;
}

// expressions — lowest to highest precedence
static ASTNode *parse_assignment()
{
    // check for identifier followed by assignment operator
    if (check(TOKEN_IDENTIFIER))
    {
        Token name = parser.current;

        // peek ahead — if next is = or compound, its assignment
        advance();

        if (match(TOKEN_EQUALS))
        {
            ASTNode *value = parse_expr();
            return ast_make_assign(name.start, name.length, value, name.line);
        }

        if (check(TOKEN_PLUS_EQUALS) || check(TOKEN_MINUS_EQUALS) ||
            check(TOKEN_STAR_EQUALS) || check(TOKEN_SLASH_EQUALS))
        {
            char op = parser.current.start[0]; // +, -, *, /
            advance();
            ASTNode *value = parse_expr();
            return ast_make_compound_assign(name.start, name.length, value, op, name.line);
        }

        if (match(TOKEN_PLUS_PLUS))
        {
            return ast_make_increment(name.start, name.length, 1, name.line);
        }

        if (match(TOKEN_MINUS_MINUS))
        {
            return ast_make_increment(name.start, name.length, -1, name.line);
        }

        // not an assignment — back up and parse as expr
        // we already consumed the identifier so build it manually
        ASTNode *ident = ast_make_identifier(name.start, name.length, name.line);

        // check for call
        if (match(TOKEN_OPEN_PAREN))
        {
            ASTNode *call = ast_make_call(ident, name.line);
            if (!check(TOKEN_CLOSE_PAREN))
            {
                do
                {
                    skip_newlines();
                    nodelist_push(&call->call.args, parse_expr());
                    skip_newlines();
                } while (match(TOKEN_COMMA));
            }
            expect(TOKEN_CLOSE_PAREN, "Expected ')' after arguments.");
            return call;
        }

        // check for index
        if (match(TOKEN_OPEN_BRACKET))
        {
            ASTNode *index = parse_expr();
            expect(TOKEN_CLOSE_BRACKET, "Expected ']' after index.");
            return ast_make_index(ident, index, name.line);
        }

        return ident;
    }

    return parse_expr();
}

static ASTNode *parse_comparison()
{
    ASTNode *left = parse_expr();
    // handled inside parse_expr via precedence climbing
    return left;
}

static ASTNode *parse_binary(int min_precedence);

static int get_precedence(TokenType type)
{
    switch (type)
    {
    case TOKEN_EQUAL_EQUAL:
    case TOKEN_BANG_EQUAL:
        return 1;
    case TOKEN_LESS:
    case TOKEN_LESS_EQUAL:
    case TOKEN_GREATER:
    case TOKEN_GREATER_EQUAL:
        return 2;
    case TOKEN_PLUS:
    case TOKEN_MINUS:
        return 3;
    case TOKEN_STAR:
    case TOKEN_SLASH:
    case TOKEN_PERCENT:
        return 4;
    default:
        return -1;
    }
}

static const char *token_op_string(TokenType type)
{
    switch (type)
    {
    case TOKEN_PLUS:
        return "+";
    case TOKEN_MINUS:
        return "-";
    case TOKEN_STAR:
        return "*";
    case TOKEN_SLASH:
        return "/";
    case TOKEN_PERCENT:
        return "%";
    case TOKEN_EQUAL_EQUAL:
        return "==";
    case TOKEN_BANG_EQUAL:
        return "!=";
    case TOKEN_LESS:
        return "<";
    case TOKEN_LESS_EQUAL:
        return "<=";
    case TOKEN_GREATER:
        return ">";
    case TOKEN_GREATER_EQUAL:
        return ">=";
    default:
        return "?";
    }
}

static ASTNode *parse_unary()
{
    if (match(TOKEN_MINUS))
    {
        int line = parser.previous.line;
        ASTNode *operand = parse_unary();
        return ast_make_unary(operand, '-', line);
    }
    return parse_primary();
}

static ASTNode *parse_binary(int min_prec)
{
    ASTNode *left = parse_unary();

    while (true)
    {
        int prec = get_precedence(parser.current.type);
        if (prec < min_prec)
            break;

        TokenType op_type = parser.current.type;
        const char *op = token_op_string(op_type);
        int line = parser.current.line;
        advance();

        ASTNode *right = parse_binary(prec + 1);
        left = ast_make_binary(left, right, op, line);
    }

    return left;
}

static ASTNode *parse_expr()
{
    return parse_binary(1);
}

static ASTNode *parse_primary()
{
    int line = parser.current.line;

    // number
    if (match(TOKEN_NUMBER))
    {
        double value = strtod(parser.previous.start, NULL);
        return ast_make_number(value, line);
    }

    // string
    if (match(TOKEN_STRING))
    {
        // strip quotes
        const char *start = parser.previous.start + 1;
        int length = parser.previous.length - 2;
        return ast_make_string(start, length, line);
    }

    // bool
    if (match(TOKEN_TRUE))
        return ast_make_bool(true, line);
    if (match(TOKEN_FALSE))
        return ast_make_bool(false, line);

    // null
    if (match(TOKEN_NULL))
        return ast_make_null(line);

    // list literal [1, 2, 3]
    if (match(TOKEN_OPEN_BRACKET))
    {
        ASTNode *list = ast_make_list(line);
        if (!check(TOKEN_CLOSE_BRACKET))
        {
            do
            {
                skip_newlines();
                nodelist_push(&list->list.elements, parse_expr());
                skip_newlines();
            } while (match(TOKEN_COMMA) && !check(TOKEN_CLOSE_BRACKET));
        }
        expect(TOKEN_CLOSE_BRACKET, "Expected ']' after list elements.");
        return list;
    }

    // grouped expr (...)
    if (match(TOKEN_OPEN_PAREN))
    {
        ASTNode *expr = parse_expr();
        expect(TOKEN_CLOSE_PAREN, "Expected ')' after expression.");
        return expr;
    }

    // identifier, call, or index
    if (match(TOKEN_IDENTIFIER))
    {
        ASTNode *ident = ast_make_identifier(
            parser.previous.start,
            parser.previous.length,
            line);

        // call
        if (match(TOKEN_OPEN_PAREN))
        {
            ASTNode *call = ast_make_call(ident, line);
            if (!check(TOKEN_CLOSE_PAREN))
            {
                do
                {
                    skip_newlines();
                    nodelist_push(&call->call.args, parse_expr());
                    skip_newlines();
                } while (match(TOKEN_COMMA));
            }
            expect(TOKEN_CLOSE_PAREN, "Expected ')' after arguments.");

            // chained index after call e.g. foo()[0]
            if (match(TOKEN_OPEN_BRACKET))
            {
                ASTNode *index = parse_expr();
                expect(TOKEN_CLOSE_BRACKET, "Expected ']' after index.");
                return ast_make_index(call, index, line);
            }

            return call;
        }

        // index
        if (match(TOKEN_OPEN_BRACKET))
        {
            ASTNode *index = parse_expr();
            expect(TOKEN_CLOSE_BRACKET, "Expected ']' after index.");
            return ast_make_index(ident, index, line);
        }

        return ident;
    }

    error("Expected expression.");
    advance();
    return ast_make_null(line);
}

// statements
static ASTNode *parse_var_declaration()
{
    int line = parser.previous.line;
    Token name = expect(TOKEN_IDENTIFIER, "Expected variable name after 'let'.");

    ASTNode *value = NULL;
    if (match(TOKEN_EQUALS))
    {
        value = parse_expr();
    }

    return ast_make_var_decl(name.start, name.length, value, line);
}

static ASTNode *parse_fn_declaration()
{
    int line = parser.previous.line;
    Token name = expect(TOKEN_IDENTIFIER, "Expected function name after 'fn'.");

    expect(TOKEN_OPEN_PAREN, "Expected '(' after function name.");

    char **params = NULL;
    int param_count = 0;
    int param_capacity = 0;

    if (!check(TOKEN_CLOSE_PAREN))
    {
        do
        {
            Token param = expect(TOKEN_IDENTIFIER, "Expected parameter name.");

            if (param_count >= param_capacity)
            {
                param_capacity = param_capacity == 0 ? 4 : param_capacity * 2;
                params = realloc(params, sizeof(char *) * param_capacity);
            }

            char *param_name = malloc(param.length + 1);
            memcpy(param_name, param.start, param.length);
            param_name[param.length] = '\0';
            params[param_count++] = param_name;

        } while (match(TOKEN_COMMA));
    }

    expect(TOKEN_CLOSE_PAREN, "Expected ')' after parameters.");
    expect(TOKEN_OPEN_BRACE, "Expected '{' before function body.");

    ASTNode *body = parse_block();

    return ast_make_function(name.start, name.length, params, param_count, body, line);
}

static ASTNode *parse_if_statement()
{
    int line = parser.previous.line;
    expect(TOKEN_OPEN_PAREN, "Expected '(' after 'if'.");
    ASTNode *condition = parse_expr();
    expect(TOKEN_CLOSE_PAREN, "Expected ')' after condition.");
    expect(TOKEN_OPEN_BRACE, "Expected '{' before if body.");
    ASTNode *then_branch = parse_block();

    ASTNode *else_branch = NULL;
    skip_newlines();
    if (match(TOKEN_ELSE))
    {
        if (match(TOKEN_IF))
        {
            else_branch = parse_if_statement(); // else if chaining
        }
        else
        {
            expect(TOKEN_OPEN_BRACE, "Expected '{' before else body.");
            else_branch = parse_block();
        }
    }

    return ast_make_if(condition, then_branch, else_branch, line);
}

static ASTNode *parse_while_statement()
{
    int line = parser.previous.line;
    expect(TOKEN_OPEN_PAREN, "Expected '(' after 'while'.");
    ASTNode *condition = parse_expr();
    expect(TOKEN_CLOSE_PAREN, "Expected ')' after condition.");
    expect(TOKEN_OPEN_BRACE, "Expected '{' before while body.");
    ASTNode *body = parse_block();
    return ast_make_while(condition, body, line);
}

static ASTNode *parse_for_statement()
{
    int line = parser.previous.line;
    expect(TOKEN_OPEN_PAREN, "Expected '(' after 'for'.");

    // init
    ASTNode *init = NULL;
    if (match(TOKEN_LET))
    {
        init = parse_var_declaration();
    }
    else
    {
        init = parse_assignment();
    }
    expect(TOKEN_SEMICOLON, "Expected ';' after for init.");

    // condition
    ASTNode *condition = parse_expr();
    expect(TOKEN_SEMICOLON, "Expected ';' after for condition.");

    // update
    ASTNode *update = parse_assignment();
    expect(TOKEN_CLOSE_PAREN, "Expected ')' after for update.");

    expect(TOKEN_OPEN_BRACE, "Expected '{' before for body.");
    ASTNode *body = parse_block();

    return ast_make_for(init, condition, update, body, line);
}

static ASTNode *parse_return_statement()
{
    int line = parser.previous.line;
    ASTNode *value = NULL;

    if (!check(TOKEN_NEWLINE) && !check(TOKEN_EOF))
    {
        value = parse_expr();
    }

    return ast_make_return(value, line);
}

static ASTNode *parse_statement()
{
    skip_newlines();

    if (match(TOKEN_LET))
        return parse_var_declaration();
    if (match(TOKEN_FN))
        return parse_fn_declaration();
    if (match(TOKEN_IF))
        return parse_if_statement();
    if (match(TOKEN_WHILE))
        return parse_while_statement();
    if (match(TOKEN_FOR))
        return parse_for_statement();
    if (match(TOKEN_RETURN))
        return parse_return_statement();

    // expression statement
    ASTNode *expr = parse_assignment();
    return ast_make_expr_stmt(expr, expr->line);
}

ASTNode *parser_parse(const char *source)
{
    lexer_init(source);
    parser.had_error = false;
    parser.current = lexer_next_token();

    ASTNode *program = ast_make_program(0);

    while (!check(TOKEN_EOF))
    {
        skip_newlines();
        if (check(TOKEN_EOF))
            break;
        ASTNode *stmt = parse_statement();
        if (stmt)
            nodelist_push(&program->program.body, stmt);
    }

    return parser.had_error ? NULL : program;
}