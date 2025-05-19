#ifndef SEMANTIC_H
#define SEMANTIC_H

#include "parser.h"

typedef enum
{
    SYMBOL_INT,
    SYMBOL_FLOAT,
    SYMBOL_STRING,
    SYMBOL_TYPE,    // Nuevo: para tipos como "int", "float", etc.
    SYMBOL_FUNCTION // Nuevo: para funciones
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
    bool is_freed; // Nuevo campo para rastrear si la tabla ya fue liberada
} SymbolTable;

typedef struct
{
    char *name;
    char *type;
    char *category;
} Keyword;

void analyze_semantics(JAMZASTNode *ast, Keyword *keywords, int keyword_count);

#endif