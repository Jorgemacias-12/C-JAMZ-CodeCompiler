#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <ctype.h>
#include "semantic.h"
#include "parser.h"
#include "utils.h"

typedef struct SimpleSymbol
{
    char *name;
    struct SimpleSymbol *next;
} SimpleSymbol;

typedef struct SimpleSymbolTable
{
    SimpleSymbol *symbols;
    struct SimpleSymbolTable *parent;
} SimpleSymbolTable;

static SimpleSymbol *find_symbol(SimpleSymbolTable *table, const char *name)
{
    for (; table; table = table->parent)
    {
        for (SimpleSymbol *sym = table->symbols; sym; sym = sym->next)
        {
            if (strcmp(sym->name, name) == 0)
                return sym;
        }
    }
    return NULL;
}

static void add_symbol(SimpleSymbolTable *table, const char *name)
{
    SimpleSymbol *sym = malloc(sizeof(SimpleSymbol));
    sym->name = strdup(name);
    sym->next = table->symbols;
    table->symbols = sym;
}

static void free_symbol_table(SimpleSymbolTable *table)
{
    while (table)
    {
        SimpleSymbol *sym = table->symbols;
        while (sym)
        {
            SimpleSymbol *next = sym->next;
            free(sym->name);
            free(sym);
            sym = next;
        }
        SimpleSymbolTable *parent = table->parent;
        free(table);
        table = parent;
    }
}

static bool is_keyword_of_category(const char *name, const char *category, Keyword *keywords, int count)
{
    for (int i = 0; i < count; i++)
    {
        if (strcmp(keywords[i].name, name) == 0 && strcmp(keywords[i].category, category) == 0)
        {
            return true;
        }
    }
    return false;
}

static bool is_valid_type(const char *name, Keyword *keywords, int count)
{
    return is_keyword_of_category(name, "type", keywords, count);
}

static bool is_control_keyword(const char *name, Keyword *keywords, int count)
{
    return is_keyword_of_category(name, "control", keywords, count);
}

static void analyze_node_with_symbols(JAMZASTNode *ast, Keyword *keywords, int keyword_count, SimpleSymbolTable *table)
{
    if (!ast)
        return;
    switch (ast->type)
    {
    case JAMZ_AST_LITERAL:
        // Usar el tipo de token original para distinguir strings/números
        if (ast->literal.value)
        {
            JAMZTokenType ttype = ast->literal.token_type;
            if (ttype != JAMZ_TOKEN_NUMBER && ttype != JAMZ_TOKEN_STRING)
            {
                print_error("Unknown literal: '%s' (line %d, col %d)\n",
                            ast->literal.value, ast->line, ast->column);
            }
        }
        break;
    case JAMZ_AST_VARIABLE:
        if (!find_symbol(table, ast->variable.var_name))
        {
            push_error("Variable '%s' not declared (line %d, col %d)\n", ast->variable.var_name, ast->line, ast->column);
        }
        break;
    case JAMZ_AST_DECLARATION:
        add_symbol(table, ast->declaration.var_name);
        if (ast->declaration.initializer)
            analyze_node_with_symbols(ast->declaration.initializer, keywords, keyword_count, table);
        break;
    case JAMZ_AST_ASSIGNMENT:
        if (!find_symbol(table, ast->assignment.var_name))
        {
            push_error("Variable '%s' not declared (line %d, col %d)\n", ast->assignment.var_name, ast->line, ast->column);
        }
        if (ast->assignment.value)
            analyze_node_with_symbols(ast->assignment.value, keywords, keyword_count, table);
        break;
    case JAMZ_AST_BLOCK:
    case JAMZ_AST_PROGRAM:
    {
        SimpleSymbolTable *local = malloc(sizeof(SimpleSymbolTable));
        local->symbols = NULL;
        local->parent = table;
        for (size_t i = 0; i < ast->block.count; i++)
        {
            analyze_node_with_symbols(ast->block.statements[i], keywords, keyword_count, local);
        }
        free_symbol_table(local);
        break;
    }
    case JAMZ_AST_IF:
        if (ast->if_stmt.condition)
            analyze_node_with_symbols(ast->if_stmt.condition, keywords, keyword_count, table);
        if (ast->if_stmt.then_branch)
            analyze_node_with_symbols(ast->if_stmt.then_branch, keywords, keyword_count, table);
        if (ast->if_stmt.else_branch)
            analyze_node_with_symbols(ast->if_stmt.else_branch, keywords, keyword_count, table);
        break;
    case JAMZ_AST_RETURN:
        if (ast->return_stmt.value)
            analyze_node_with_symbols(ast->return_stmt.value, keywords, keyword_count, table);
        break;
    case JAMZ_AST_BINARY:
        if (ast->binary.left)
            analyze_node_with_symbols(ast->binary.left, keywords, keyword_count, table);
        if (ast->binary.right)
            analyze_node_with_symbols(ast->binary.right, keywords, keyword_count, table);
        break;
    default:
        break;
    }
}

void analyze_semantics(JAMZASTNode *ast, Keyword *keywords, int keyword_count)
{
    SimpleSymbolTable *global = malloc(sizeof(SimpleSymbolTable));
    global->symbols = NULL;
    global->parent = NULL;
    analyze_node_with_symbols(ast, keywords, keyword_count, global);
    free_symbol_table(global);
}