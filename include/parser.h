#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"

typedef enum
{
    JAMZ_AST_PROGRAM,
    JAMZ_AST_BLOCK,
    JAMZ_AST_RETURN,
    JAMZ_AST_IF,
    JAMZ_AST_EXPRESSION,
    JAMZ_AST_BINARY,
    JAMZ_AST_LITERAL,
    JAMZ_AST_VARIABLE
} JAMZASTNodeType;

typedef struct JAMZASTNode JAMZASTNode;

typedef struct
{
    JAMZASTNode *condition;
    JAMZASTNode *then_branch;
    JAMZASTNode *else_branch; // Can be null
} JAMZIfStatement;

typedef struct
{
    JAMZASTNode **statements;
    size_t count;
} JAMZBlock;

typedef struct
{
    JAMZASTNode *value;
} JAMZReturnStatement;

typedef struct
{
    JAMZASTNode *left;
    JAMZToken operator;
    JAMZASTNode *right;
} JAMZBinaryExpr;

typedef struct
{
    JAMZToken name;
} JAMZVariable;

typedef struct
{
    JAMZToken value;
} JAMZLiteral;

struct JAMZASTNode
{
    JAMZASTNodeType type;

    union
    {
        JAMZIfStatement if_stmt;
        JAMZBlock block;
        JAMZReturnStatement return_stmt;
        JAMZBinaryExpr binary;
        JAMZLiteral literal;
        JAMZVariable variable;
    };
    int line;
    int column;
};

typedef struct
{
    JAMZTokenList *tokens;
    size_t current;
    bool had_error;
} JAMZParser;

JAMZASTNode *parser_parse(JAMZTokenList *tokens);
void free_ast(JAMZASTNode *node);

#endif