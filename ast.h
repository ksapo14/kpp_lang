#ifndef kpp_ast_h
#define kpp_ast_h

#include <stdbool.h>

typedef enum
{
    // statements
    NODE_PROGRAM,
    NODE_VAR_DECLARATION,
    NODE_ASSIGN,
    NODE_IF,
    NODE_WHILE,
    NODE_FOR,
    NODE_RETURN,
    NODE_BLOCK,
    NODE_EXPR_STMT, // expression used as statement e.g. print(x)

    // expressions
    NODE_BINARY,
    NODE_UNARY,
    NODE_LITERAL_NUMBER,
    NODE_LITERAL_STRING,
    NODE_LITERAL_BOOL,
    NODE_LITERAL_NULL,
    NODE_LITERAL_LIST,
    NODE_IDENTIFIER,
    NODE_CALL,
    NODE_INDEX,
    NODE_COMPOUND_ASSIGN, // +=, -=, etc.
    NODE_INCREMENT,       // ++, --
    NODE_FUNCTION,        // fn declaration
} NodeType;

// forward declare so nodes can reference each other
typedef struct ASTNode ASTNode;

typedef struct
{
    ASTNode **statements;
    int count;
    int capacity;
} NodeList;

// the main node struct — a tagged union
struct ASTNode
{
    NodeType type;
    int line;

    union
    {
        // NODE_PROGRAM, NODE_BLOCK
        struct
        {
            NodeList body;
        } program;

        // NODE_LITERAL_NUMBER
        struct
        {
            double value;
        } number;

        // NODE_LITERAL_STRING
        struct
        {
            char *value;
            int length;
        } string;

        // NODE_LITERAL_BOOL
        struct
        {
            bool value;
        } boolean;

        // NODE_LITERAL_LIST
        struct
        {
            NodeList elements;
        } list;

        // NODE_IDENTIFIER
        struct
        {
            char *name;
        } identifier;

        // NODE_BINARY
        struct
        {
            ASTNode *left;
            ASTNode *right;
            char operators[3]; // +, -, *, /, %, ==, !=, <, >, <=, >=
        } binary;

        // NODE_UNARY
        struct
        {
            ASTNode *operand;
            char operators; // - or !
        } unary;

        // NODE_VAR_DECLARATION
        struct
        {
            char *name;
            ASTNode *value; // nullable
        } var_decl;

        // NODE_ASSIGN
        struct
        {
            char *name;
            ASTNode *value;
        } assign;

        // NODE_COMPOUND_ASSIGN
        struct
        {
            char *name;
            ASTNode *value;
            char operators; // +, -, *, /
        } compound_assign;

        // NODE_INCREMENT
        struct
        {
            char *name;
            int delta; // +1 or -1
        } increment;

        // NODE_IF
        struct
        {
            ASTNode *condition;
            ASTNode *then_branch;
            ASTNode *else_branch; // nullable
        } if_stmt;

        // NODE_WHILE
        struct
        {
            ASTNode *condition;
            ASTNode *body;
        } while_stmt;

        // NODE_FOR
        struct
        {
            ASTNode *init;
            ASTNode *condition;
            ASTNode *update;
            ASTNode *body;
        } for_stmt;

        // NODE_RETURN
        struct
        {
            ASTNode *value; // nullable
        } return_stmt;

        // NODE_FUNCTION
        struct
        {
            char *name;
            char **params;
            int param_count;
            ASTNode *body;
        } function;

        // NODE_CALL
        struct
        {
            ASTNode *callee;
            NodeList args;
        } call;

        // NODE_INDEX
        struct
        {
            ASTNode *list;
            ASTNode *index;
        } index;

        // NODE_EXPR_STMT
        struct
        {
            ASTNode *expr;
        } expr_stmt;
    };
};

// constructors
ASTNode *ast_make_number(double value, int line);
ASTNode *ast_make_string(const char *start, int length, int line);
ASTNode *ast_make_bool(bool value, int line);
ASTNode *ast_make_null(int line);
ASTNode *ast_make_identifier(const char *start, int length, int line);
ASTNode *ast_make_binary(ASTNode *left, ASTNode *right, const char *op, int line);
ASTNode *ast_make_unary(ASTNode *operand, char op, int line);
ASTNode *ast_make_var_decl(const char *name, int name_len, ASTNode *value, int line);
ASTNode *ast_make_assign(const char *name, int name_len, ASTNode *value, int line);
ASTNode *ast_make_compound_assign(const char *name, int name_len, ASTNode *value, char op, int line);
ASTNode *ast_make_increment(const char *name, int name_len, int delta, int line);
ASTNode *ast_make_if(ASTNode *condition, ASTNode *then_branch, ASTNode *else_branch, int line);
ASTNode *ast_make_while(ASTNode *condition, ASTNode *body, int line);
ASTNode *ast_make_for(ASTNode *init, ASTNode *condition, ASTNode *update, ASTNode *body, int line);
ASTNode *ast_make_return(ASTNode *value, int line);
ASTNode *ast_make_block(int line);
ASTNode *ast_make_function(const char *name, int name_len, char **params, int param_count, ASTNode *body, int line);
ASTNode *ast_make_call(ASTNode *callee, int line);
ASTNode *ast_make_index(ASTNode *list, ASTNode *index, int line);
ASTNode *ast_make_list(int line);
ASTNode *ast_make_program(int line);
ASTNode *ast_make_expr_stmt(ASTNode *expr, int line);

void nodelist_push(NodeList *list, ASTNode *node);
void ast_free(ASTNode *node);

#endif