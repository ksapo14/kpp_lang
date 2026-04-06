#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "compiler.h"
#include "ast.h"
#include "chunk.h"
#include "value.h"

static Compiler compiler;

// helpers
static void error(const char *message)
{
    fprintf(stderr, "Compiler error: %s\n", message);
    compiler.had_error = true;
}

static void emit_byte(uint8_t byte, int line)
{
    chunk_write(compiler.chunk, byte, line);
}

static void emit_two(uint8_t a, uint8_t b, int line)
{
    emit_byte(a, line);
    emit_byte(b, line);
}

static int emit_constant(Value value, int line)
{
    int idx = chunk_add_constant(compiler.chunk, value);
    emit_two(OP_PUSH, idx, line);
    return idx;
}

// emit a jump instruction, return the offset of the jump
// so we can patch it later once we know the jump target
static int emit_jump(uint8_t op, int line)
{
    emit_byte(op, line);
    emit_byte(0xff, line);            // placeholder high byte
    emit_byte(0xff, line);            // placeholder low byte
    return compiler.chunk->count - 2; // offset of the placeholder
}

// go back and fill in the jump target once we know it
static void patch_jump(int offset)
{
    int jump = compiler.chunk->count - offset - 2;

    if (jump > 0xffff)
    {
        error("Jump too large.");
        return;
    }

    compiler.chunk->code[offset] = (jump >> 8) & 0xff;
    compiler.chunk->code[offset + 1] = jump & 0xff;
}

// emit a loop instruction that jumps backwards
static void emit_loop(int loop_start, int line)
{
    emit_byte(OP_LOOP, line);

    int offset = compiler.chunk->count - loop_start + 2;
    if (offset > 0xffff)
    {
        error("Loop body too large.");
        return;
    }

    emit_byte((offset >> 8) & 0xff, line);
    emit_byte(offset & 0xff, line);
}

// add a string to the constant pool as a variable name
static int identifier_constant(const char *name, int line)
{
    char *copy = strdup(name);
    return chunk_add_constant(compiler.chunk, STRING_VAL(copy));
}

// forward declare
static void compile_node(ASTNode *node);
static void compile_stmt(ASTNode *node);
static void compile_expr(ASTNode *node);

// expressions
static void compile_number(ASTNode *node)
{
    emit_constant(NUMBER_VAL(node->number.value), node->line);
}

static void compile_string(ASTNode *node)
{
    char *copy = strdup(node->string.value);
    emit_constant(STRING_VAL(copy), node->line);
}

static void compile_bool(ASTNode *node)
{
    emit_byte(node->boolean.value ? OP_TRUE : OP_FALSE, node->line);
}

static void compile_null(ASTNode *node)
{
    emit_byte(OP_NULL, node->line);
}

static void compile_identifier(ASTNode *node)
{
    int idx = identifier_constant(node->identifier.name, node->line);
    emit_two(OP_GET_GLOBAL, idx, node->line);
}

static void compile_binary(ASTNode *node)
{
    compile_expr(node->binary.left);
    compile_expr(node->binary.right);

    const char *op = node->binary.operators;
    int line = node->line;

    if (strcmp(op, "+") == 0)
        emit_byte(OP_ADD, line);
    else if (strcmp(op, "-") == 0)
        emit_byte(OP_SUB, line);
    else if (strcmp(op, "*") == 0)
        emit_byte(OP_MUL, line);
    else if (strcmp(op, "/") == 0)
        emit_byte(OP_DIV, line);
    else if (strcmp(op, "%") == 0)
        emit_byte(OP_MOD, line);
    else if (strcmp(op, "==") == 0)
        emit_byte(OP_EQ, line);
    else if (strcmp(op, "!=") == 0)
        emit_byte(OP_NEQ, line);
    else if (strcmp(op, "<") == 0)
        emit_byte(OP_LT, line);
    else if (strcmp(op, ">") == 0)
        emit_byte(OP_GT, line);
    else if (strcmp(op, "<=") == 0)
        emit_byte(OP_LTE, line);
    else if (strcmp(op, ">=") == 0)
        emit_byte(OP_GTE, line);
    else
        error("Unknown binary operator.");
}

static void compile_unary(ASTNode *node)
{
    compile_expr(node->unary.operand);
    switch (node->unary.operators)
    {
    case '-':
        emit_byte(OP_NEGATE, node->line);
        break;
    case '!':
        emit_byte(OP_NOT, node->line);
        break;
    default:
        error("Unknown unary operator.");
        break;
    }
}

static void compile_list(ASTNode *node)
{
    // compile each element onto the stack
    NodeList *elements = &node->list.elements;
    for (int i = 0; i < elements->count; i++)
    {
        compile_expr(elements->statements[i]);
    }
    // OP_BUILD_LIST pops N elements and builds a list value
    emit_two(OP_BUILD_LIST, elements->count, node->line);
}

static void compile_index(ASTNode *node)
{
    compile_expr(node->index.list);
    compile_expr(node->index.index);
    emit_byte(OP_INDEX_GET, node->line);
}

static void compile_call(ASTNode *node)
{
    // push the function onto the stack first
    compile_expr(node->call.callee);

    // then push each argument
    NodeList *args = &node->call.args;
    for (int i = 0; i < args->count; i++)
    {
        compile_expr(args->statements[i]);
    }

    // OP_CALL takes the arg count as operand
    emit_two(OP_CALL, args->count, node->line);
}

static void compile_assign(ASTNode *node)
{
    compile_expr(node->assign.value);
    int idx = identifier_constant(node->assign.name, node->line);
    emit_two(OP_SET_GLOBAL, idx, node->line);
}

static void compile_compound_assign(ASTNode *node)
{
    // desugar x += y into x = x + y
    int idx = identifier_constant(node->compound_assign.name, node->line);
    emit_two(OP_GET_GLOBAL, idx, node->line);  // push current value
    compile_expr(node->compound_assign.value); // push rhs

    switch (node->compound_assign.operators)
    {
    case '+':
        emit_byte(OP_ADD, node->line);
        break;
    case '-':
        emit_byte(OP_SUB, node->line);
        break;
    case '*':
        emit_byte(OP_MUL, node->line);
        break;
    case '/':
        emit_byte(OP_DIV, node->line);
        break;
    default:
        error("Unknown compound assignment operator.");
        break;
    }

    emit_two(OP_SET_GLOBAL, idx, node->line);
}

static void compile_increment(ASTNode *node)
{
    // desugar x++ into x = x + 1
    int idx = identifier_constant(node->increment.name, node->line);
    emit_two(OP_GET_GLOBAL, idx, node->line);
    emit_constant(NUMBER_VAL(1.0), node->line);
    emit_byte(node->increment.delta == 1 ? OP_ADD : OP_SUB, node->line);
    emit_two(OP_SET_GLOBAL, idx, node->line);
}

static void compile_expr(ASTNode *node)
{
    switch (node->type)
    {
    case NODE_LITERAL_NUMBER:
        compile_number(node);
        break;
    case NODE_LITERAL_STRING:
        compile_string(node);
        break;
    case NODE_LITERAL_BOOL:
        compile_bool(node);
        break;
    case NODE_LITERAL_NULL:
        compile_null(node);
        break;
    case NODE_LITERAL_LIST:
        compile_list(node);
        break;
    case NODE_IDENTIFIER:
        compile_identifier(node);
        break;
    case NODE_BINARY:
        compile_binary(node);
        break;
    case NODE_UNARY:
        compile_unary(node);
        break;
    case NODE_INDEX:
        compile_index(node);
        break;
    case NODE_CALL:
        compile_call(node);
        break;
    case NODE_ASSIGN:
        compile_assign(node);
        break;
    case NODE_COMPOUND_ASSIGN:
        compile_compound_assign(node);
        break;
    case NODE_INCREMENT:
        compile_increment(node);
        break;
    default:
        error("Unknown expression node type.");
        break;
    }
}

// statements
static void compile_var_declaration(ASTNode *node)
{
    // compile value or push null if no initializer
    if (node->var_decl.value)
    {
        compile_expr(node->var_decl.value);
    }
    else
    {
        emit_byte(OP_NULL, node->line);
    }

    int idx = identifier_constant(node->var_decl.name, node->line);
    emit_two(OP_DEFINE_GLOBAL, idx, node->line);
}

static void compile_if(ASTNode *node)
{
    // compile condition
    compile_expr(node->if_stmt.condition);

    // jump past then branch if false
    int then_jump = emit_jump(OP_JUMP_IF_FALSE, node->line);
    emit_byte(OP_POP, node->line); // pop condition

    // compile then branch
    compile_stmt(node->if_stmt.then_branch);

    // jump past else branch after then
    int else_jump = emit_jump(OP_JUMP, node->line);

    // patch the then jump to here
    patch_jump(then_jump);
    emit_byte(OP_POP, node->line); // pop condition

    // compile else branch if present
    if (node->if_stmt.else_branch)
    {
        compile_stmt(node->if_stmt.else_branch);
    }

    patch_jump(else_jump);
}

static void compile_while(ASTNode *node)
{
    int loop_start = compiler.chunk->count;

    // compile condition
    compile_expr(node->while_stmt.condition);

    // jump past body if false
    int exit_jump = emit_jump(OP_JUMP_IF_FALSE, node->line);
    emit_byte(OP_POP, node->line); // pop condition

    // compile body
    compile_stmt(node->while_stmt.body);

    // loop back to condition
    emit_loop(loop_start, node->line);

    patch_jump(exit_jump);
    emit_byte(OP_POP, node->line); // pop condition
}

static void compile_for(ASTNode *node)
{
    // init
    if (node->for_stmt.init)
    {
        compile_stmt(node->for_stmt.init);
    }

    int loop_start = compiler.chunk->count;

    // condition
    int exit_jump = -1;
    if (node->for_stmt.condition)
    {
        compile_expr(node->for_stmt.condition);
        exit_jump = emit_jump(OP_JUMP_IF_FALSE, node->line);
        emit_byte(OP_POP, node->line);
    }

    // body
    compile_stmt(node->for_stmt.body);

    // update
    if (node->for_stmt.update)
    {
        compile_expr(node->for_stmt.update);
        emit_byte(OP_POP, node->line); // discard update result
    }

    // loop back
    emit_loop(loop_start, node->line);

    if (exit_jump != -1)
    {
        patch_jump(exit_jump);
        emit_byte(OP_POP, node->line);
    }
}

static void compile_return(ASTNode *node)
{
    if (node->return_stmt.value)
    {
        compile_expr(node->return_stmt.value);
    }
    else
    {
        emit_byte(OP_NULL, node->line);
    }
    emit_byte(OP_RETURN, node->line);
}

static void compile_function(ASTNode *node)
{
    // save current chunk and compile function body into a new chunk
    Chunk *outer_chunk = compiler.chunk;

    Chunk *fn_chunk = malloc(sizeof(Chunk));
    chunk_init(fn_chunk);
    compiler.chunk = fn_chunk;

    // compile the body
    NodeList *body = &node->function.body->program.body;
    for (int i = 0; i < body->count; i++)
    {
        compile_stmt(body->statements[i]);
    }

    // implicit null return at end of function
    emit_byte(OP_NULL, node->line);
    emit_byte(OP_RETURN, node->line);

    // restore outer chunk
    compiler.chunk = outer_chunk;

    // build the function value
    char **params = malloc(sizeof(char *) * node->function.param_count);
    for (int i = 0; i < node->function.param_count; i++)
    {
        params[i] = strdup(node->function.params[i]);
    }

    ObjFunction *fn = function_new(node->function.name, params, node->function.param_count);
    fn->chunk = fn_chunk;

    // push function as constant and define it as a global
    int idx = chunk_add_constant(compiler.chunk, FUNCTION_VAL(fn));
    emit_two(OP_PUSH, idx, node->line);
    int name_idx = identifier_constant(node->function.name, node->line);
    emit_two(OP_DEFINE_GLOBAL, name_idx, node->line);
}

static void compile_block(ASTNode *node)
{
    NodeList *body = &node->program.body;
    for (int i = 0; i < body->count; i++)
    {
        compile_stmt(body->statements[i]);
    }
}

static void compile_stmt(ASTNode *node)
{
    switch (node->type)
    {
    case NODE_VAR_DECLARATION:
        compile_var_declaration(node);
        break;
    case NODE_FUNCTION:
        compile_function(node);
        break;
    case NODE_IF:
        compile_if(node);
        break;
    case NODE_WHILE:
        compile_while(node);
        break;
    case NODE_FOR:
        compile_for(node);
        break;
    case NODE_RETURN:
        compile_return(node);
        break;
    case NODE_BLOCK:
        compile_block(node);
        break;

    case NODE_EXPR_STMT:
        compile_expr(node->expr_stmt.expr);
        emit_byte(OP_POP, node->line); // discard result
        break;

    default:
        error("Unknown statement node type.");
        break;
    }
}

static void compile_node(ASTNode *node)
{
    if (node->type == NODE_PROGRAM)
    {
        NodeList *body = &node->program.body;
        for (int i = 0; i < body->count; i++)
        {
            compile_stmt(body->statements[i]);
        }
        emit_byte(OP_HALT, 0);
    }
    else
    {
        compile_stmt(node);
    }
}

Chunk *compiler_compile(ASTNode *program)
{
    Chunk *chunk = malloc(sizeof(Chunk));
    chunk_init(chunk);

    compiler.chunk = chunk;
    compiler.had_error = false;

    compile_node(program);

    if (compiler.had_error)
    {
        chunk_free(chunk);
        free(chunk);
        return NULL;
    }

    return chunk;
}