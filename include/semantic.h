#ifndef SEMANTIC_H
#define SEMANTIC_H

#include "parser.h"

typedef enum
{
    SYMBOL_INT,
    SYMBOL_FLOAT,
    SYMBOL_STRING
} SymbolType;

typedef struct Symbol
{
    char *name;
    SymbolType type;
    struct Symbol *next;
} Symbol;

typedef struct SymbolTable
{
    Symbol *symbols;
    struct SymbolTable *parent;
} SymbolTable;

typedef struct
{
    char *name;
    char *type;
    char *category;
} Keyword;

void analyze_semantics(JAMZASTNode *ast, Keyword *keywords, int keyword_count);

#endif