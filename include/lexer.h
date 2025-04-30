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
    TOKEN_STRING,
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
    int line;
    int column;
    char character;
    char *message;
} LexerError;

typedef struct
{
    TokenNode *head;
    TokenNode *tail;
    LexerError *errors;
    int error_count;
    int error_capacity;
    bool has_error;
} TokenList;

TokenList *lexer_analyze(const char *source_code);
void free_tokens(TokenList *list);
void print_lexer_errors(const TokenList *list);

#endif