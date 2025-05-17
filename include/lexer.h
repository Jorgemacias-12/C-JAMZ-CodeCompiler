#ifndef LEXER_H
#define LEXER_H

#include <stdbool.h>
#include <stdlib.h>

typedef enum
{
    JAMZ_TOKEN_INT,
    JAMZ_TOKEN_RETURN,
    JAMZ_TOKEN_IDENTIFIER,
    JAMZ_TOKEN_NUMBER,
    JAMZ_TOKEN_OPERATOR,
    JAMZ_TOKEN_SEMICOLON,
    JAMZ_TOKEN_LPAREN,
    JAMZ_TOKEN_RPAREN,
    JAMZ_TOKEN_LBRACE,
    JAMZ_TOKEN_RBRACE,
    JAMZ_TOKEN_STRING,
    JAMZ_TOKEN_EOF,
    JAMZ_TOKEN_UNKNOWN
} JAMZTokenType;

typedef struct
{
    JAMZTokenType type;
    char *lexeme;
    int line;
    int column;
} JAMZToken;

typedef struct
{
    int line;
    int column;
    char character;
    char *message;
} JAMZLexerError;

typedef struct
{
    JAMZToken *tokens;
    size_t count;
    size_t capacity;
    bool has_error;
    JAMZLexerError *errors;
    size_t error_count;
} JAMZTokenList;

JAMZTokenList *lexer_analyze(const char *source);
const char *jamz_token_type_to_string(JAMZTokenType type);
void print_tokens(const JAMZTokenList *list);
void free_tokens(JAMZTokenList *list);

#endif
