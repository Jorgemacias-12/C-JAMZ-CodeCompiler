#ifndef LEXER_H
#define LEXER_H

#include <stdbool.h>

typedef enum
{
    TOKEN_INT,
    TOKEN_RETURN,
    TOKEN_IDENTIFIER,
    TOKEN_NUMBER,
    TOKEN_OPERATOR,
    TOKEN_SEMICOLON,
    TOKEN_LPAREN,
    TOKEN_RPAREN,
    TOKEN_LBRACE,
    TOKEN_RBRACE,
    TOKEN_EOF,
    TOKEN_UNKNOWN,
} TokenType;

typedef struct
{
    TokenType type;
    char *lexeme;
    int line;
    int column;
} Token;

typedef struct TokenNode
{
    Token token;
    struct TokenNode *next;
} TokenNode;

typedef struct
{
    TokenNode *head;
    TokenNode *tail;
    bool has_error;
} TokenList;

TokenList *lexer_analyze(const char *source);
void free_tokens(TokenList *list);

#endif