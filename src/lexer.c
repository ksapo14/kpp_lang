#include <ctype.h>
#include <string.h>

#include "lexer.h"

static bool isAtEnd(Lexer *lexer)
{
    return *lexer->current == '\0';
}

static Token makeToken(Lexer *lexer, TokenType type)
{
    Token token;
    token.type = type;
    token.start = lexer->start;
    token.length = (int)(lexer->current - lexer->start);
    token.line = lexer->startLine;
    token.column = lexer->startColumn;
    return token;
}

static Token errorToken(Lexer *lexer, const char *message)
{
    Token token;
    token.type = TOKEN_ERROR;
    token.start = message;
    token.length = (int)strlen(message);
    token.line = lexer->line;
    token.column = lexer->column;
    return token;
}

static char advanceChar(Lexer *lexer)
{
    char c = *lexer->current++;
    if (c == '\n')
    {
        lexer->line++;
        lexer->column = 1;
    }
    else
    {
        lexer->column++;
    }
    return c;
}

static char peek(Lexer *lexer)
{
    return *lexer->current;
}

static char peekNext(Lexer *lexer)
{
    if (isAtEnd(lexer))
        return '\0';
    return lexer->current[1];
}

static bool matchChar(Lexer *lexer, char expected)
{
    if (isAtEnd(lexer))
        return false;
    if (*lexer->current != expected)
        return false;

    advanceChar(lexer);
    return true;
}

static void skipWhitespace(Lexer *lexer)
{
    for (;;)
    {
        char c = peek(lexer);
        switch (c)
        {
        case ' ':
        case '\r':
        case '\t':
            advanceChar(lexer);
            break;
        case '\n':
            advanceChar(lexer);
            break;
        case '/':
            if (peekNext(lexer) == '/')
            {
                while (peek(lexer) != '\n' && !isAtEnd(lexer))
                    advanceChar(lexer);
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

static bool isAlpha(char c)
{
    return isalpha((unsigned char)c) || c == '_';
}

static bool isDigit(char c)
{
    return isdigit((unsigned char)c);
}

static TokenType checkKeyword(Lexer *lexer, int start, int length, const char *rest, TokenType type)
{
    if (lexer->current - lexer->start == start + length &&
        memcmp(lexer->start + start, rest, (size_t)length) == 0)
    {
        return type;
    }
    return TOKEN_IDENTIFIER;
}

static TokenType identifierType(Lexer *lexer)
{
    switch (lexer->start[0])
    {
    case 'a':
        return checkKeyword(lexer, 1, 2, "nd", TOKEN_AND);
    case 'e':
        return checkKeyword(lexer, 1, 3, "lse", TOKEN_ELSE);
    case 'f':
        if (lexer->current - lexer->start > 1)
        {
            switch (lexer->start[1])
            {
            case 'a':
                return checkKeyword(lexer, 2, 3, "lse", TOKEN_FALSE);
            case 'u':
                return checkKeyword(lexer, 2, 1, "n", TOKEN_FUN);
            }
        }
        break;
    case 'i':
        return checkKeyword(lexer, 1, 1, "f", TOKEN_IF);
    case 'l':
        return checkKeyword(lexer, 1, 2, "et", TOKEN_LET);
    case 'n':
        return checkKeyword(lexer, 1, 2, "il", TOKEN_NIL);
    case 'o':
        return checkKeyword(lexer, 1, 1, "r", TOKEN_OR);
    case 'p':
        return checkKeyword(lexer, 1, 4, "rint", TOKEN_PRINT);
    case 'r':
        return checkKeyword(lexer, 1, 5, "eturn", TOKEN_RETURN);
    case 't':
        return checkKeyword(lexer, 1, 3, "rue", TOKEN_TRUE);
    case 'w':
        return checkKeyword(lexer, 1, 4, "hile", TOKEN_WHILE);
    }

    return TOKEN_IDENTIFIER;
}

static Token identifier(Lexer *lexer)
{
    while (isAlpha(peek(lexer)) || isDigit(peek(lexer)))
        advanceChar(lexer);

    return makeToken(lexer, identifierType(lexer));
}

static Token number(Lexer *lexer)
{
    while (isDigit(peek(lexer)))
        advanceChar(lexer);

    if (peek(lexer) == '.' && isDigit(peekNext(lexer)))
    {
        advanceChar(lexer);
        while (isDigit(peek(lexer)))
            advanceChar(lexer);
    }

    return makeToken(lexer, TOKEN_NUMBER);
}

static Token string(Lexer *lexer)
{
    while (peek(lexer) != '"' && !isAtEnd(lexer))
        advanceChar(lexer);

    if (isAtEnd(lexer))
        return errorToken(lexer, "Unterminated string.");

    advanceChar(lexer);
    return makeToken(lexer, TOKEN_STRING);
}

void initLexer(Lexer *lexer, const char *source)
{
    lexer->start = source;
    lexer->current = source;
    lexer->line = 1;
    lexer->column = 1;
    lexer->startLine = 1;
    lexer->startColumn = 1;
}

Token scanToken(Lexer *lexer)
{
    skipWhitespace(lexer);

    lexer->start = lexer->current;
    lexer->startLine = lexer->line;
    lexer->startColumn = lexer->column;

    if (isAtEnd(lexer))
        return makeToken(lexer, TOKEN_EOF);

    char c = advanceChar(lexer);
    if (isAlpha(c))
        return identifier(lexer);
    if (isDigit(c))
        return number(lexer);

    switch (c)
    {
    case '(':
        return makeToken(lexer, TOKEN_LEFT_PAREN);
    case ')':
        return makeToken(lexer, TOKEN_RIGHT_PAREN);
    case '{':
        return makeToken(lexer, TOKEN_LEFT_BRACE);
    case '}':
        return makeToken(lexer, TOKEN_RIGHT_BRACE);
    case ';':
        return makeToken(lexer, TOKEN_SEMICOLON);
    case ',':
        return makeToken(lexer, TOKEN_COMMA);
    case '.':
        return makeToken(lexer, TOKEN_DOT);
    case '-':
        return makeToken(lexer, TOKEN_MINUS);
    case '+':
        return makeToken(lexer, TOKEN_PLUS);
    case '/':
        return makeToken(lexer, TOKEN_SLASH);
    case '*':
        return makeToken(lexer, TOKEN_STAR);
    case '!':
        return makeToken(lexer, matchChar(lexer, '=') ? TOKEN_BANG_EQUAL : TOKEN_BANG);
    case '=':
        return makeToken(lexer, matchChar(lexer, '=') ? TOKEN_EQUAL_EQUAL : TOKEN_EQUAL);
    case '<':
        return makeToken(lexer, matchChar(lexer, '=') ? TOKEN_LESS_EQUAL : TOKEN_LESS);
    case '>':
        return makeToken(lexer, matchChar(lexer, '=') ? TOKEN_GREATER_EQUAL : TOKEN_GREATER);
    case '"':
        return string(lexer);
    }

    return errorToken(lexer, "Unexpected character.");
}
