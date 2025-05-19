#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>
#include <locale.h>
#include <stdarg.h>
#include "semantic.h"
#include "parser.h"
#include "utils.h"

static Symbol *find_symbol(SymbolTable *table, const char *name)
{
    for (; table; table = table->parent)
    {
        for (Symbol *sym = table->symbols; sym; sym = sym->next)
        {
            if (strcmp(sym->name, name) == 0)
                return sym;
        }
    }
    return NULL;
}

// Cambiar asignaciones de sym->type para usar SymbolType en lugar de char *
static void add_symbol(SymbolTable *table, const char *name, SymbolType type)
{
    Symbol *sym = safe_malloc(sizeof(Symbol));
    sym->name = strdup(name);
    sym->type = type; // Usar directamente el enum SymbolType
    sym->next = table->symbols;
    table->symbols = sym;
}

// Ajustar free_symbol_table para verificar el campo is_freed antes de liberar la tabla y marcarla como liberada después de la operación
void free_symbol_table(SymbolTable *table)
{
    if (table == NULL || table->is_freed)
    {
        log_debug("[LOG] Intento de liberar una tabla ya liberada o nula.\n");
        return;
    }

    Symbol *sym = table->symbols;
    while (sym != NULL)
    {
        Symbol *next = sym->next;
        if (sym->name != NULL)
        {
            log_debug("[LOG] Liberando símbolo: %s\n", sym->name);
            free(sym->name);
        }
        free(sym);
        sym = next;
    }

    table->is_freed = true; // Marcar la tabla como liberada
    log_debug("[LOG] Tabla de símbolos liberada correctamente.\n");
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
    for (int i = 0; i < count; i++)
    {
        if (strcmp(keywords[i].name, name) == 0 && strcmp(keywords[i].category, "type") == 0)
        {
            return true;
        }
    }
    return false;
}

static bool is_control_keyword(const char *name, Keyword *keywords, int count)
{
    return is_keyword_of_category(name, "control", keywords, count);
}

// Ajustar comparaciones en analyze_node_with_symbols para usar SymbolType
static void analyze_node_with_symbols(JAMZASTNode *ast, Keyword *keywords, int keyword_count, SymbolTable *table)
{
    if (!ast)
    {
        log_debug("Nodo AST nulo\n");
        return;
    }
    log_debug("Analizando nodo AST de tipo %d\n", ast->type);
    switch (ast->type)
    {
    case JAMZ_AST_LITERAL:
        if (ast->literal.value)
        {
            log_debug("Literal encontrado: %s\n", ast->literal.value);
            JAMZTokenType ttype = ast->literal.token_type;
            if (ttype != JAMZ_TOKEN_NUMBER && ttype != JAMZ_TOKEN_STRING && ttype != JAMZ_TOKEN_CHAR)
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
        log_debug("Declaración de variable: %s de tipo %s\n", ast->declaration.var_name, ast->declaration.type_name);
        SymbolType type;
        if (strcmp(ast->declaration.type_name, "int") == 0)
            type = SYMBOL_INT;
        else if (strcmp(ast->declaration.type_name, "float") == 0)
            type = SYMBOL_FLOAT;
        else if (strcmp(ast->declaration.type_name, "string") == 0)
            type = SYMBOL_STRING;
        else
        {
            push_error("Tipo '%s' no válido para la variable '%s' (línea %d, col %d)\n",
                       ast->declaration.type_name, ast->declaration.var_name, ast->line, ast->column);
            return;
        }
        add_symbol(table, ast->declaration.var_name, type);
        if (ast->declaration.initializer)
            analyze_node_with_symbols(ast->declaration.initializer, keywords, keyword_count, table);
        break;
    case JAMZ_AST_ASSIGNMENT:
    {
        log_debug("Asignación a la variable: %s\n", ast->assignment.var_name);
        Symbol *sym = find_symbol(table, ast->assignment.var_name);
        if (!sym)
        {
            push_error("Variable '%s' no declarada (línea %d, col %d)\n", ast->assignment.var_name, ast->line, ast->column);
            return;
        }
        SymbolType rhs_type;
        if (ast->assignment.value->type == JAMZ_AST_LITERAL)
        {
            if (ast->assignment.value->literal.token_type == JAMZ_TOKEN_NUMBER)
                rhs_type = SYMBOL_INT;
            else if (ast->assignment.value->literal.token_type == JAMZ_TOKEN_STRING)
                rhs_type = SYMBOL_STRING;
            else
            {
                push_error("Tipo de literal no soportado (línea %d, col %d)\n", ast->line, ast->column);
                return;
            }
        }
        else
        {
            push_error("Tipo de asignación no soportado (línea %d, col %d)\n", ast->line, ast->column);
            return;
        }
        if (sym->type != rhs_type)
        {
            push_error("Incompatibilidad de tipos: no se puede asignar '%d' a la variable '%s' de tipo '%d' (línea %d, col %d)\n",
                       rhs_type, ast->assignment.var_name, sym->type, ast->line, ast->column);
        }
        analyze_node_with_symbols(ast->assignment.value, keywords, keyword_count, table);
        break;
    }
    case JAMZ_AST_BLOCK:
    case JAMZ_AST_PROGRAM:
    {
        log_debug("Creando tabla de símbolos local\n");
        SymbolTable *local = safe_malloc(sizeof(SymbolTable));
        local->symbols = NULL;
        local->parent = table;
        log_debug("Creando tabla de símbolos local en %p\n", (void *)local);
        for (size_t i = 0; i < ast->block.count; i++)
        {
            analyze_node_with_symbols(ast->block.statements[i], keywords, keyword_count, local);
        }
        // free_symbol_table(local);
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
                Symbol *sym = find_symbol(table, ast->binary.left->variable.var_name);
                if (sym)
                    left_type = sym->type == SYMBOL_INT ? "int" : "char*";
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
                Symbol *sym = find_symbol(table, ast->binary.right->variable.var_name);
                if (sym)
                    right_type = sym->type == SYMBOL_INT ? "int" : "char*";
            }
        }
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
        log_debug("Nodo AST no manejado: tipo %d\n", ast->type);
        break;
    }
}

// Imprime la tabla de símbolos como un árbol (solo la tabla actual, no los padres)
void print_symbol_table_ast(const SymbolTable *table, int indent)
{
    if (!table)
    {
        return;
    }
    for (const Symbol *sym = table->symbols; sym; sym = sym->next)
    {
        for (int i = 0; i < indent; ++i)
            printf("  ");
        printf("|- %s : %d\n", sym->name, sym->type);
    }
}

// Agregar un indicador para rastrear si la tabla ya fue liberada
static bool is_table_freed(SymbolTable *table)
{
    return table == NULL || (table->symbols == NULL && table->parent == NULL);
}

// Modificar analyze_semantics para evitar llamadas redundantes
void analyze_semantics(JAMZASTNode *ast, Keyword *keywords, int keyword_count)
{
    SymbolTable *global = safe_malloc(sizeof(SymbolTable));
    global->symbols = NULL;
    global->parent = NULL;

    for (int i = 0; i < keyword_count; i++)
    {
        if (strcmp(keywords[i].category, "type") == 0)
        {
            add_symbol(global, keywords[i].name, SYMBOL_TYPE);
        }
        else if (strcmp(keywords[i].category, "function") == 0)
        {
            add_symbol(global, keywords[i].name, SYMBOL_FUNCTION);
        }
    }

    analyze_node_with_symbols(ast, keywords, keyword_count, global);
    print_symbol_table_ast(global, 0);

    // Verificar si la tabla ya fue liberada antes de intentar liberarla
    if (!is_table_freed(global))
    {
        free_symbol_table(global);
    }
    else
    {
        log_debug("[TRACE] Intento de liberar tabla global ya liberada o nula\n");
    }
}