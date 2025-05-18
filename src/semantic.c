#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>
#include "semantic.h"
#include "parser.h"
#include "utils.h"

typedef struct SimpleSymbol
{
    char *name;
    char *type; // Nuevo: tipo como string (ej: "int", "char", ...)
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

static void add_symbol(SimpleSymbolTable *table, const char *name, const char *type)
{
    SimpleSymbol *sym = malloc(sizeof(SimpleSymbol));
    sym->name = strdup(name);
    sym->type = strdup(type);
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
            free(sym->type);
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
        add_symbol(table, ast->declaration.var_name, ast->declaration.type_name);
        if (ast->declaration.initializer)
            analyze_node_with_symbols(ast->declaration.initializer, keywords, keyword_count, table);
        break;
    case JAMZ_AST_ASSIGNMENT:
    {
        SimpleSymbol *sym = find_symbol(table, ast->assignment.var_name);
        if (!sym)
        {
            push_error("Variable '%s' not declared (line %d, col %d)\n", ast->assignment.var_name, ast->line, ast->column);
        }
        else if (ast->assignment.value)
        {
            // Comprobar tipo del valor asignado
            const char *rhs_type = NULL;
            if (ast->assignment.value->type == JAMZ_AST_LITERAL)
            {
                JAMZTokenType ttype = ast->assignment.value->literal.token_type;
                if (ttype == JAMZ_TOKEN_NUMBER)
                    rhs_type = "int";
                else if (ttype == JAMZ_TOKEN_STRING)
                    rhs_type = "char*";
            }
            else if (ast->assignment.value->type == JAMZ_AST_VARIABLE)
            {
                SimpleSymbol *rhs_sym = find_symbol(table, ast->assignment.value->variable.var_name);
                if (rhs_sym)
                    rhs_type = rhs_sym->type;
            }
            if (rhs_type && strcmp(sym->type, rhs_type) != 0)
            {
                push_error("Type mismatch: cannot assign '%s' to variable '%s' of type '%s' (line %d, col %d)\n",
                           rhs_type, ast->assignment.var_name, sym->type, ast->line, ast->column);
            }
            analyze_node_with_symbols(ast->assignment.value, keywords, keyword_count, table);
        }
        break;
    }
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
    {
        // Comprobación de tipos en operaciones binarias
        const char *left_type = NULL;
        const char *right_type = NULL;
        if (ast->binary.left)
        {
            analyze_node_with_symbols(ast->binary.left, keywords, keyword_count, table);
            if (ast->binary.left->type == JAMZ_AST_LITERAL)
            {
                JAMZTokenType ttype = ast->binary.left->literal.token_type;
                if (ttype == JAMZ_TOKEN_NUMBER)
                    left_type = "int";
                else if (ttype == JAMZ_TOKEN_STRING)
                    left_type = "char*";
            }
            else if (ast->binary.left->type == JAMZ_AST_VARIABLE)
            {
                SimpleSymbol *sym = find_symbol(table, ast->binary.left->variable.var_name);
                if (sym)
                    left_type = sym->type;
            }
        }
        if (ast->binary.right)
        {
            analyze_node_with_symbols(ast->binary.right, keywords, keyword_count, table);
            if (ast->binary.right->type == JAMZ_AST_LITERAL)
            {
                JAMZTokenType ttype = ast->binary.right->literal.token_type;
                if (ttype == JAMZ_TOKEN_NUMBER)
                    right_type = "int";
                else if (ttype == JAMZ_TOKEN_STRING)
                    right_type = "char*";
            }
            else if (ast->binary.right->type == JAMZ_AST_VARIABLE)
            {
                SimpleSymbol *sym = find_symbol(table, ast->binary.right->variable.var_name);
                if (sym)
                    right_type = sym->type;
            }
        }
        // Solo permitimos operaciones entre int por ahora
        if (left_type && right_type && strcmp(left_type, right_type) != 0)
        {
            push_error("Type mismatch in binary operation: '%s' vs '%s' (line %d, col %d)\n",
                       left_type, right_type, ast->line, ast->column);
        }
        else if (left_type && strcmp(left_type, "int") != 0)
        {
            push_error("Only 'int' type supported in binary operations (line %d, col %d)\n", ast->line, ast->column);
        }
        break;
    }
    default:
        break;
    }
}

// Imprime la tabla de símbolos como un árbol (AST)
void print_symbol_table_ast(const SimpleSymbolTable *table, int indent)
{
    if (!table)
        return;
    for (const SimpleSymbol *sym = table->symbols; sym; sym = sym->next)
    {
        for (int i = 0; i < indent; ++i)
            printf("  ");
        printf("|- %s : %s\n", sym->name, sym->type);
    }
    if (table->parent)
    {
        print_symbol_table_ast(table->parent, indent + 1);
    }
}

void analyze_semantics(JAMZASTNode *ast, Keyword *keywords, int keyword_count)
{
    SimpleSymbolTable *global = malloc(sizeof(SimpleSymbolTable));
    global->symbols = NULL;
    global->parent = NULL;
    analyze_node_with_symbols(ast, keywords, keyword_count, global);
    printf("\nVariables y tipos encontrados por el análisis semántico (formato árbol):\n");
    print_symbol_table_ast(global, 0);
    free_symbol_table(global);
}