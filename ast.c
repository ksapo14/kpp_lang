#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "ast.h"

static ASTNode *make_node(NodeType type, int line)
{
    ASTNode *node = malloc(sizeof(ASTNode));
    node->type = type;
    node->line = line;
    return node;
}

void nodelist_push(NodeList *list, ASTNode *node)
{
    if (list->count >= list->capacity)
    {
        list->capacity = list->capacity == 0 ? 8 : list->capacity * 2;
        list->statements = realloc(list->statements, sizeof(ASTNode *) * list->capacity);
    }
    list->statements[list->count++] = node;
}

ASTNode *ast_make_number(double value, int line)
{
    ASTNode *node = make_node(NODE_LITERAL_NUMBER, line);
    node->number.value = value;
    return node;
}

ASTNode *ast_make_string(const char *start, int length, int line)
{
    ASTNode *node = make_node(NODE_LITERAL_STRING, line);
    node->string.value = malloc(length + 1);
    memcpy(node->string.value, start, length);
    node->string.value[length] = '\0';
    node->string.length = length;
    return node;
}

ASTNode *ast_make_bool(bool value, int line)
{
    ASTNode *node = make_node(NODE_LITERAL_BOOL, line);
    node->boolean.value = value;
    return node;
}

ASTNode *ast_make_null(int line)
{
    return make_node(NODE_LITERAL_NULL, line);
}

ASTNode *ast_make_identifier(const char *start, int length, int line)
{
    ASTNode *node = make_node(NODE_IDENTIFIER, line);
    node->identifier.name = malloc(length + 1);
    memcpy(node->identifier.name, start, length);
    node->identifier.name[length] = '\0';
    return node;
}

ASTNode *ast_make_binary(ASTNode *left, ASTNode *right, const char *op, int line)
{
    ASTNode *node = make_node(NODE_BINARY, line);
    node->binary.left = left;
    node->binary.right = right;
    strncpy(node->binary.operators, op, 2);
    node->binary.operators[2] = '\0';
    return node;
}

ASTNode *ast_make_unary(ASTNode *operand, char op, int line)
{
    ASTNode *node = make_node(NODE_UNARY, line);
    node->unary.operand = operand;
    node->unary.operators = op;
    return node;
}

ASTNode *ast_make_var_decl(const char *name, int name_len, ASTNode *value, int line)
{
    ASTNode *node = make_node(NODE_VAR_DECLARATION, line);
    node->var_decl.name = malloc(name_len + 1);
    memcpy(node->var_decl.name, name, name_len);
    node->var_decl.name[name_len] = '\0';
    node->var_decl.value = value;
    return node;
}

ASTNode *ast_make_assign(const char *name, int name_len, ASTNode *value, int line)
{
    ASTNode *node = make_node(NODE_ASSIGN, line);
    node->assign.name = malloc(name_len + 1);
    memcpy(node->assign.name, name, name_len);
    node->assign.name[name_len] = '\0';
    node->assign.value = value;
    return node;
}

ASTNode *ast_make_compound_assign(const char *name, int name_len, ASTNode *value, char op, int line)
{
    ASTNode *node = make_node(NODE_COMPOUND_ASSIGN, line);
    node->compound_assign.name = malloc(name_len + 1);
    memcpy(node->compound_assign.name, name, name_len);
    node->compound_assign.name[name_len] = '\0';
    node->compound_assign.value = value;
    node->compound_assign.operators = op;
    return node;
}

ASTNode *ast_make_increment(const char *name, int name_len, int delta, int line)
{
    ASTNode *node = make_node(NODE_INCREMENT, line);
    node->increment.name = malloc(name_len + 1);
    memcpy(node->increment.name, name, name_len);
    node->increment.name[name_len] = '\0';
    node->increment.delta = delta;
    return node;
}

ASTNode *ast_make_if(ASTNode *condition, ASTNode *then_branch, ASTNode *else_branch, int line)
{
    ASTNode *node = make_node(NODE_IF, line);
    node->if_stmt.condition = condition;
    node->if_stmt.then_branch = then_branch;
    node->if_stmt.else_branch = else_branch;
    return node;
}

ASTNode *ast_make_while(ASTNode *condition, ASTNode *body, int line)
{
    ASTNode *node = make_node(NODE_WHILE, line);
    node->while_stmt.condition = condition;
    node->while_stmt.body = body;
    return node;
}

ASTNode *ast_make_for(ASTNode *init, ASTNode *condition, ASTNode *update, ASTNode *body, int line)
{
    ASTNode *node = make_node(NODE_FOR, line);
    node->for_stmt.init = init;
    node->for_stmt.condition = condition;
    node->for_stmt.update = update;
    node->for_stmt.body = body;
    return node;
}

ASTNode *ast_make_return(ASTNode *value, int line)
{
    ASTNode *node = make_node(NODE_RETURN, line);
    node->return_stmt.value = value;
    return node;
}

ASTNode *ast_make_block(int line)
{
    ASTNode *node = make_node(NODE_BLOCK, line);
    node->program.body.statements = NULL;
    node->program.body.count = 0;
    node->program.body.capacity = 0;
    return node;
}

ASTNode *ast_make_function(const char *name, int name_len, char **params, int param_count, ASTNode *body, int line)
{
    ASTNode *node = make_node(NODE_FUNCTION, line);
    node->function.name = malloc(name_len + 1);
    memcpy(node->function.name, name, name_len);
    node->function.name[name_len] = '\0';
    node->function.params = params;
    node->function.param_count = param_count;
    node->function.body = body;
    return node;
}

ASTNode *ast_make_call(ASTNode *callee, int line)
{
    ASTNode *node = make_node(NODE_CALL, line);
    node->call.callee = callee;
    node->call.args.statements = NULL;
    node->call.args.count = 0;
    node->call.args.capacity = 0;
    return node;
}

ASTNode *ast_make_index(ASTNode *list, ASTNode *index, int line)
{
    ASTNode *node = make_node(NODE_INDEX, line);
    node->index.list = list;
    node->index.index = index;
    return node;
}

ASTNode *ast_make_list(int line)
{
    ASTNode *node = make_node(NODE_LITERAL_LIST, line);
    node->list.elements.statements = NULL;
    node->list.elements.count = 0;
    node->list.elements.capacity = 0;
    return node;
}

ASTNode *ast_make_program(int line)
{
    ASTNode *node = make_node(NODE_PROGRAM, line);
    node->program.body.statements = NULL;
    node->program.body.count = 0;
    node->program.body.capacity = 0;
    return node;
}

ASTNode *ast_make_expr_stmt(ASTNode *expr, int line)
{
    ASTNode *node = make_node(NODE_EXPR_STMT, line);
    node->expr_stmt.expr = expr;
    return node;
}

void ast_free(ASTNode *node)
{
    if (!node)
        return;

    switch (node->type)
    {
    case NODE_LITERAL_STRING:
        free(node->string.value);
        break;
    case NODE_IDENTIFIER:
        free(node->identifier.name);
        break;
    case NODE_BINARY:
        ast_free(node->binary.left);
        ast_free(node->binary.right);
        break;
    case NODE_UNARY:
        ast_free(node->unary.operand);
        break;
    case NODE_VAR_DECLARATION:
        free(node->var_decl.name);
        ast_free(node->var_decl.value);
        break;
    case NODE_ASSIGN:
        free(node->assign.name);
        ast_free(node->assign.value);
        break;
    case NODE_COMPOUND_ASSIGN:
        free(node->compound_assign.name);
        ast_free(node->compound_assign.value);
        break;
    case NODE_INCREMENT:
        free(node->increment.name);
        break;
    case NODE_IF:
        ast_free(node->if_stmt.condition);
        ast_free(node->if_stmt.then_branch);
        ast_free(node->if_stmt.else_branch);
        break;
    case NODE_WHILE:
        ast_free(node->while_stmt.condition);
        ast_free(node->while_stmt.body);
        break;
    case NODE_FOR:
        ast_free(node->for_stmt.init);
        ast_free(node->for_stmt.condition);
        ast_free(node->for_stmt.update);
        ast_free(node->for_stmt.body);
        break;
    case NODE_RETURN:
        ast_free(node->return_stmt.value);
        break;
    case NODE_FUNCTION:
        free(node->function.name);
        for (int i = 0; i < node->function.param_count; i++)
        {
            free(node->function.params[i]);
        }
        free(node->function.params);
        ast_free(node->function.body);
        break;
    case NODE_CALL:
        ast_free(node->call.callee);
        for (int i = 0; i < node->call.args.count; i++)
        {
            ast_free(node->call.args.statements[i]);
        }
        free(node->call.args.statements);
        break;
    case NODE_INDEX:
        ast_free(node->index.list);
        ast_free(node->index.index);
        break;
    case NODE_LITERAL_LIST:
        for (int i = 0; i < node->list.elements.count; i++)
        {
            ast_free(node->list.elements.statements[i]);
        }
        free(node->list.elements.statements);
        break;
    case NODE_PROGRAM:
    case NODE_BLOCK:
        for (int i = 0; i < node->program.body.count; i++)
        {
            ast_free(node->program.body.statements[i]);
        }
        free(node->program.body.statements);
        break;
    case NODE_EXPR_STMT:
        ast_free(node->expr_stmt.expr);
        break;
    default:
        break;
    }

    free(node);
}