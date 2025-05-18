#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"

typedef enum
{
    JAMZ_AST_PROGRAM,
    JAMZ_AST_BLOCK,
    JAMZ_AST_DECLARATION,
    JAMZ_AST_ASSIGNMENT,
    JAMZ_AST_RETURN,
    JAMZ_AST_IF,
    JAMZ_AST_EXPRESSION,
    JAMZ_AST_BINARY,
    JAMZ_AST_LITERAL,
    JAMZ_AST_VARIABLE
} JAMZASTNodeType;

// Declaración de variable
typedef struct
{
    char *type_name; // "int", "char", etc.
    char *var_name;
    struct JAMZASTNode *initializer; // Puede ser NULL
} JAMZDeclaration;

// Asignación
typedef struct
{
    char *var_name;
    struct JAMZASTNode *value;
} JAMZAssignment;

// Expresión binaria
typedef struct
{
    struct JAMZASTNode *left;
    char *op; // "+", "-", "*", "/"
    struct JAMZASTNode *right;
} JAMZBinaryExpr;

// Variable
typedef struct
{
    char *var_name;
} JAMZVariable;

// Literal
typedef struct
{
    char *value;              // número, string, etc.
    JAMZTokenType token_type; // <--- Añadido: tipo de token original (JAMZ_TOKEN_NUMBER, JAMZ_TOKEN_STRING, etc)
} JAMZLiteral;

// Nodo principal del AST
typedef struct JAMZASTNode
{
    JAMZASTNodeType type;
    union
    {
        JAMZDeclaration declaration;
        JAMZAssignment assignment;
        JAMZBinaryExpr binary;
        JAMZVariable variable;
        JAMZLiteral literal;
        struct
        {
            struct JAMZASTNode **statements;
            size_t count;
        } block;
        struct
        {
            struct JAMZASTNode *value;
        } return_stmt;
        struct
        {
            struct JAMZASTNode *condition;
            struct JAMZASTNode *then_branch;
            struct JAMZASTNode *else_branch;
        } if_stmt;
    };
    int line;
    int column;
} JAMZASTNode;

typedef struct
{
    JAMZTokenList *tokens;
    size_t current;
    bool had_error;
} JAMZParser;

JAMZASTNode *parser_parse(JAMZTokenList *tokens);
void free_ast(JAMZASTNode *node);

#endif