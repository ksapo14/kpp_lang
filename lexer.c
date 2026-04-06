#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "lexer.h"

typedef struct
{
    const char *start;   // start of current token
    const char *current; // current character
    int line;
} Lexer;

static Lexer lexer;

void lexer_init(const char *source)
{
    lexer.start = source;
    lexer.current = source;
    lexer.line = 1;
}

// helpers
static bool is_at_end()
{
    return *lexer.current == '\0';
}

static char advance()
{
    return *lexer.current++;
}

static char peek()
{
    return *lexer.current;
}

static char peek_next()
{
    if (is_at_end())
        return '\0';
    return *(lexer.current + 1);
}

static bool match(char expected)
{
    if (is_at_end())
        return false;
    if (*lexer.current != expected)
        return false;
    lexer.current++;
    return true;
}

static bool is_digit(char c)
{
    return c >= '0' && c <= '9';
}

static bool is_alpha(char c)
{
    return (c >= 'a' && c <= 'z') ||
           (c >= 'A' && c <= 'Z') ||
           c == '_';
}

// token builders
static Token make_token(TokenType type)
{
    Token token;
    token.type = type;
    token.start = lexer.start;
    token.length = (int)(lexer.current - lexer.start);
    token.line = lexer.line;
    return token;
}

static Token error_token(const char *message)
{
    Token token;
    token.type = TOKEN_ERROR;
    token.start = message;
    token.length = (int)strlen(message);
    token.line = lexer.line;
    return token;
}

// keyword check
static TokenType check_keyword(int start, int length, const char *rest, TokenType type)
{
    if (lexer.current - lexer.start == start + length &&
        memcmp(lexer.start + start, rest, length) == 0)
    {
        return type;
    }
    return TOKEN_IDENTIFIER;
}

static TokenType identifier_type()
{
    switch (lexer.start[0])
    {
    case 'l':
        return check_keyword(1, 2, "et", TOKEN_LET);
    case 'n':
        return check_keyword(1, 3, "ull", TOKEN_NULL);
    case 't':
        return check_keyword(1, 3, "rue", TOKEN_TRUE);
    case 'f':
        if (lexer.current - lexer.start > 1)
        {
            switch (lexer.start[1])
            {
            case 'a':
                return check_keyword(2, 3, "lse", TOKEN_FALSE);
            case 'n':
                return check_keyword(1, 1, "n", TOKEN_FN);
            case 'o':
                return check_keyword(2, 1, "r", TOKEN_FOR);
            }
        }
        break;
    case 'r':
        return check_keyword(1, 5, "eturn", TOKEN_RETURN);
    case 'i':
        return check_keyword(1, 1, "f", TOKEN_IF);
    case 'e':
        return check_keyword(1, 3, "lse", TOKEN_ELSE);
    case 'w':
        return check_keyword(1, 4, "hile", TOKEN_WHILE);
    }
    return TOKEN_IDENTIFIER;
}

// token scanners
static Token scan_number()
{
    while (is_digit(peek()))
        advance();

    // decimal support for later
    if (peek() == '.' && is_digit(peek_next()))
    {
        advance(); // consume '.'
        while (is_digit(peek()))
            advance();
    }

    return make_token(TOKEN_NUMBER);
}

static Token scan_identifier()
{
    while (is_alpha(peek()) || is_digit(peek()))
        advance();
    return make_token(identifier_type());
}

static Token scan_string()
{
    while (peek() != '"' && !is_at_end())
    {
        if (peek() == '\n')
            lexer.line++;
        advance();
    }

    if (is_at_end())
        return error_token("Unterminated string.");

    advance(); // closing quote
    return make_token(TOKEN_STRING);
}

static void skip_whitespace()
{
    while (true)
    {
        char c = peek();
        switch (c)
        {
        case ' ':
        case '\r':
        case '\t':
            advance();
            break;
        // skip comments
        case '/':
            if (peek_next() == '/')
            {
                while (peek() != '\n' && !is_at_end())
                    advance();
            }
            else
            {
                return;
            }
            break;
        default:
            return;
        }
    }
}

Token lexer_next_token()
{
    skip_whitespace();
    lexer.start = lexer.current;

    if (is_at_end())
        return make_token(TOKEN_EOF);

    char c = advance();

    if (is_digit(c))
        return scan_number();
    if (is_alpha(c))
        return scan_identifier();

    switch (c)
    {
    case '(':
        return make_token(TOKEN_OPEN_PAREN);
    case ')':
        return make_token(TOKEN_CLOSE_PAREN);
    case '{':
        return make_token(TOKEN_OPEN_BRACE);
    case '}':
        return make_token(TOKEN_CLOSE_BRACE);
    case '[':
        return make_token(TOKEN_OPEN_BRACKET);
    case ']':
        return make_token(TOKEN_CLOSE_BRACKET);
    case ',':
        return make_token(TOKEN_COMMA);
    case ';':
        return make_token(TOKEN_SEMICOLON);
    case '\n':
        lexer.line++;
        return make_token(TOKEN_NEWLINE);

    // two character tokens
    case '+':
        if (match('+'))
            return make_token(TOKEN_PLUS_PLUS);
        if (match('='))
            return make_token(TOKEN_PLUS_EQUALS);
        return make_token(TOKEN_PLUS);
    case '-':
        if (match('-'))
            return make_token(TOKEN_MINUS_MINUS);
        if (match('='))
            return make_token(TOKEN_MINUS_EQUALS);
        return make_token(TOKEN_MINUS);
    case '*':
        if (match('='))
            return make_token(TOKEN_STAR_EQUALS);
        return make_token(TOKEN_STAR);
    case '/':
        if (match('='))
            return make_token(TOKEN_SLASH_EQUALS);
        return make_token(TOKEN_SLASH);
    case '%':
        return make_token(TOKEN_PERCENT);
    case '=':
        if (match('='))
            return make_token(TOKEN_EQUAL_EQUAL);
        return make_token(TOKEN_EQUALS);
    case '!':
        if (match('='))
            return make_token(TOKEN_BANG_EQUAL);
        return error_token("Expected '=' after '!'.");
    case '<':
        if (match('='))
            return make_token(TOKEN_LESS_EQUAL);
        return make_token(TOKEN_LESS);
    case '>':
        if (match('='))
            return make_token(TOKEN_GREATER_EQUAL);
        return make_token(TOKEN_GREATER);
    case '"':
        return scan_string();
    }

    return error_token("Unrecognized character.");
}