#ifndef kpp_lexer_h
#define kpp_lexer_h

typedef enum
{
    // literals
    TOKEN_NUMBER,
    TOKEN_IDENTIFIER,
    TOKEN_STRING,

    // keywords
    TOKEN_LET,
    TOKEN_NULL,
    TOKEN_TRUE,
    TOKEN_FALSE,
    TOKEN_FN,
    TOKEN_RETURN,
    TOKEN_IF,
    TOKEN_ELSE,
    TOKEN_WHILE,
    TOKEN_FOR,

    // operators
    TOKEN_PLUS,
    TOKEN_MINUS,
    TOKEN_STAR,
    TOKEN_SLASH,
    TOKEN_PERCENT,
    TOKEN_EQUALS,
    TOKEN_PLUS_EQUALS,
    TOKEN_MINUS_EQUALS,
    TOKEN_STAR_EQUALS,
    TOKEN_SLASH_EQUALS,
    TOKEN_PLUS_PLUS,
    TOKEN_MINUS_MINUS,

    // comparison
    TOKEN_EQUAL_EQUAL,
    TOKEN_BANG_EQUAL,
    TOKEN_LESS,
    TOKEN_LESS_EQUAL,
    TOKEN_GREATER,
    TOKEN_GREATER_EQUAL,

    // grouping
    TOKEN_OPEN_PAREN,
    TOKEN_CLOSE_PAREN,
    TOKEN_OPEN_BRACE,
    TOKEN_CLOSE_BRACE,
    TOKEN_OPEN_BRACKET,
    TOKEN_CLOSE_BRACKET,
    TOKEN_COMMA,
    TOKEN_SEMICOLON,

    // misc
    TOKEN_NEWLINE,
    TOKEN_EOF,
    TOKEN_ERROR,
} TokenType;

typedef struct
{
    TokenType type;
    const char *start; // pointer into source string, no copying
    int length;
    int line;
} Token;

void lexer_init(const char *source);
Token lexer_next_token();

#endif